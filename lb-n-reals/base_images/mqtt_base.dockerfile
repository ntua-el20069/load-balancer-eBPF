FROM ubuntu:jammy AS mqtt_base_image
LABEL os.version="Ubuntu 22.04.5 LTS"

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