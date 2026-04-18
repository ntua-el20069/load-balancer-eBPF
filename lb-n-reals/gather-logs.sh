#!/bin/bash

COLOR_RED="\033[0;31m"
COLOR_GREEN="\033[0;32m"
COLOR_OFF="\033[0m"

docker compose config | grep -P "container_name\:\s(client_.*)" | awk '{print $2}' | while read -r NAME; do
    LOG_FILE="experiment_logs/$NAME.log"
    echo "--- Gather logs from container $NAME ---"
    docker container logs "$NAME" > "$LOG_FILE" 2>&1 || echo -e "${COLOR_RED} Failed to get logs for container $NAME, skipping... ${COLOR_OFF}"
done

docker compose config | grep -P "container_name\:\s(real_.*|katran)" | awk '{print $2}' | while read -r NAME; do
    LOG_FILE="experiment_logs/$NAME.log"
    echo "--- Gather logs from container $NAME ---"
    docker container logs "$NAME" > "$LOG_FILE" 2>&1 || echo -e "${COLOR_RED} Failed to get logs for container $NAME, skipping... ${COLOR_OFF}"
done
