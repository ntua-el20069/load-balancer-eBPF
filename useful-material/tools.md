# Useful tools



### Debugging tools
- Run tcpdump on interfaces of `gateway`  container
- change the docker containers networking that is configured on `compose.yaml` (`macvlan` worked)
- mqtt message to `lb_from_scratch`
```bash
mosquitto_pub -h ${SCRATCH_LB_IP}  -p ${MQTT_PORT} -m "motor temp, current, ..." -t sensors/
```
- use of `bpftool` (useful commands)
```bash
export PROG_ID=$(bpftool prog list | grep xdp_load | awk -F':' '{ print $1 }')
export MAP_ID=$(bpftool map list | grep mqtt_topic | awk -F':' '{ print $1 }')
bpftool prog list
bpftool prog show name xdp_load_balancer
bpftool prog show id $PROG_ID
bpftool prog show 
bpftool prog dump xlated name xdp_load_balancer
bpftool map list
bpftool map show id $MAP_ID
bpftool map dump id $MAP_ID
bpftool map lookup id $MAP_ID key 100 0 0 0 0 0 0 0
bpftool prog tracelog
# Note that you have to specify each of the 8 bytes of the key individually, starting
# with the least significant
```

- Update eBPF map using a userspace program
```bash
./user_bpfmap 59 sensors/ 10.1.50.32
./user_bpfmap $MAP_ID motors99 10.1.50.32
```

- View the status of TCP connections
```bash
netstat -tn
```

- Observability with eBPF programs (fentry) on client

```bash
cd src/2-kprobe-unlink
../../additional_packages/ecc ./tcp_seq.bpf.c
../../additional_packages/ecli package.json

bpftool btf dump file /sys/kernel/btf/vmlinux format c > include/vmlinux.h
make
bpftool btf dump file tcp_seq.bpf.o
sudo bpftool prog loadall tcp_seq.bpf.o /sys/fs/bpf/tcp_seq_tracing

cat /sys/kernel/debug/tracing/trace_pipe

sudo bpftool prog loadall tcp_seq.bpf.o /sys/fs/bpf/tcp_trace autoattach

sudo bpftool link show

sudo bpftool btf dump file /sys/kernel/btf/vmlinux | grep tcp_validate_incoming

cp -r ./lb_from_scratch/include/ ./client/
```


- Use of bpftrace 

```bash
bpftool prog list
bpftool prog tracelog

sudo bpftrace -e 'tracepoint:xdp:* { @cnt[probe] = count(); }'

bpftrace -e \
 'tracepoint:xdp:xdp_bulk_tx{@redir_errno[-args->err] = count();}'
```

- generate `vmlinux.h` file with `bpftool`
```bash
# generate the vmlinux.h header file
mkdir -p include
bpftool btf dump file /sys/kernel/btf/vmlinux format c > include/vmlinux.h
```