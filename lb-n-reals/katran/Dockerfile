# The 'jammy' tag officially refers to Ubuntu 22.04 LTS.
# This image is built for the standard x86-64 architecture by default.
FROM ubuntu:jammy

# Optional: Set a label to confirm the OS version
LABEL os.version="Ubuntu 22.04.5 LTS"

# Set a helpful maintainer label
LABEL maintainer="docker-katran-lb"

# Install necessary network utilities (iproute2 is usually pre-installed but good practice)
# We also install 'net-tools' for commands like ifconfig, though 'ip' is preferred.
RUN apt-get update && \
    apt-get install -y iproute2 net-tools traceroute && \
    apt install build-essential -y && \
    apt install -y git pkg-config libiberty-dev cmake clang-13 libelf-dev libfmt-dev && \
    apt install -y wget vim curl iputils-ping ethtool protobuf-compiler && \
    apt install -y tcpdump && \
    ## for libbpf and bpftool
    # apt-get install -y apt-transport-https ca-certificates curl clang llvm jq && \
    # apt-get install -y libelf-dev libpcap-dev libbfd-dev binutils-dev build-essential make  && \
    # apt-get install -y linux-tools-common linux-tools-$(uname -r)  && \
    # apt-get install -y bpfcc-tools && \
    # apt-get install -y python3-pip && \
    rm -rf /var/lib/apt/lists/* 

# for libbpf and bpftool, need root access for some make install cmds
# RUN git clone --recurse-submodules https://github.com/lizrice/learning-ebpf && \
#     cd learning-ebpf && \
#     cd libbpf/src && \
#     make install && \
#     cd ../.. && \
#     cd ..  && \
#     git clone --recurse-submodules https://github.com/libbpf/bpftool.git && \
#     cd bpftool/src  && \
#     make install  && \
#     cd ..


# make sure that BPF's jit is enabled
# RUN sysctl net.core.bpf_jit_enable=1

# Create the non-root user 'simple_user'
# The user ID (1000) and group ID (1000) are common choices for non-root users.
RUN groupadd --gid 1000 simple_user && \
    useradd --uid 1000 --gid simple_user --shell /bin/bash --create-home simple_user

# Add 'simple_user' to the 'sudo' group
# This allows the user to run commands with elevated privileges (if needed).
# Note: For security, giving sudo access in a container is often discouraged, 
# but it aligns with your specific request.
RUN apt update && apt install -y sudo && \
    usermod -aG sudo simple_user && \
    rm -rf /var/lib/apt/lists/*


# Add NOPASSWD setting for simple_user (must be done as root)
# This uses the 'visudo' utility (via tee) to add a line granting passwordless sudo access.
RUN echo "simple_user ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

# Perform specific RUN commands as 'simple_user'
# Use the 'USER' instruction before any command you want to run as this user.
USER simple_user

# Example: Run a command to create a directory in the user's home and set permissions
RUN mkdir -p /home/simple_user/data && \
    touch /home/simple_user/data/testfile.txt

WORKDIR /home/simple_user

RUN git clone https://github.com/ntua-el20069/katran.git && \
    git config --global --add safe.directory /home/simple_user/katran

WORKDIR /home/simple_user/katran

RUN chmod +x build_katran.sh && \
    ./build_katran.sh


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

RUN go mod init example_grpc && \
    go mod tidy && \
    ./build_grpc_client.sh


WORKDIR /home/simple_user

COPY . .
RUN sudo chmod +x /home/simple_user/setup.sh

# Add a script to set up interfaces or other configurations at container startup
USER root
ENTRYPOINT [ "/home/simple_user/setup.sh" ]
