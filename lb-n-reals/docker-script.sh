#!/usr/bin/env bash

set -xeou pipefail

echo "Build base_images"
date '+%d/%m/%Y_%H:%M:%S'

pushd .

cd base_images
chmod +x ./build_images.sh
./build_images.sh

popd

echo "Docker Compose Up Containers"
date '+%d/%m/%Y_%H:%M:%S'

docker compose up --build -d

echo "Finished"
date '+%d/%m/%Y_%H:%M:%S'

