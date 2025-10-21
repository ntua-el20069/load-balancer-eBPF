#!/bin/bash

# katran setup

sudo sysctl net.core.bpf_jit_enable=1

sudo ip link add name ipip0 type ipip external  && \
sudo ip link add name ipip60 type ip6tnl external && \
sudo ip link set up dev ipip0 && \
sudo ip link set up dev ipip60 && \
sudo tc qd add  dev eth0 clsact && \
sudo apt install ethtool && \
sudo /usr/sbin/ethtool --offload eth0 lro off && \
sudo /usr/sbin/ethtool --offload eth0 gro off

