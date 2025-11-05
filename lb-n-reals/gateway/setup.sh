#!/bin/bash

# gateway setup

# logging of commands, exit if any cmd fails
set -euxo pipefail

# static route for Katran VIPs
ip route add ${VIP_SUBNET} via ${KATRAN_IP} dev eth1

# keep container running indefinitely
sleep infinity