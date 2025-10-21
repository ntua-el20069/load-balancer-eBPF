# [cite_start]Use the Ubuntu 22.04 LTS (Jammy) image as the base [cite: 1, 3]
FROM ubuntu:jammy

# Optional: Set a label to confirm the OS version (from old-Dockerfile)
LABEL os.version="Ubuntu 22.04.5 LTS"
LABEL maintainer="docker-real"

# Set Python environment variables for best practices (from new-Dockerfile)
# [cite_start]Prevents Python from writing pyc files. [cite: 8]
ENV PYTHONDONTWRITEBYTECODE=1
# [cite_start]Keeps Python from buffering stdout and stderr to avoid log loss. [cite: 9]
ENV PYTHONUNBUFFERED=1

# Install required system packages: Python3, pip, and network tools
RUN apt-get update && \
    # Install Python 3 and pip
    apt-get install -y python3 python3-pip && \
    # Install network tools (iproute2, iputils-ping, etc.) from old-Dockerfile
    apt-get install -y iproute2 iputils-ping net-tools curl wget vim && \
    # Clean up the apt cache to reduce image size
    rm -rf /var/lib/apt/lists/*

# Set the working directory for the application
WORKDIR /app


# # Create the non-root user 'simple_user'
# # The user ID (1000) and group ID (1000) are common choices for non-root users.
# RUN groupadd --gid 1000 simple_user && \
#     useradd --uid 1000 --gid simple_user --shell /bin/bash --create-home simple_user

# # Add 'simple_user' to the 'sudo' group
# # This allows the user to run commands with elevated privileges (if needed).
# # Note: For security, giving sudo access in a container is often discouraged, 
# # but it aligns with your specific request.
# RUN apt update && apt install -y sudo && \
#     usermod -aG sudo simple_user && \
#     rm -rf /var/lib/apt/lists/*


# # Add NOPASSWD setting for simple_user (must be done as root)
# # This uses the 'visudo' utility (via tee) to add a line granting passwordless sudo access.
# RUN echo "simple_user ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

# Install Python dependencies. This assumes a 'requirements.txt' file exists in the build context.
# Using 'pip3' since we installed 'python3-pip' on the Ubuntu base.
RUN --mount=type=cache,target=/root/.cache/pip \
    --mount=type=bind,source=requirements.txt,target=requirements.txt \
    python3 -m pip install -r requirements.txt

# Copy the source code into the container.
COPY . .

RUN chmod +x setup.sh

# [cite_start]Switch to the non-privileged user to run the application for security[cite: 14].
# USER simple_user

# [cite_start]Expose the port that the application listens on[cite: 15].
EXPOSE 8000

# Run the FastAPI application using uvicorn (from new-Dockerfile).
ENTRYPOINT [ "/app/setup.sh" ]