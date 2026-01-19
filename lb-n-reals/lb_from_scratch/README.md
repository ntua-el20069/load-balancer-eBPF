
# lb-from-scratch

Credits for this lb setup and code [lizrice/lb-from-scratch](https://github.com/lizrice/lb-from-scratch/tree/main)

```bash
# for Documentation
cat ./xdp_lb_kern.bpf.c | grep -P "//\s+\[.*\]:"

```


## Run in shared mode

```bash
cd katran/
export KATRAN_INTERFACE="eth0"
# mounts bpffs
# executes the program (katran/lib/xdproot.cpp) that 
#   loads the xdp_root program (katran/lib/bpf/xdp_root.c), 
#   and pins to the /sys/fs/bpf/jmp_eth0 path
chmod +x ./install_xdproot.sh
./install_xdproot.sh

# verify xdp_root program was successfully loaded
bpftool prog show name xdp_root
```


```bash
# Inspect the root_array
export ROOT_MAP_ID=$(bpftool map list | grep root_array | awk -F':' '{ print $1 }')
bpftool map show name root_array
bpftool map dump name root_array

# verify mount bpffs
mount | grep bpf
```

```bash
# Start katran_server_grpc with additional flags
... -map_path /sys/fs/bpf/jmp_eth0 -prog_pos=1
```


```bash
make xdp_pass_kern.o

bpftool prog load xdp_pass_kern.o /sys/fs/bpf/xdp_pass_kern

bpftool prog list
bpftool prog show name xdp_prog_simple


# bpftool net attach xdpgeneric pinned /sys/fs/bpf/xdp_pass_kern  dev eth0
# libbpf: Kernel error message: XDP program already attached
# Error: interface xdpgeneric attach failed: Device or resource busy


cd /lb_from_scratch/xdp-tutorial/basic00-update-prog-array
export ROOT_MAP_ID=$(bpftool map list | grep root_array | awk -F':' '{ print $1 }')
export MY_XDP_PROG_ID=$(bpftool prog list | grep xdp_dummy_prog | awk -F':' '{ print $1 }')
./user_root_array ${ROOT_MAP_ID} 0 ${MY_XDP_PROG_ID}

export LB_XDP_PROG_ID=$(bpftool prog list | grep xdp_load_balancer | awk -F':' '{ print $1 }')
./user_root_array ${ROOT_MAP_ID} 1 ${LB_XDP_PROG_ID}


export ROOT_MAP_ID=$(bpftool map list | grep root_array | awk -F':' '{ print $1 }')
bpftool map show name root_array
bpftool map dump name root_array
```



```bash
clang -g -O2 -target bpf -c xdp_lb_kern.bpf.c -o xdp_lb_kern.bpf.o
sudo bpftool gen skeleton xdp_lb_kern.bpf.o > /lb_from_scratch/xdp-tutorial/basic00-loader/tail_call.skel.h
cd /lb_from_scratch/xdp-tutorial/basic00-loader
clang -o loader loader.c -lbpf

```

