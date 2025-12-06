FROM ubuntu:jammy
LABEL os.version="Ubuntu 22.04.5 LTS"
LABEL maintainer="docker-lb-from-scratch"

RUN apt-get update && \
    apt-get install -y git clang llvm libelf-dev libpcap-dev  libssl-dev build-essential make && \ 
    apt-get install -y linux-tools-common && \
    apt-get install -y iproute2 net-tools traceroute && \
    apt-get install -y wget vim curl iputils-ping ethtool sudo && \
    rm -rf /var/lib/apt/lists/*

COPY ./get_deps.sh /lb_from_scratch/get_deps.sh
WORKDIR /lb_from_scratch

RUN chmod +x get_deps.sh && \
    ./get_deps.sh

# until here, if we do multiple docker builds, 
# the deps will be cached (Docker Build Cache)

COPY . /lb_from_scratch/

RUN chmod +x setup.sh

ENTRYPOINT [ "/lb_from_scratch/setup.sh" ]