#!/bin/bash

# gateway setup

# logging of commands, exit if any cmd fails
set -euxo pipefail

# static route for Katran VIPs
ip route add 10.1.50.0/24 via 10.1.2.102 dev eth1

# keep container running indefinitely
sleep infinity