#!/bin/bash

# lb_from_scratch setup

# logging of commands, exit if any cmd fails
set -euxo pipefail

# static route
ip route add ${GENERAL_SUBNET} via ${GATEWAY_SCRATCH_LB_IP} dev eth0

# keep container running indefinitely
sleep infinity