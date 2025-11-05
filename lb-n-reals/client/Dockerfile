FROM ubuntu:jammy

LABEL os.version="Ubuntu 22.04.5 LTS"
LABEL maintainer="docker-client"

# Install utilities
RUN apt-get update && \
    apt-get install -y iproute2 traceroute iputils-ping net-tools curl wget vim && \
    apt install -y tcpdump && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /home/client-folder

COPY . .

RUN chmod +x setup.sh

ENTRYPOINT [ "/home/client-folder/setup.sh" ]