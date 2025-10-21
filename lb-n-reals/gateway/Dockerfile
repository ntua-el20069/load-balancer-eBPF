# The 'jammy' tag officially refers to Ubuntu 22.04 LTS.
# This image is built for the standard x86-64 architecture by default.
FROM ubuntu:jammy

# Optional: Set a label to confirm the OS version
LABEL os.version="Ubuntu 22.04.5 LTS"

# Set a helpful maintainer label
LABEL maintainer="docker-gateway"

# Install network tools like iproute2 (for 'ip' command) and ping
RUN apt-get update && \
    apt-get install -y iproute2 iputils-ping net-tools && \
    rm -rf /var/lib/apt/lists/*

# The command to keep the container running indefinitely
# so you can connect to it.
CMD ["sleep", "infinity"]