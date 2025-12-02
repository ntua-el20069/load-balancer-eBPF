FROM ubuntu:jammy
LABEL os.version="Ubuntu 22.04.5 LTS"
LABEL maintainer="docker-lb-from-scratch"

RUN apt-get update && \
    apt-get install -y git clang llvm libelf-dev libpcap-dev  libssl-dev build-essential make && \ 
    apt-get install -y linux-tools-common && \
    apt-get install -y iproute2 net-tools traceroute && \
    apt-get install -y wget vim curl iputils-ping ethtool sudo && \
    rm -rf /var/lib/apt/lists/*

COPY . /lb_from_scratch
WORKDIR /lb_from_scratch

RUN chmod +x setup.sh && \
    chmod +x get_deps.sh && \
    ./get_deps.sh

RUN git clone --recurse-submodules https://github.com/libbpf/bpftool.git
    ## install bpftool
    # cd bpftool/src  && \
    # make install  && \
    # cd ../.. 

ENTRYPOINT [ "/lb_from_scratch/setup.sh" ]