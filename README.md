# eBPF Load Balancer
This project utilizes [katran](https://github.com/facebookincubator/katran) for use as a load balancer between MQTT clients and an MQTT cluster of brokers. It is currently under development. The used topology includes client, Katran LB, gateway and real-server. These services run into Docker containers that have a specific network connection as shown here.

<img src="images/topology.png" width="100%"/>

The test was done on 
```txt
Operating System: Ubuntu 22.04.5 LTS              
Kernel: Linux 6.8.0-83-generic
Architecture: x86-64
```
The test was NOT successful on WSL / Windows environments. 
Be conscious if you try to test this in a host different than Ubuntu.

## Docker setup

On Ubuntu host execute:
```bash
git clone https://github.com/nickpapakon/load-balancer-eBPF.git
cd load-balancer-eBPF/lb-n-reals/
```
Ensure Docker Desktop is up and running and then,
```bash
docker compose up --build -d
```
After docker compose command completes (it may take 20-30 minutes), 
Wait until the entrypoint scripts `setup.sh` finish (watch the logs of each container) and then ensure that all containers are up and running.

### Katran container Setup

Open 3 terminals `termA`, `termB`, `termC` executing the katran docker container shell by using 
```bash
docker exec -it katran sh
```
- On `termA` terminal, run the katran server
```bash
cd /home/simple_user/katran/_build
sudo ./build/example_grpc/katran_server_grpc -balancer_prog ./deps/bpfprog/bpf/balancer.bpf.o  -forwarding_cores=0 -hc_forwarding=false -lru_size=10000 -default_mac ${GATEWAY_KATRAN_MAC}
```
- On `termB`, use bpftool to inspect the logs of the bpf program
```bash
cat /sys/kernel/debug/tracing/trace_pipe
```
- On `termC`, run client commands to configure VIPs and reals for Katran
```bash
cd /home/simple_user/katran/example_grpc/goclient/src/katranc/main

# configure a VIP that corresponds to all reals/MQTT Brokers
# Brokers Run on 
./main -A -t ${VIP_ALL}:${MQTT_PORT}
./main -a -t ${VIP_ALL}:${MQTT_PORT} -r ${REAL_1_IP} -w 1
./main -a -t ${VIP_ALL}:${MQTT_PORT} -r ${REAL_2_IP} -w 1
./main -a -t ${VIP_ALL}:${MQTT_PORT} -r ${REAL_3_IP} -w 1

# list available services (VIP -> reals mapping)
./main -l
```

<!--
```bash
bpftool prog list
bpftool prog tracelog

sudo bpftrace -e 'tracepoint:xdp:* { @cnt[probe] = count(); }'

bpftrace -e \
 'tracepoint:xdp:xdp_bulk_tx{@redir_errno[-args->err] = count();}'
```
-->


<!--
#####  ipip0 and ipip6tnl interfaces on Katran (needed only for healthchecking)

# setup interfaces for ipip encapsulation
ip link add name ipip0 type ipip external
ip link set up dev ipip0
ip a a ${LOCAL_IP_FOR_IPIP}/32 dev ipip0

# if you want to set down and delete interface
# ip link set down dev ipip0
# ip link del  ipip0

# the following interface type is not supported on WSL
# ip link add name ipip60 type ip6tnl external
# ip link set up dev ipip60

##### Traffic control and Queueing Discipline

# attach clsact qdisc on egress interface for usage in case of health checks
tc qd add  dev eth0 clsact

# if you want to show or delete the traffic control qdisc
# tc qd show dev eth0
# tc qd del dev eth0 clsact

-->

### Client container
In a similar way, open a terminal and execute the shell of client container
```bash
docker exec -it client sh
```
Ensure the following mqtt publish and watch docker container logs to ensure that broker / real_1 gets the message
```bash
mosquitto_pub -h ${REAL_1_IP} -t motor -p ${MQTT_PORT} -m "motor temp, current, ..."
```
Then try to make the request to katran:
```bash
# by running the following you expect one of the 3 brokers will receive the message
mosquitto_pub -h ${VIP_ALL} -t motor -p ${MQTT_PORT} -m "motor temp, current, ..."
```

<!--
mosquitto_pub -h ${SCRATCH_LB_IP} -t motor -p ${MQTT_PORT} -m "motor temp, current, ..."
-->
You should see from docker container logs that one of the reals / brokers receives the message and you should be able to see the katran logs on `termB`
```txt
bpf_trace_printk: Redirecting packet to real ...
```

### Debugging tools-steps
- Run tcpdump on interfaces of `gateway` and `real` containers
- change the docker containers networking that is configured on `compose.yaml` (`macvlan` worked)
