FROM ubuntu:jammy

LABEL os.version="Ubuntu 22.04.5 LTS"
LABEL maintainer="docker-gateway"

# Install utilities
RUN apt-get update && \
    apt-get install -y iproute2 traceroute iputils-ping net-tools && \
    apt install -y tcpdump && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /home/gateway-folder

COPY . .

RUN chmod +x setup.sh

ENTRYPOINT [ "/home/gateway-folder/setup.sh" ]