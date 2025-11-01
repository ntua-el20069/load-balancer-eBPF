#!/bin/bash

# client setup

# logging of commands, exit if any cmd fails
set -euxo pipefail

# static route
ip route add 10.1.0.0/16 via 10.1.1.101 dev eth0

# sleep so that container does not exit
sleep infinity
