#!/bin/bash

# katran setup

# logging of commands, exit if any cmd fails
set -euxo pipefail

# cannot do that 
# sysctl: cannot stat /proc/sys/net/core/bpf_jit_enable: No such file or directory
# sudo sysctl net.core.bpf_jit_enable=1

ip link add name ipip0 type ipip external
ip link set up dev ipip0
ip a a 127.0.0.42/32 dev ipip0

# the following interface type is not supported on WSL
# ip link add name ipip60 type ip6tnl external
# ip link set up dev ipip60

tc qd add  dev eth0 clsact
apt install ethtool
/usr/sbin/ethtool --offload eth0 lro off
/usr/sbin/ethtool --offload eth0 gro off

## static routes
ip route add 10.1.0.0/16 via 10.1.2.101 dev eth0

## install required libraries for libbpf and bpftool
apt-get update && \
apt-get install -y apt-transport-https ca-certificates curl clang llvm jq && \
apt-get install -y libelf-dev libpcap-dev libbfd-dev binutils-dev build-essential make  && \
apt-get install -y bpfcc-tools && \
apt-get install -y python3-pip && \
rm -rf /var/lib/apt/lists/* 

## install bpftool (need for root access)
git clone --recurse-submodules https://github.com/lizrice/learning-ebpf && \
	cd learning-ebpf/libbpf/src && \
	make install && \
	cd ../../..
git clone --recurse-submodules https://github.com/libbpf/bpftool.git && \
	cd bpftool/src  && \
	make install  && \
	cd ../..


sleep infinity

