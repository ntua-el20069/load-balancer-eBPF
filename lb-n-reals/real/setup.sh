#!/bin/bash

# real setup

# logging of commands, exit if any cmd fails
set -euxo pipefail

# static route
ip route add 10.1.0.0/16 via 10.1.3.101 dev eth0

# setup interfaces for ipip encapsulation
ip link add name ipip0 type ipip external
ip link set up dev ipip0

# the following interface type is not supported on WSL
# ip link add name ipip60 type ip6tnl external
# ip link set up dev ipip60

# set an ip for the ipip0 to operate
ip a a 127.0.0.42/32 dev ipip0

# Katran IP as loopback: TODO change to sth configurable
ip a a 10.1.50.50/32 dev lo

# remove rp_filter
for sc in $(sysctl -a | awk '/\.rp_filter/ {print $1}'); do  echo $sc ; sysctl ${sc}=0; done

# run the application
uvicorn app:app --host 0.0.0.0 --port 8000
