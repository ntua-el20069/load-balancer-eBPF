FROM mqtt_base

LABEL os.version="Ubuntu 22.04.5 LTS"
LABEL maintainer="docker-real"

# Python environment variables
# Prevents Python from writing pyc files.
ENV PYTHONDONTWRITEBYTECODE=1
# Keeps Python from buffering stdout and stderr to avoid log loss.
ENV PYTHONUNBUFFERED=1

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