#!/bin/bash

# real setup

# logging of commands, exit if any cmd fails
set -euxo pipefail

# static route
ip route add ${GENERAL_SUBNET} via ${GATEWAY_REAL_IP} dev eth0

# setup interfaces for ipip encapsulation
ip link add name ipip0 type ipip external
ip link set up dev ipip0
ip a a ${LOCAL_IP_FOR_IPIP}/32 dev ipip0

# the following interface type is not supported on WSL
# ip link add name ipip60 type ip6tnl external
# ip link set up dev ipip60

# Katran IP as loopback
ip a a ${VIP_1}/32 dev lo

# remove rp_filter
for sc in $(sysctl -a | awk '/\.rp_filter/ {print $1}'); do  echo $sc ; sysctl ${sc}=0; done

# run the application
uvicorn app:app --host 0.0.0.0 --port 8000
