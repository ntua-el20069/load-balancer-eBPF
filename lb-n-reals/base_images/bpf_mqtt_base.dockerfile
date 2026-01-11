FROM ubuntu:jammy AS bpf_mqtt_base_image
LABEL os.version="Ubuntu 22.04.5 LTS"

RUN apt-get update && \
    apt-get install -y git clang llvm libelf-dev libpcap-dev  libssl-dev build-essential make libc6-dev-i386 m4 && \ 
    apt-get install -y linux-tools-common && \
    apt-get install -y iproute2 net-tools traceroute tcpdump && \
    apt-get install -y wget vim curl iputils-ping ethtool sudo && \
    rm -rf /var/lib/apt/lists/*

# Install utilities
RUN apt-get update && \
    apt-get install -y python3 python3-pip git && \
    apt-get install -y iproute2 traceroute iputils-ping net-tools curl wget vim tcpdump \
                        software-properties-common && \
    apt-add-repository ppa:mosquitto-dev/mosquitto-ppa && \
    apt-get update && \
    apt-get -y install mosquitto && \
    apt-get -y install mosquitto-clients && \
    rm -rf /var/lib/apt/lists/*