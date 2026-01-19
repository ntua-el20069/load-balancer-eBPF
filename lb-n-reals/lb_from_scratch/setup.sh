#!/bin/bash

# lb_from_scratch setup

# logging of commands, exit if any cmd fails
set -euxo pipefail

# static route
ip route add ${GENERAL_SUBNET} via ${GATEWAY_SCRATCH_LB_IP} dev eth0

# make the project (and load the eBPF/XDP program)

# TODO make it with makefile
# make xdp_lb_kern.bpf.o

# cd /lb_from_scratch/xdp-tutorial && make

# cd /lb_from_scratch/xdp-tutorial/basic00-update-prog-array
# export ROOT_MAP_ID=$(bpftool map list | grep root_array | awk -F':' '{ print $1 }')
# export MY_XDP_PROG_ID=$(bpftool prog list | grep xdp_dummy_prog | awk -F':' '{ print $1 }')
# ./user_root_array ${ROOT_MAP_ID} 0 ${MY_XDP_PROG_ID}

# export LB_XDP_PROG_ID=$(bpftool prog list | grep xdp_load_balancer | awk -F':' '{ print $1 }')
# ./user_root_array ${ROOT_MAP_ID} 1 ${LB_XDP_PROG_ID}


cd /lb_from_scratch/xdp-tutorial/basic00-loader
./loader eth0

# keep container running indefinitely
sleep infinity