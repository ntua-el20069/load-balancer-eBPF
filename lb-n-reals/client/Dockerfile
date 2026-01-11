FROM bpf_mqtt_base

LABEL os.version="Ubuntu 22.04.5 LTS"
LABEL maintainer="docker-client"

# Python environment variables
# Prevents Python from writing pyc files.
ENV PYTHONDONTWRITEBYTECODE=1
# Keeps Python from buffering stdout and stderr to avoid log loss.
ENV PYTHONUNBUFFERED=1

WORKDIR /home/client-folder

COPY ./get_deps.sh /home/client-folder/get_deps.sh

RUN chmod +x get_deps.sh && \
    ./get_deps.sh

# Install Python dependencies. This assumes a 'requirements.txt' file exists in the build context.
RUN --mount=type=cache,target=/root/.cache/pip \
    --mount=type=bind,source=requirements.txt,target=requirements.txt \
    python3 -m pip install -r requirements.txt

COPY . .

RUN chmod +x setup.sh

ENTRYPOINT [ "/home/client-folder/setup.sh" ]