# The 'jammy' tag officially refers to Ubuntu 22.04 LTS.
# This image is built for the standard x86-64 architecture by default.
FROM ubuntu:jammy

# Optional: Set a label to confirm the OS version
LABEL os.version="Ubuntu 22.04.5 LTS"

# Set a helpful maintainer label
LABEL maintainer="docker-client"

# 1. Create the non-root user 'simple_user'
# The user ID (1000) and group ID (1000) are common choices for non-root users.
RUN groupadd --gid 1000 simple_user && \
    useradd --uid 1000 --gid simple_user --shell /bin/bash --create-home simple_user

# 2. Add 'simple_user' to the 'sudo' group
# This allows the user to run commands with elevated privileges (if needed).
# Note: For security, giving sudo access in a container is often discouraged, 
# but it aligns with your specific request.
RUN apt update && apt install -y sudo && \
    usermod -aG sudo simple_user && \
    rm -rf /var/lib/apt/lists/*


# 1. Add NOPASSWD setting for simple_user (must be done as root)
# This uses the 'visudo' utility (via tee) to add a line granting passwordless sudo access.
RUN echo "simple_user ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

# 3. Perform specific RUN commands as 'simple_user'
# Use the 'USER' instruction before any command you want to run as this user.
USER simple_user

# Example: Run a command to create a directory in the user's home and set permissions
RUN mkdir -p /home/simple_user/data && \
    touch /home/simple_user/data/testfile.txt

# 4. (Optional but recommended) Set the default working directory
WORKDIR /home/simple_user

# 5. Set the default user for any future RUN commands and when the container starts
# This ensures that when you run `docker run -it <image> sh`, you connect as simple_user.
USER simple_user



# Install network tools like iproute2 (for 'ip' command) and ping
RUN sudo apt-get update && \
    sudo apt-get install -y iproute2 net-tools && \
    sudo rm -rf /var/lib/apt/lists/*

# The command to keep the container running indefinitely
# so you can connect to it.
CMD ["sleep", "infinity"]