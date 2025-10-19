# The 'jammy' tag officially refers to Ubuntu 22.04 LTS.
# This image is built for the standard x86-64 architecture by default.
FROM ubuntu:jammy

# Optional: Set a label to confirm the OS version
LABEL os.version="Ubuntu 22.04.5 LTS"

# Install necessary network utilities (iproute2 is usually pre-installed but good practice)
# We also install 'net-tools' for commands like ifconfig, though 'ip' is preferred.
RUN apt-get update && \
    apt-get install -y iproute2 net-tools && \
    rm -rf /var/lib/apt/lists/*

# The command that will run when the container starts.
# It simply keeps the container running so you can connect and configure it.
# This prevents the container from immediately exiting.
CMD ["sleep", "infinity"]

# Optional: Add a script to set up the tunnel automatically
# COPY setup_tunnel.sh /usr/local/bin/
# CMD ["/usr/local/bin/setup_tunnel.sh"]