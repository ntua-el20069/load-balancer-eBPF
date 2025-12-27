FROM ubuntu:jammy
LABEL os.version="Ubuntu 22.04.5 LTS"
LABEL maintainer="docker-lb-from-scratch"

RUN apt-get update && \
    apt-get install -y git clang llvm libelf-dev libpcap-dev  libssl-dev build-essential make libc6-dev-i386 m4 && \ 
    apt-get install -y linux-tools-common && \
    apt-get install -y iproute2 net-tools traceroute && \
    apt-get install -y wget vim curl iputils-ping ethtool sudo && \
    rm -rf /var/lib/apt/lists/*

COPY ./get_deps.sh /lb_from_scratch/get_deps.sh
WORKDIR /lb_from_scratch

RUN chmod +x get_deps.sh && \
    ./get_deps.sh

# clone the xdp-tutorial repo inorder to get some common files for userspace eBPF programs
RUN git clone --recurse-submodules https://github.com/xdp-project/xdp-tutorial.git

RUN cd xdp-tutorial && \
    chmod +x ./configure && \
    ./configure && \
    rm -rf basic* packet* tracing*

# until here, if we do multiple docker builds, 
# the deps will be cached (Docker Build Cache)
# whatever file changes below this line will invalidate the cache

COPY ./userspace /lb_from_scratch/xdp-tutorial/basic00-update-map

RUN cd /lb_from_scratch/xdp-tutorial && \
    make

COPY . /lb_from_scratch/

RUN chmod +x setup.sh

ENTRYPOINT [ "/lb_from_scratch/setup.sh" ]