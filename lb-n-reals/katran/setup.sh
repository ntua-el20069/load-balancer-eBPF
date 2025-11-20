#!/bin/bash

# katran setup

# logging of commands, exit if any cmd fails
set -euxo pipefail

# The following command fails
# touch /proc/sys/net/core/bpf_jit_enable || true
# sudo sysctl net.core.bpf_jit_enable=1
# sysctl: cannot stat /proc/sys/net/core/bpf_jit_enable: No such file or directory

# ipip0 and ip6tnl interfaces required only in case of healthchecking
# traffic control, queueing disciplines etc required only in case of healthchecking

# disable LRO and GRO on eth0
/usr/sbin/ethtool --offload eth0 lro off
# /usr/sbin/ethtool --offload eth0 gro off
# Could not change any device features
# Actual changes:
# rx-gro: on [requested off]

# static route
ip route add ${GENERAL_SUBNET} via ${GATEWAY_KATRAN_IP} dev eth0

## install bpftool
cd bpftool/src  && \
make install  && \
cd ../..

# keep container running indefinitely
sleep infinity

