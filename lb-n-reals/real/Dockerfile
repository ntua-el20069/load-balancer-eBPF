FROM ubuntu:jammy

LABEL os.version="Ubuntu 22.04.5 LTS"
LABEL maintainer="docker-real"

# Python environment variables
# Prevents Python from writing pyc files.
ENV PYTHONDONTWRITEBYTECODE=1
# Keeps Python from buffering stdout and stderr to avoid log loss.
ENV PYTHONUNBUFFERED=1

# Install required system packages
RUN apt-get update && \
    apt-get install -y python3 python3-pip && \
    apt-get install -y iproute2 traceroute iputils-ping net-tools curl wget vim tcpdump \
                        software-properties-common && \
    apt-add-repository ppa:mosquitto-dev/mosquitto-ppa && \
    apt-get update && \
    apt-get -y install mosquitto && \
    apt-get -y install mosquitto-clients && \
    # Clean up the apt cache to reduce image size
    rm -rf /var/lib/apt/lists/*

# Set the working directory for the application
WORKDIR /home/real-folder


# Install Python dependencies. This assumes a 'requirements.txt' file exists in the build context.
RUN --mount=type=cache,target=/root/.cache/pip \
    --mount=type=bind,source=requirements.txt,target=requirements.txt \
    python3 -m pip install -r requirements.txt

# Copy the source code into the container.
COPY . .

RUN chmod +x setup.sh

# Expose the port that the application listens on
EXPOSE 8000

# setup network interfaces and start the application
ENTRYPOINT [ "/home/real-folder/setup.sh" ]