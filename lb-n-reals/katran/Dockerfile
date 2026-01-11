FROM bpf_base

LABEL os.version="Ubuntu 22.04.5 LTS"
LABEL maintainer="docker-katran-lb"

# Install necessary utilities
RUN apt-get update && \
    apt install -y pkg-config libiberty-dev cmake clang-13 libfmt-dev && \
    apt install -y sudo protobuf-compiler && \
    rm -rf /var/lib/apt/lists/* 

# Create the non-root user 'simple_user'
# Add 'simple_user' to the 'sudo' group
# This allows the user to run commands with elevated privileges (if needed).
# Add NOPASSWD setting for simple_user (must be done as root)
RUN groupadd --gid 1000 simple_user && \
    useradd --uid 1000 --gid simple_user --shell /bin/bash --create-home simple_user && \
    usermod -aG sudo simple_user && \
    echo "simple_user ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

# Perform specific RUN commands as 'simple_user'
USER simple_user

WORKDIR /home/simple_user

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

# change the argument below to invalidate cache and re-run git clone
ARG CLONE_CACHE_BUSTER=1

# Clone the forked Katran repository
RUN git clone https://github.com/nickpapakon/katran.git && \
    git config --global --add safe.directory /home/simple_user/katran

WORKDIR /home/simple_user/katran

# Git checkout to working branch and make the build script executable
RUN git checkout mqtt_LB && \
    chmod +x build_katran.sh

RUN echo "[Katran Build] build only Katran dependencies" && \
    export BUILD_DEPS=1 && \
    ./build_katran.sh

RUN echo "[Katran Build] build bpftool and grpc only" && \
    export BUILD_EXAMPLE_GRPC=1 && \
    export BUILD_KATRAN_TPR=1 && \
    ./build_katran.sh

# if needed to re-build Katran because a src file changed
# change the argument below to invalidate docker build cache for the Katran build step
# ARG KATRAN_BUILD_CACHE_BUSTER=1
# RUN git pull

RUN echo "[Katran Build] build and test Katran" && \
    export BUILD_AND_TEST=1 && \
    ./build_katran.sh

# If changes in Go client done 
# change the argument here to invalidate docker build cache for the Go client build step
# ARG GO_CLIENT_BUILD_CACHE_BUSTER=1

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
