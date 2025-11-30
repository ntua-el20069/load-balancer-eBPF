FROM ubuntu:jammy

LABEL os.version="Ubuntu 22.04.5 LTS"
LABEL maintainer="docker-katran-lb"

# Install necessary utilities
RUN apt-get update && \
    apt-get install -y iproute2 net-tools traceroute && \
    apt install build-essential -y && \
    apt install -y git pkg-config libiberty-dev cmake clang-13 libelf-dev libfmt-dev && \
    apt install -y wget vim curl iputils-ping ethtool protobuf-compiler && \
    apt install -y tcpdump && \
    rm -rf /var/lib/apt/lists/* 

# Create the non-root user 'simple_user'
RUN groupadd --gid 1000 simple_user && \
    useradd --uid 1000 --gid simple_user --shell /bin/bash --create-home simple_user

# Add 'simple_user' to the 'sudo' group
# This allows the user to run commands with elevated privileges (if needed).
RUN apt update && apt install -y sudo && \
    usermod -aG sudo simple_user && \
    rm -rf /var/lib/apt/lists/*


# Add NOPASSWD setting for simple_user (must be done as root)
RUN echo "simple_user ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

# Perform specific RUN commands as 'simple_user'
USER simple_user

# Example: Run a command to create a directory in the user's home and set permissions
RUN mkdir -p /home/simple_user/data && \
    touch /home/simple_user/data/testfile.txt

WORKDIR /home/simple_user

# Clone the forked Katran repository
RUN git clone https://github.com/nickpapakon/katran.git && \
    git config --global --add safe.directory /home/simple_user/katran

WORKDIR /home/simple_user/katran

# Build Katran
RUN chmod +x build_katran.sh && \
    ./build_katran.sh

# Install Go 1.25.1 and set up environment variables and install packages
RUN sudo rm -rf /usr/local/go && \ 
    sudo rm -rf /usr/local/go* && \
    sudo rm -rf /home/simple_user/go && \
    sudo rm -rf /usr/bin/go && \
    wget https://go.dev/dl/go1.25.1.linux-amd64.tar.gz && \
    sudo tar -C /usr/local -xzf go1.25.1.linux-amd64.tar.gz

ENV PATH="/usr/local/go/bin:${PATH}"
ENV GOPATH="/home/simple_user/go"
ENV PATH="${GOPATH}/bin:${PATH}"

RUN go install google.golang.org/protobuf/cmd/protoc-gen-go@latest && \
    go install google.golang.org/grpc/cmd/protoc-gen-go-grpc@latest && \
    cp $(go env GOPATH)/bin/protoc-gen-go-grpc $(go env GOPATH)/bin/protoc-gen-go_grpc

WORKDIR /home/simple_user/katran/example_grpc

# Build the gRPC client
RUN go mod init example_grpc && \
    go mod tidy && \
    ./build_grpc_client.sh


WORKDIR /home/simple_user

COPY . .
RUN sudo chmod +x /home/simple_user/setup.sh

# Add a script to set up interfaces or other configurations at container startup (need root access)
USER root

ENTRYPOINT [ "/home/simple_user/setup.sh" ]
