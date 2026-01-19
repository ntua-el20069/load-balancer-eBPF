#!/usr/bin/env bash
# Example script to start and run xdproot program
set -xeo pipefail

BPF_DIR="/sys/fs/bpf/xdp"

if [ -z "${LB_INTERFACE}" ]
then
    LB_INTERFACE=eth0
fi

out=$(mount | grep bpffs) || true
if [ -z "$out" ]; then
    sudo mount -t bpf bpffs /sys/fs/bpf/
fi


rm -rf $BPF_DIR
mkdir -p $BPF_DIR

bpftool prog loadall xdp_lb_kern.bpf.o $BPF_DIR type xdp

bpftool net attach xdpgeneric pinned $BPF_DIR/xdp_root dev ${LB_INTERFACE} 

cd /lb_from_scratch/xdp-tutorial/basic00-update-prog-array

export ROOT_MAP_ID=$(bpftool map list | grep root_array | awk -F':' '{ print $1 }')

export MY_XDP_PROG_ID=$(bpftool prog list | grep xdp_dummy_prog | awk -F':' '{ print $1 }')
./user_root_array ${ROOT_MAP_ID} 0 ${MY_XDP_PROG_ID}

export LB_XDP_PROG_ID=$(bpftool prog list | grep xdp_load_balancer | awk -F':' '{ print $1 }')
./user_root_array ${ROOT_MAP_ID} 1 ${LB_XDP_PROG_ID}





