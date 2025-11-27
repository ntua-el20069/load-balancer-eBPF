FROM ubuntu:jammy

LABEL os.version="Ubuntu 22.04.5 LTS"
LABEL maintainer="docker-client"

# Python environment variables
# Prevents Python from writing pyc files.
ENV PYTHONDONTWRITEBYTECODE=1
# Keeps Python from buffering stdout and stderr to avoid log loss.
ENV PYTHONUNBUFFERED=1

# Install utilities
RUN apt-get update && \
    apt-get install -y python3 python3-pip && \
    apt-get install -y iproute2 traceroute iputils-ping net-tools curl wget vim tcpdump \
                        software-properties-common && \
    apt-add-repository ppa:mosquitto-dev/mosquitto-ppa && \
    apt-get update && \
    apt-get -y install mosquitto && \
    apt-get -y install mosquitto-clients && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /home/client-folder

# Install Python dependencies. This assumes a 'requirements.txt' file exists in the build context.
RUN --mount=type=cache,target=/root/.cache/pip \
    --mount=type=bind,source=requirements.txt,target=requirements.txt \
    python3 -m pip install -r requirements.txt

COPY . .

RUN chmod +x setup.sh

ENTRYPOINT [ "/home/client-folder/setup.sh" ]