# The 'jammy' tag officially refers to Ubuntu 22.04 LTS.
# This image is built for the standard x86-64 architecture by default.
FROM ubuntu:jammy

# Optional: Set a label to confirm the OS version
LABEL os.version="Ubuntu 22.04.5 LTS"

# Set a helpful maintainer label
LABEL maintainer="docker-client"


# Example: Run a command to create a directory in the user's home and set permissions
RUN mkdir -p /home/client-folder && \
    touch /home/client-folder/testfile.txt

# Install network tools like iproute2 (for 'ip' command) and ping
RUN sudo apt-get update && \
    sudo apt-get install -y iproute2 net-tools curl wget vim && \
    sudo rm -rf /var/lib/apt/lists/*

# The command to keep the container running indefinitely
# so you can connect to it.
CMD ["sleep", "infinity"]