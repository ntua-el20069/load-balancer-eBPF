# eBPF Load Balancer
This project utilizes [katran](https://github.com/facebookincubator/katran) for use as a load balancer between MQTT clients and an MQTT cluster of brokers. It is currently under development. The used topology includes Katran LB, gateway, some clients and some reals/brokers. These services run into Docker containers that have a specific network connection as shown here.

<img src="images/new_topology.png" style="display: block; margin: 0 auto; width: 70%; height: 70%;" />

The test was done on 
```txt
Operating System: Ubuntu 22.04.5 LTS              
Kernel: Linux 6.8.0-83-generic
Architecture: x86-64
```
The test was NOT successful on WSL / Windows environments. 
Be conscious if you try to test this in a host different than Ubuntu.

*Any attempt to run this compose project should be done on OSes running Linux kernel (check Katran Requirements for detailed info) due to the eBPF dependency.* 

## Docker setup

Docker Resources limits used:
```txt
CPU limit: 8
Memory Limit: 6 GB
Swap: 0 Bytes
Disk usage limit: 80 GB
```

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
After this script completes (it may take 20-30 minutes), ensure all containers were built and run.

## Test

Experiments can be done using `lb-n-reals/experiment.sh` (explore environmental variables on `.env`)
Tests that contain multiple experiments, comparing the following  setups, are done using `lb-n-reals/test-*.sh` scripts:
- single broker
- Load Balancing using the eBPF Load Balancer
- Load Balancing using Shared Subscriptions

Load Balancer evolution and tests results have been kept inside `lb-n-reals/evolution/`. 
For a quick understanding, read the introduction and the last test done. 

You can switch to the appropriate branch to inspect the code and configuration at the time of each experiment. **Caution**: the operation of the setup depends on the `mqtt_LB` branch of the [forked Katran repo](https://github.com/nickpapakon/katran/tree/mqtt_LB) .


## Monitoring

- cAdvisor, Prometheus, Grafana containers can also run to gather and visualize the resources usage data for each container
- You can inspect measurements from completed experiments using the guide in `past_monitor`


## Constraints

- All clients send messages using `MQTT_VIP` as destination IP.
- Only unencrypted [MQTT](https://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html) over TCP/IPv4 is supported for communication
- Each client sends MQTT publish messages with only 1 topic. For example, a client cannot send messages to both temperature and humidity topics, else massive packet loss will be observed. Noting that, two clients can publish to the same topic, without any consequences.
- The first MQTT publish message of each client is expected to be lost whenever. If client changes IP, again the first message of this client will be lost.
- Clients should inspect the status of the connection and try to re-establish MQTT/TCP connections.
- Clients should have distinct IP addresses. So, clients cannot be behind the same PAT Gateway.
- MQTT topics sent in the client messages should not exceed 256 characters length.
- The number of clients supported by the system is restricted by the amount of [locked memory](https://dl.acm.org/doi/10.1145/3371038) that can be allocated for eBPF Maps. Constant `MAX_CLIENTS` inside  `lb-n-reals/katran/bpf/mqtt_topic_based_fwd.h` can be modified to address the need for more clients, but a significant change the overcomes the locaked memory limit will cause a failure during the loading of the eBPF program. Keep real-time clients below the `MAX_CLIENTS` value, otherwise massive packet-loss may be observed as a result of eBPF Map's `mqtt_client_ip_to_topic` misses.
- Constants `MAX_VIPS` and `MAX_TOPIC_MAPPINGS` restrict the maximum number of VIPs and mappings between VIPs and topics. Can be modified but as referred, locked memory wall should be kept on mind.
- [Katran requirements](https://github.com/facebookincubator/katran#environment-requirements-for-katran-to-run) should also be satisfied.

**Deploying the proposed Load Balancer in a real-world production environment requires further testing**, as the experimental network traffic load used in this study does not fully represent a high-intensity use case. In addition, issues concerning packet losses are reported. The most significant one is that, at the time of the development of this Load Balancer, [TCP segmentation](https://datatracker.ietf.org/doc/html/rfc9293#name-segmentation) was ignored and this may cause massive packet loss in some rare circumstances.

