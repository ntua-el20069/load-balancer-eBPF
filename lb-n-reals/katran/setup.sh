#!/bin/bash

# katran setup

# cannot do that 
# sysctl: cannot stat /proc/sys/net/core/bpf_jit_enable: No such file or directory
# sudo sysctl net.core.bpf_jit_enable=1

sudo ip link add name ipip0 type ipip external
sudo ip link add name ipip60 type ip6tnl external
sudo ip link set up dev ipip0
sudo ip link set up dev ipip60
sudo tc qd add  dev eth0 clsact
sudo apt install ethtool
sudo /usr/sbin/ethtool --offload eth0 lro off
sudo /usr/sbin/ethtool --offload eth0 gro off

## add static routes
sudo ip route add 10.1.0.0/16 via 10.1.2.101 dev eth0

sleep infinity

