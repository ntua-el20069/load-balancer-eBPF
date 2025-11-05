#!/bin/bash

# katran setup

# logging of commands, exit if any cmd fails
set -euxo pipefail

# The following command fails
# sudo sysctl net.core.bpf_jit_enable=1
# sysctl: cannot stat /proc/sys/net/core/bpf_jit_enable: No such file or directory

# setup interfaces for ipip encapsulation
ip link add name ipip0 type ipip external
ip link set up dev ipip0
ip a a ${LOCAL_IP_FOR_IPIP}/32 dev ipip0

# the following interface type is not supported on WSL
# ip link add name ipip60 type ip6tnl external
# ip link set up dev ipip60

# attach clsact qdisc on egress interface for usage in case of health checks
tc qd add  dev eth0 clsact

# disable LRO and GRO on eth0
apt install ethtool
/usr/sbin/ethtool --offload eth0 lro off
/usr/sbin/ethtool --offload eth0 gro off

# static route
ip route add ${GENERAL_SUBNET} via ${GATEWAY_KATRAN_IP} dev eth0

## install required libraries for libbpf and bpftool
apt-get update && \
apt-get install -y apt-transport-https ca-certificates curl clang llvm jq && \
apt-get install -y libelf-dev libpcap-dev libbfd-dev binutils-dev build-essential make  && \
apt-get install -y bpfcc-tools && \
apt-get install -y python3-pip && \
rm -rf /var/lib/apt/lists/* 

## install libbpf, bpftool (need for root access)
git clone --recurse-submodules https://github.com/lizrice/learning-ebpf && \
	cd learning-ebpf/libbpf/src && \
	make install && \
	cd ../../..
git clone --recurse-submodules https://github.com/libbpf/bpftool.git && \
	cd bpftool/src  && \
	make install  && \
	cd ../..

# keep container running indefinitely
sleep infinity

