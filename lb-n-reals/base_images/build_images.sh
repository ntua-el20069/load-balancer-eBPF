#!/usr/bin/env bash

set -xeo pipefail

# Useful constants
COLOR_RED="\033[0;31m"
COLOR_GREEN="\033[0;32m"
COLOR_OFF="\033[0m"

target_images=$(ls -1 | grep dockerfile | awk -F'.' '{print $1}')

for f in *.dockerfile; do
    target_img="${f%.dockerfile}"
    echo -e "${COLOR_GREEN} Building image: ${target_img}:latest ${COLOR_OFF}"
    docker build -t "${target_img}:latest" -f "./$f" .
done