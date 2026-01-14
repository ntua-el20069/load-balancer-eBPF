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
Ensure Docker Desktop is up and running and then, use a script that 
- builds the base images (e.g. `bpf_mqtt_base` is an ubuntu img containing most utilities needed for eBPF programs and MQTT messaging)
- does docker compose up for the containers
```bash
chmod +x ./docker-script.sh
./docker-script.sh
```
After this script completes (it may take 20-30 minutes), 
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

You should see from docker container logs that one of the reals / brokers receives the message and you should be able to see the katran logs on `termB`
```txt
bpf_trace_printk: Redirecting packet to real ...
```
