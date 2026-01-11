FROM ubuntu:jammy AS bpf_base_image
LABEL os.version="Ubuntu 22.04.5 LTS"

RUN apt-get update && \
    apt-get install -y git clang llvm libelf-dev libpcap-dev  libssl-dev build-essential make libc6-dev-i386 m4 && \ 
    apt-get install -y linux-tools-common && \
    apt-get install -y iproute2 net-tools traceroute tcpdump && \
    apt-get install -y wget vim curl iputils-ping ethtool sudo && \
    rm -rf /var/lib/apt/lists/*

    #  Katran dependencies
    # apt install -y pkg-config libiberty-dev cmake clang-13 libfmt-dev && \
    # apt install -y sudo protobuf-compiler && \
