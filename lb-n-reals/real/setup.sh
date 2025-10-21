#!/bin/bash

# real setup

ip route add 10.1.0.0/16 via 10.1.3.101 dev eth0

ip link add name ipip0 type ipip external
ip link add name ipip60 type ip6tnl external
ip link set up dev ipip0
ip link set up dev ipip60

ip a a 127.0.0.42/32 dev ipip0

# Katran IP as loopback: TODO change to sth configurable
ip a a 10.1.2.102/32 dev lo

for sc in $(sysctl -a | awk '/\.rp_filter/ {print $1}'); do  echo $sc ; sysctl ${sc}=0; done

uvicorn app:app --host 0.0.0.0 --port 8000