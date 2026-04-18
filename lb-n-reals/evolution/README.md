# MQTT Topic Based Load Balancing

Imagine of an MQTT cluster containing brokers and clients were these rules are satisfied:
- Only MQTT publish messages are delivered
- Each client sends messages of only one topic (e.g. Many clients can publish to topic `temperature`, but there cannot be any client that publishes to both `temperature` and `humidity` topics)

We want to load-balance the MQTT publish messages to brokers (e.g. A,B,C,D,E,F,G,H) based on their topic.

| topic    | Broker  |
| -------- | ------- |
| apples   | A/B/C   |
| oranges  | D/E     |
| lemons   | F/G/H   |

For example, an MQTT publish message that has topic `apples` should be forwarded to one broker between broker A, broker B, broker C.

Grouping of these IPs can be done by utilizing an existing project `Katran`, that proposes VIPs. So VIP_I can stand for A/B/C, VIP_II for D/E and so on. Load balancing between brokers of a certain VIP is also done by Katran project, so here we will emphasize on deciding the VIP that corresponds to a certain MQTT communication (including TCP 3WHS, MQTT CONNECT/CONNACK, MQTT PUBLISH and MQTT DISCONNECT REQ). Katran also handles the forwarding of a flow of messages (identifies that from the 5-tuple of proto, ports, ip-addrs) to be done to the same real/broker.

As each client sends messages only of one topic, we can predict the topic that the client wants to send based on the client IP. In fact, Load Balancer can maintain a map that correlates client IPs (keys) with the last topic that was sent by them (value).
However, whenever client sends the first packet or whenever client changes IP, there is no correct corresponding match in this Map and the TCP SYN, ACK and MQTT CONNECT packets may be delivered to a non-responsible VIP (these packets do not contain the MQTT topic - but should be forwarded to the correct VIP). When the MQTT publish packet arrives Load Balancer understands that the previous segments that initiated the connection were not properly forwarded (What to do with this PUBLISH packet is still a **TODO**). Load Balancer updates the Map to correctly identify the last topic published by this client IP (so the next packets by this IP will bw correctly forwarded to the responsible VIP). 

## Test 1

### Limitations

In order to present a simple first version some **constraints** were adopted.
- Fixed Topic Length (`FIXED_TOPIC_LENGTH 8`): eBPF Verifier complains when `memcpy` has a length parameter value that is not a compile-time constant
TODO in next test to remove this limitation (and ensure that variable topic length can be used - MQTT topic length can vary to up to 2^16 - 1 )
- Only one client used: This is a limitation of the scratch LB.
However this should not be a problem, as we tested the circumstance that this client changes IP and continues publishing messages
- IPv6 packets are not handled: If we need to run MQTT and TCP over IPv6, modifications are needed (however eBPF Maps and structs already contain the field for the ipv6 address in a union with ipv4 address)
- Suppose that we use the classic MQTT over TCP
- Whenever the client IP changes the first MQTT PUBLISH message is lost (see description below)

### MQTT-Topic based forwarding logic 

In this test:
We want to load balance MQTT PUBLISH messages based on topics
| topic        | Broker  |
| ------------ | ------- |
| sensors/     | real_1  |
| other topics | real_2  |

### scratch LB eBPF Maps

The `lb_from_scratch` container is a simple Load Balancer that supports only one client (see limitations above) and runs an eBPF program that manipulates the BPF maps:
- eBPF Map `mqtt_topic_to_vip`: topic `sensors/` key corresponds to VIP that maps to broker `real_1`
- eBPF Map `client_ips`: client IP      (This map is used because scratch LB is silly - it does not learn client IPs on its own)
                                      (In case of Katran we won't need this Map because Reals send the packets directly to the clients, so Katran receives Packets only from the clients and won't send anything to them)
- eBPF Map `mqtt_client_ip_to_topic` is only modified by the kernel eBPF program. Whenever a MQTT PUBLISH packet is received, this map is updated (key `client src IP` -> value `topic`) so  we maintain the last topic that was published by each client IP and the next time a packet arrives the LB can do a correct prediction of the topic based on src IP and forward to the responsible IP.


### Test Procedure

- `Phase 1`: Client has initially the IP `10.1.1.107`. Sends 3 MQTT PUBLISH messages to the MQTT VIP
The first one **fails** to be delivered (client IP does not exist yet as key in the eBPF map that predicts topics)
The next two are successfully delivered to the correct MQTT real server
Let's say that client IP changes to 10.1.1.102
- `Phase 2`: After the change of the client IP to `10.1.1.102`. Client sends 3 MQTT PUBLISH messages as before.
Again the first PUBLISH message **fails** to be delivered (new client IP not in BPF map)
The next two are successfully delivered to the correct MQTT real server

Logs and a capture are stored in `test-1/`.

### Commands

- set `CLIENT_IP=10.1.1.107` in the `.env` file

- Docker compose of `client`, `lb_from_scratch`, `real_1`, `real_2`, `gateway` containers 

- 4 terminals open were you will connect to containers `docker exec -it <container_name> sh`: `client_SH`, `lb_SH_1`, `lb_SH_2`, `gateway_SH`

- In `lb_SH_1`, check the trace pipe (where `bpf_printk` commands write their output)
```bash
bpftool prog tracelog
```

- Gateway captures packets on the interface that looks to the `lb_from_scratch`. So, in `gateway_SH`
```bash
tcpdump -n -i eth3 -nnXXtttt -w /tmp/gateway_eth3_capture.pcap -C 3 -G 600 
```

- In `lb_SH_2`, update the eBPF Map `mqtt_topic_to_vip` so that depicts that the responsible broker for topic `sensors/` is  `real_1` and instruct the `client_ips` Map with the client IP
```bash
# LB from Scratch - Update eBPF map from userspace  [topic -> responsible broker]
export MAP_ID=$(bpftool map list | grep mqtt_topic | awk -F':' '{ print $1 }')
bpftool map show id $MAP_ID
cd xdp-tutorial/basic00-update-map
./user_bpfmap $MAP_ID sensors/ $REAL_1_IP
bpftool map dump id $MAP_ID

# eBPF Map that will store [key:1 -> CLIENT_IP]
export MAP_ID=$(bpftool map list | grep client_ips | awk -F':' '{ print $1 }')
bpftool map show id $MAP_ID
# CLIENT_IP=10.1.1.107
bpftool map update id $MAP_ID key 1 0 0 0 value 10 1 1 107 0 0 0 0 0 0 0 0 0 0 0 0
bpftool map dump id $MAP_ID
```

- In `client_SH`, make the MQTT publish messages
```bash
cd utils
./massive_pub.sh
# How many times would you like to Do MQTT PUB? 3
# Enter the QOS level (0, 1, or 2): 0
```

- (change client IP): set `CLIENT_IP=10.1.1.102` in the `.env` file

- Docker compose of `client` AGAIN.

- In `lb_SH_2`,  instruct the `client_ips` Map with the NEW client IP
```bash
# eBPF Map that will store [key:1 -> new CLIENT_IP]
export MAP_ID=$(bpftool map list | grep client_ips | awk -F':' '{ print $1 }')
bpftool map show id $MAP_ID
# CLIENT_IP=10.1.1.102
bpftool map update id $MAP_ID key 1 0 0 0 value 10 1 1 102 0 0 0 0 0 0 0 0 0 0 0 0
bpftool map dump id $MAP_ID
```

- In `client_SH`, make the MQTT publish messages (again)... (Now client publishes from the new IP)
- Stop and Copy the trace pipe logs from `lb_SH_1`
- Stop and save the capture from `gateway_SH`




## Test 2

### Limitations - Modifications since previous test

Feature:
- Variable MQTT Topic Length up to 80 characters is supported in this version

The rest limitations have not been addressed yet.

### MQTT-Topic based forwarding logic 

| topic        | Broker  |
| ------------ | ------- |
| sensors/     | real_1  |
| living-room/ | real_2  |
| other topics | real_3  |

### Test Procedure
- Publish 3 times to a big 80-char topic that has broker_1 as the responsible server. The first MQTT PUBLISH will fail (the TCP 3WHS and MQTT connect were forwarded to real_3 as there was no predicted topic). The next two PUBLISH messages will be successfully forwarded to real_1.
- Publish 3 times to `sensors/` so that the PUBLISH messages go to `real_1`. Now all 3 messages will succeed. In fact, the first one is lucky due to the fact that the previous topic that was sent by the client IP was also intended to be forwarded to real_1 (so although predicted_topic is different - the 3WHS has luckily been done with the right broker - real_1)
- Publish 3 times to `living-room/` so that the PUBLISH messages go to `real_2`. Here the first PUBLISH will fail (TCP 3WHS with real_1) but the next ones will properly be sent to real_2

### Commands


- Docker compose of `client`, `lb_from_scratch`, `real_1`, `real_2`, `gateway` containers 

- 4 terminals open were you will connect to containers `docker exec -it <container_name> sh`: `client_SH`, `lb_SH_1`, `lb_SH_2`, `gateway_SH`

- In `lb_SH_1`, check the trace pipe (where `bpf_printk` commands write their output)
```bash
bpftool prog tracelog
```

- Gateway captures packets on the interface that looks to the `lb_from_scratch`. So, in `gateway_SH`
```bash
tcpdump -n -i eth3 -nnXXtttt -w /tmp/gateway_eth3_capture.pcap -C 3 -G 600 
```


```bash
# LB from Scratch - Update eBPF map from userspace  [topic -> responsible broker]
export MAP_ID=$(bpftool map list | grep mqtt_topic | awk -F':' '{ print $1 }')
bpftool map show id $MAP_ID
cd xdp-tutorial/basic00-update-map
./user_bpfmap $MAP_ID aabbccddeeaabbccddeeaabbccddeeaabbccddeeaabbccddeeaabbccddeeaabbccddeeaabbccddee $REAL_1_IP
./user_bpfmap $MAP_ID qqwweerrttqqwweerrttqqwweerrttqqwweerrttqqwweerrttqqwweerrttqqwweerrttqqwweerrtt $REAL_2_IP
./user_bpfmap $MAP_ID sensors/ $REAL_1_IP
./user_bpfmap $MAP_ID living-room/ $REAL_2_IP
bpftool map dump id $MAP_ID


# client
# expected to be fwd to real 1
# 3 times
mosquitto_pub -h ${SCRATCH_LB_IP}  -p ${MQTT_PORT} -m "motor temp, current, ..." -t aabbccddeeaabbccddeeaabbccddeeaabbccddeeaabbccddeeaabbccddeeaabbccddeeaabbccddee --qos 0

# 3 times
mosquitto_pub -h ${SCRATCH_LB_IP}  -p ${MQTT_PORT} -m "motor temp, current, ..." -t sensors/ --qos 0

#expected to be fwd to real 2
# 3 times
mosquitto_pub -h ${SCRATCH_LB_IP}  -p ${MQTT_PORT} -m "motor temp, current, ..." -t living-room/ --qos 0
```
- Stop and Copy the trace pipe logs from `lb_SH_1`
- Stop and save the capture from `gateway_SH`


- If you try `netstat -tn` for the `test-2/test-3` version (exactly after the first publish command that will not succed), 
you will get a tcp active connection that has non-zero Send-Q and is in state FIN_WAIT1. Connection has not closed by client even if client received a TCP RST from the real.

```bash
# netstat -tn
Active Internet connections (w/o servers)
Proto Recv-Q Send-Q Local Address           Foreign Address         State      
tcp        0     39 10.1.1.102:47764        10.1.5.102:1883         FIN_WAIT1
```

<!--
python3 client_pub_opts.py -H 127.0.0.1 -t motor/ -P 1883 -k 5 -N 20 -S 1

mosquitto_pub -h ${SCRATCH_LB_IP}  -p ${MQTT_PORT} -m "" -t aabbccddeeaabbccddeeaabbccddeeaabbccddeeaabbccddeeaabbccddeeaabbccddeeaabbccddee --qos 0
-->

## Test 3

### Test Procedure

No modifications done on Load Balancer. 
- Send multiple MQTT PUBLISH messages of the same topic under a single TCP connection using a python `paho.mqtt` client
- All of them fail (as TCP 3WHS has been sent to a non-responsible server and the PUBLISH messages are sent to another server - the responsible one)
- Repeat the procedure - rerun the python client that sends multiple MQTT PUBLISH messages with the same topic 
- Now a new TCP connection is used and thus the messages are all properly forwarded to the same responsible broker.

### Command

```bash
# keep-alive of 3 sec
# send 6 messages
# sleep time between them: 1 sec
python3 client_pub_opts.py -H ${SCRATCH_LB_IP} -t living-room/ -P ${MQTT_PORT} -k 3 -N 6 -S 1

# first time we expect that no message will be sent correctly to the right browser (TCP 3WHS and connect with real_3 - though message intended for real_2)

# second time we run the command, as a new TCP 3WHS happens to the appropriate broker, all MQTT PUBLISH messages are correctly delivered to broker real_2
```


## Test 4

### Limitations

- variable topic len up to 8 chars
- 1st mqtt pub message lost every time the client changes IP, loss continues until client re-connects
- no IPv6 handling
- every packet destined to MQTT-port (1883) is handled in this way (idea: use the configurable `MQTT_VIP` to discriminate MQTT service)

### eBPF Programs

This version consists of multiple bpf (xdp) programs that are tail-called using `bpf_tail_call`.
- `xdp_root`: The XDP program that is attached to the network interface and makes a tail call to `root_array[1]` program
- `root_array[1]`: My XDP program `mqtt_fwd` that performs the MQTT topic-based forwarding logic. If the received packet is destined to the `MQTT_PORT = 1883` then this program **changes the destination IP to a VIP** that represents a group of reals/brokers. The group of reals is selected **based on the (predicted/actual) MQTT topic** that (would be / is) used in the communication (check previous explanations for this logic). At the end, the program makes tail call to `root_array[2]` either there was a modification in the frame or not.
- `root_array[2]`: here Katran `balancer_ingress` xdp program is registered so that takes responsibility for the load balancing task. Katran looks at the destination IP of the packet and selects the real-broker to which the frame will be forwarded based on the VIP - reals mapping (from eBPF Maps that have been modified from userspace). 5-tuple (proto, src IP, src port, dst IP, dst port) is also considered in order to forward packets from the same session to the same broker. While making the encapsulated IPIP packet, the old ip header destination address (which was modified by `mqtt_fwd` program) changes to the `MQTT_VIP` value (stored in `mqtt_service_vips` eBPF map modified by userspace) which is meant to be used from the client for the communication with LB (checksum also needs to be updated).



### Topic to VIP mapping

| topic        | VIP         |
| ------------ | ----------- |
| `sensor/a`   | VIP_A       |
| `sensor/b`   | VIP_B       |
| other topics | VIP_DEFAULT |

### VIP to reals mapping 

| VIP         | Reals (brokers)  |
| ----------- | ---------------- |
| VIP_A       | 1                |
| VIP_B       | 2                |
| VIP_DEFAULT | 3                |


### Commands

Open three terminals `termA`, `termB`, `termC` to interact with Katran container
```bash
docker exec -it katran sh
```

- On `termA`, Install xdproot and Run Katran Server-Loader.
```bash
cd /home/simple_user/katran
./install_xdproot.sh
```

```bash
cd /home/simple_user/katran/_build
sudo ./build/example_grpc/katran_server_grpc -balancer_prog ./deps/bpfprog/bpf/balancer.bpf.o  -forwarding_cores=0 -hc_forwarding=false -lru_size=10000 -default_mac ${GATEWAY_KATRAN_MAC} -map_path /sys/fs/bpf/jmp_${KATRAN_INTERFACE} -prog_pos=2 -mqtt_fwd true -mqtt_topic_based_fwd_prog ./deps/bpfprog/bpf/mqtt_topic_based_fwd.bpf.o -mqtt_prog_pos 1 -intf ${KATRAN_INTERFACE}
```

- After the previous commands, you should see 3 xdp programs loaded in bpf/kernel (`xdp_root`, `mqtt_fwd`, `balancer_ingress`). You should also see a bpf Map `root_array` that contains the mapping `1 -> mqtt_fwd, 2 -> balancer_ingress` and is used for the `bpf_tail_call` calls. You can inspect these with the bpftool commands on `termB` (run the final command also to inspect the `bpf_printk` logs from the bpf programs):
```bash
bpftool prog list | grep xdp && \
bpftool map show name root_array && \
bpftool map dump name root_array && \
bpftool prog tracelog
```

<!--
Lookup / Disable mqtt_fwd program
bpftool map lookup name root_array key 1 0 0 0
bpftool map delete  name root_array key 1 0 0 0
-->

- On `termC`, run commands for the go client to configure VIPs and reals for Katran
```bash
cd /home/simple_user/katran/example_grpc/goclient/src/katranc/main

# configure a VIP groups
./main -A -t ${VIP_A}:${MQTT_PORT}
./main -a -t ${VIP_A}:${MQTT_PORT} -r ${REAL_1_IP} -w 1
./main -A -t ${VIP_B}:${MQTT_PORT} 
./main -a -t ${VIP_B}:${MQTT_PORT} -r ${REAL_2_IP} -w 1
./main -A -t ${VIP_DEFAULT}:${MQTT_PORT} 
./main -a -t ${VIP_DEFAULT}:${MQTT_PORT} -r ${REAL_3_IP} -w 1

# list available services (VIP -> reals mapping)
./main -l
```

- On `termC`, configure the mapping of topics to VIPs for `mqtt_fwd` program
```bash
# Update eBPF map from userspace  [topic -> VIP (group of responsible services)]
export MAP_ID=$(bpftool map list | grep mqtt_topic | awk -F':' '{ print $1 }') && \
bpftool map show id $MAP_ID  && \
cd /home/simple_user/xdp-tutorial/basic00-update-map  && \
./user_bpfmap $MAP_ID sensor/a $VIP_A  && \
./user_bpfmap $MAP_ID sensor/b $VIP_B  && \
bpftool map dump id $MAP_ID
```

- On `termC`, configure the general mqtt VIP that the client uses, so that `balancer_ingress` changes the destination IP to this one instead of the specific VIP that was given by the `mqtt_fwd` program
```bash
export MAP_ID=$(bpftool map list | grep mqtt_service | awk -F':' '{ print $1 }')
bpftool map show id $MAP_ID
# MQTT_VIP=10.1.50.200
# echo $MQTT_VIP | awk -F'.' '{ print $1  }'
bpftool map update id $MAP_ID key 0 0 0 0 value 10 1 50 200 0 0 0 0 0 0 0 0 0 0 0 0
bpftool map dump id $MAP_ID
```

- **Gateway**: Open terminal and start capturing on  `any` interfaces
```bash
docker exec -it gateway sh

tcpdump -n -i any -nnXXtttt -w /tmp/gateway_any_capture.pcap -C 3 -G 600 
```

- **Client**: Open another terminal and Then try to publish messages to mqtt_LB/katran:
```bash
docker exec -it client sh

# run the following multiple (3+) times
mosquitto_pub -h ${MQTT_VIP} -t sensor -p ${MQTT_PORT} -m "motor temp, current, ..."
# Expectations (all times sent to VIP_DEFAULT group - no predicted topic / no matching topic-VIP mapping found)
#               in our case goes always to real 3
```

- Stop and collect the captures on gateway containers

- Collect logs from `bpftool prog tracelog` running on `termB`


## Test 5

Minor fixes concerning a bug and better logging.
Test shows the behavior of `mqtt_LB` when receiving many MQTT messages with different topics
As expected the first packet with a topic that is meant to be sent to a different VIP group does not properly arrive on any of the responsible brokers due to false topic prediction. All other commands are the same. Hopefully,

- In this version, instead of manually pasting the commands you can use the `scripts/` provided in `/home/simple_user` of katran container
```bash
docker exec -it katran sh

# termA
./scripts/loader.sh
# termB
./scripts/debug.sh
# termC
./scripts/userspace.sh
```

- **Gateway**: Open terminal and start capturing on  `any` interfaces
```bash
docker exec -it gateway sh

tcpdump -n -i any -nnXXtttt -w /tmp/gateway_any_capture.pcap -C 3 -G 600 
```

- **Client**: Open another terminal and Then try to publish messages to mqtt_LB/katran:
```bash
# run the following multiple (3+) times
mosquitto_pub -h ${MQTT_VIP} -t sensor/a -p ${MQTT_PORT} -m "motor temp, current, ..."
# Expectations (see docker container logs):
# 1st time:  
#       Message is possibly NOT PUBLISHED to any broker (connection done with a broker from the VIP_DEFAULT VIP group) - Here real-3
# Next times: 
#       Message is successfully published to a responsible (member of VIP_A) broker  - Here real-1


# repeat procedure for sensor/b -> VIP_B
mosquitto_pub -h ${MQTT_VIP} -t sensor/b -p ${MQTT_PORT} -m "motor temp, current, ..."
# Expectations (see docker container logs):
# 1st time:  
#       Message is NOT PUBLISHED to any broker (connection done with a broker from the VIP_A - Here real-1 -
#       due to the fact that mqtt_fwd prog makes prediction of topic based on the client IP - 
#       Here predicted topic: sensor/a   that differs from the actual)
# Next times: 
#       Message is successfully published to a responsible (member of VIP_B) broker  - Here real-2  
#       (as the client-IP to mqtt topic mapping has been updated from the previous message)
```

- Stop and collect the captures on gateway containers

- Collect logs from `bpftool prog tracelog` running on `termB`

- (Optional) You can make just a single command that publishes an MQTT message (whose topic is expected to be mis-predicted) and notice that the `TCP RST` that comes from the real/broker and then the client remains silent (closes the TCP connection due to reset). This could be observed also in the capture, as the next `TCP SYN` after a `TCP RST` is after 3 seconds (generated by a user command - and not automatically generated). Short proof of the reset of the connection: if you immediately run `ss -tin` or `netstat -tn` after the publish command you won't see anything. 

- (Optional) (not captured) Similar results to Test 3 are produced by the following procedure
```bash
cd utils
python3 client_pub_opts.py -H ${MQTT_VIP} -t sensor -P ${MQTT_PORT} -k 3 -N 6 -S 1
```


## Test 6

In this test, we introduce more clients and reals.
- Each client publishes to a specific topic (many clients may publish to the same topic though)
- Each MQTT-topic may be intended for  many reals. 
- Each message will be received by only one of these reals.

### Clients publish to topics

| client | topic                      |
| ------ | -------------------------- |
|  1,2   | `measurements/temperature` |
| 3,4,5  | `measurements/humidity`    |
|   6    | `measurements/other`       |


### Topic to VIP  &  VIP to reals   mappings

| topic                      | VIP         | Reals (brokers)  |
| ---------------------------| ----------- | ---------------- |
| `measurements/temperature` | VIP_A       | 1,2              |
| `measurements/humidity`    | VIP_B       | 3,4              |
| other topics               | VIP_DEFAULT | 5,6              |


### Commands

- Start `katran` and `gateway` containers. Configure by userspace the BPF maps of katran using `scripts/userspace.sh`
```bash
docker compose up --build -d katran gateway real_[1-6]
# ensure all containers are up (retries may be needed for gateway)

docker exec -it katran sh -c "scripts/userspace.sh && scripts/debug.sh"
# wait and notice logs until the above script reaches the last command `bpftool prog tracelog`
```

- Start `clients` (setup script has been modified so that clients begin MQTT publishing 100 messages without further action) and notice **CPU, memory usage, NET I/O**  of `reals` containers during the test (be ready publishing-test lasts only some seconds and no longer than minute depending the `TOTAL_MESSAGES` and `SLEEP_TIME` env variables). Maybe, later, use grafana to monitor them
```bash
docker compose up --build -d client_[1-6]

# notice CPU, MEM, NET I/O
docker stats --no-stream 
```

- Gather experiment container logs and collect manually the `docker stats` and `bpftool prog tracelog` logs (then parse them with regex)
```bash
mkdir experiment_logs/
chmod +x gather-logs.sh
./gather-logs.sh
```

### Experiment Observations

```bash
user$[~/load-balancer-eBPF/lb-n-reals]
└──> python parse_logs.py --reals 6 --log_dir ./evolution/test-6
Client 2 sent 99 PUBLISH messages to real 1
Client 1 sent 99 PUBLISH messages to real 1
Client 4 sent 99 PUBLISH messages to real 4
Client 5 sent 99 PUBLISH messages to real 4
Client 3 sent 99 PUBLISH messages to real 4
Client 6 sent 100 PUBLISH messages to real 6
```

- We noticed that packets from the same client go to the same real/broker due to the 5-tuple based LB,
When there is keep-alive setting enabled, connection remains alive between client and broker and is verified through PINGREQ/PINGRESP periodically, so the same src port is used for MQTT communication and thus the 5-tuple remains the same and LB forwards to the same real/broker.
- When there is a faulty prediction concerning the topic, the first MQTT PUBLISH message is lost


### TODOs

- Parse logs
- Compare with PUBLISH to a single broker
- monitoring (cAdvisor, Prometheus, eBPF exporter, Grafana)
- Next test with VIP reals overlapping and change of client IP
- Other manner to measure using eBPF
- BPF_PRINTs should be removed for performance
- container resources limits
- Katran performance
- Comparison with simple broker response and Shared Subscriptions
- LPM eBPF Maps for support of wildcard `#` at the end of the topic 


## Test 7

Test 7 procedure is automated using the `test-7.sh` script (which uses multiple times the `experiment.sh` script)
```bash
cd lb-n-reals
chmod +x experiment.sh 
chmod +x test-7.sh
./test-7.sh
```

### Clients publish to topics

| client     | topic                      |
| ---------- | -------------------------- |
|  0,1,2,3   | `measurements/temperature` |
| 4,5,6,7,8  | `measurements/humidity`    |
|   9        | `measurements/other`       |


### Topic to VIP  &  VIP to reals   mappings

| topic                      | VIP         | Reals (brokers)  |
| ---------------------------| ----------- | ---------------- |
| `measurements/temperature` | VIP_A       | 1,2              |
| `measurements/humidity`    | VIP_B       | 3,4              |
| other topics               | VIP_DEFAULT | 5,6              |


### Topology

- 10 clients
- 6 reals/brokers
- 1 LB (Katran with mqtt_fwd program)
- Gateway that connects all the subnets

### Procedure

All the following steps are done automatically by the `experiment.sh` script. The procedure is the following:
- Start katran, gateway and reals containers. Katran container will load BPF programs and configure BPF maps from the setup. 
- Wait till Katran is ready (check logs) and then start clients container that will publish MQTT messages to the LB (each client publishes 100 messages in 10 seconds).
- When client publishes finish, gather logs and parse them with `parse_logs.py` script to verify that messages were sent to the correct brokers and that the first message of each client was lost due to false topic prediction (as expected).
- Check the `evolution/test-7/experiment*/results.txt` in order to inspect which publish messages are successfully delivered and validate that they are sent to one of the responsible brokers for the published topic and that the first message of each client is lost due to false topic prediction (as expected).
- Also you can notice CPU, MEM, NET I/O of reals containers during the test using the grafana dashboard with connection to `http://prometheus:9090`


This procedure will be repeated 3 times (as in `test-7.sh`).
1. Clients publish only to 1 broker
2. Client publish to the LB and LB forwards to the responsible brokers. However, the first publish message of each client may be lost due to the fact that LB has not seen this client IP previously and thus does not have a correct prediction of the topic that this client will publish to (and thus the TCP 3WHS and MQTT CONNECT are forwarded to a non-responsible broker). After the first message, LB learns the topic that each client publishes to and thus forwards correctly all the next messages.
3. Clients change IP and publish again (simulates the begginning of a new DHCP lease time period). As in the previous case, the first message is lost due to false topic prediction (new client IP) but all the next messages are correctly forwarded to the responsible brokers.


### Outcomes 

- When clients change IP or publish for first time, the first message is lost due to false topic prediction 
- All the next messages are correctly forwarded to the responsible brokers by the LB based on the topic of the message 
- Messages are load balanced per client as LB uses the 5-tuple for forwarding. Consequently, messages from the same client are forwarded to the same broker (as clients use keep-alive and thus the same src port for MQTT communication)

### Monitoring of test results

- Use of cAdvisor for container metrics along with Prometheus and Grafana for visualization.
- the `evolution/test-7/grafana.db` contains the dashboard made that contain metrics (`container_memory_rss`, `container_memory_usage_bytes`, `container_network_receive_packets_total`, `container_pressure_cpu_waiting_seconds_total`) 
- These metrics are shown in `evolution/test-7/test-7.png`
- For better metrics import Grafana dashboard from [cAdvisor Docker Insights](https://grafana.com/grafana/dashboards/19908-docker-container-monitoring-with-prometheus-and-cadvisor/)
- Experiment phases (1: 10.25-10.30 ,  2: 10.30-10.35, 3: 10.35-10.40) are shown in the graph 





## Test 8

This tests shows that LB can also change the payload of the MQTT PUBLISH message. Here, `mqtt_fwd` program is modified so that it changes the first 5 characters of the payload to uppercase. `mqtt_fwd` program assumes that will only receive MQTT PUBLISH messages with topic `measurements/temperature` and thus it forwards the curdata pointer by 24 chars (length of topic). 

Inspect **frame.number 29,31** in the capture to see the change of the payload `(caution -> CAUTIon)`

### Warnings

However, many factors were not taken into consideration
- TCP header checksum was not re-computed (this may cause client drop the segment as it may be invalid)
- forwarding the `curdata` pointer by a variable `topic_len` causes pointer invalidation and verifier rejects the bpf program. It may be done in another way using better boundary checks, though here, we assumed fixed len topic for simplicity
- If more bytes are added or some removed from the payload (using `bpf_xdp_adjust_(head|tail)`  - also the TCP header fields that indigate segment/payload length should be updated), then this change is inconsistent with the information/state variables that client keeps. For instance,
Client sends a 400 bytes segment with SEG.SEQ=0  so keeps the state variables SND.UNA = 0, SND.NXT = 400
If the load balancer changes the segment from 400 to 300 bytes (by removing some payload for example), 
the server that receives this segment will give an ACK with SEG.ACK = 300.
When client receives the ACK, client sees  SEG.ACK = 300  and compares with SND.NXT = 400, so understands that the last 100 bytes were not ACKed.
So client retransmits bytes 301-400 setting SEG.SEQ=0, and state variables SND.UNA = 300, SND.NXT = 400...
More weird things may happen if LB increases the segment size (so client would receive an ACK - e.g. 500 that would be greater than SND.NXT)

For all these cases, modifying the payload of TCP is **NOT recommended** and may cause a series of unwanted events. 

### Procedure

```bash
docker compose up --build -d

docker exec -it gateway sh
tcpdump -n -i any -nnXXtttt -w /tmp/gateway_any_capture.pcap -C 3 -G 600 

docker exec -it client_1 sh
# 3 times
mosquitto_pub -h ${MQTT_VIP}  -p ${MQTT_PORT} -m "caution: the first 5 characters will be converted to uppercase by the LB" -t "measurements/temperature" --qos 0

mkdir experiment_logs
chmod +x gather-logs.sh
./gather-logs.sh
source ../.venv/bin/activate
python parse_logs.py --reals 6 --log_dir experiment_logs/ | sort > experiment_logs/results.txt
cp .env experiment_logs/

# Notice the logs (Messages received by brokers)
### real_6
Received PUBLISH from auto-80261A0F-AEEB-4CE6-D285-407F101905F7 (d0, q0, r0, m0, 'measurements/temperature', ... (72 bytes))
### real_5
Received PUBLISH from auto-7CDEE752-B8D6-466A-EACA-78762F3823FC (d0, q0, r0, m0, 'measurements/temperature', ... (72 bytes))
Received PUBLISH from auto-2F327F23-28BA-9202-475D-0390AA132D52 (d0, q0, r0, m0, 'measurements/temperature', ... (72 bytes))
```


## Test 9

This test is conducted in two phases (A,B) with similar setup, In phase B, the only difference is that some extra statistics are enabled that show the CPU Usage of the xdp programs. However, the collection procedure of these stats affects Katran's userspace resources usage. So, in section A we will only discuss the userspace CPU usage and memory usage of Katran and in section B we will compare the CPU usage and network traffic of the brokers and the XDP CPU program usage vs Shared Subscription's broker CPU usage. 

### Procedure

- ensure `Docker Desktop` up and running
- Install and operate `node_exporter` to export specialized metrics to prometheus
```bash
# Install Node exporter 
# https://prometheus.io/docs/guides/node-exporter/

# (Instructions for Linux)
# Make a dir where the XDP prog metrics will be gathered to be scraped via prometheus
# Then give permission to this user to be able to write inside this dir
mkdir -p /var/lib/node_exporter/textfile_metrics/
sudo chown $USER:$USER /var/lib/node_exporter/textfile_metrics/

# Run the node_exporter 
./node_exporter --collector.textfile.directory=/var/lib/node_exporter/textfile_metrics/

# Ensure mode_exporter is running on 0.0.0.0:9100  where the prometheus container seeks for metrics
# if the port is busy use another port but take care to update prometheus.yml accordingly
```
- Run the experiment `./test.sh` like this
```bash
# Phase A, does not scrape XDP metrics
# Phase B, scrapes XDP metrics and thus affects Katran container userspace CPU and Memory usage
sed -i 's/^SCRAPE_XDP=.*/SCRAPE_XDP=0/' .env  && ./test.sh &&  sed -i 's/^SCRAPE_XDP=.*/SCRAPE_XDP=1/' .env  && sleep 300  &&  ./test.sh
```
- Results are kept in `evolution/test_9` directory. 
- If you want to save a prometheus snapshot with the data and inspect them with grafana, read `past_monitor/README.md`

### Clients publish to topics

| client     | topic                      |
| ---------- | -------------------------- |
|  0,1,2,3   | `measurements/temperature` |
| 4,5,6,7,8  | `measurements/humidity`    |
|   9        | `measurements/other`       |


### Topic to VIP  &  VIP to reals   mappings

| topic                      | VIP         | Reals (brokers)  |
| ---------------------------| ----------- | ---------------- |
| `measurements/temperature` | VIP_A       | 1,2              |
| `measurements/humidity`    | VIP_B       | 3,4              |
| other topics               | VIP_DEFAULT | 5,6              |


### Environment

This test was conducted in 3 phases. In all these phases these are common:
- 10 clients
- Each message is approximately 1000 Bytes
- Each client makes a total of 1000 MQTT Publish messages at a  16.7 Hz frequency  (1 message per 0.06 secs)
- Each client publishes to a specific topic specified in `client/setup.sh`

Consequently the network traffic (computing only MQTT PUBLISH message frames that are the vast majority) is around
```
MQTT PUBLISH frames traffic ~= (10 clients) * (1000 B) / (0.06 sec) = 166,666 B/sec  ~=  167 kB/sec  (1.3 Mbps)
```

The 3 different phases are the following. Client publish to
1. a single broker `real_0`
2. `katran`, the eBPF Load Balancer (actually `mqtt_fwd` + `balancer_ingress`)  which is in front of 6 brokers `real_[1-6]`  (L4 Load Balancing)
3. `shared_subs_broker`, a broker that maintains shared subscriptions , done from  6 containers `real_[1-6]`  and forwards the appropriate PUBLISH messages (L7 Load Balancing)

Notes:
- Logging has been disabled both in `katran` and `shared_subs_broker` to avoid performance overheads (as it would be in production environments)

- `experiment_results.txt` contains the actual results in term of received PUBLISH messages by the brokers for each phase. In the Load Balancer Test, *Ignore the messages published to real_0. These just exist in the .txt because the container real_0 was not restarted*
- `monitoring` contains images with graphs from grafana about CPU Usage, Received Network Traffic, Memory Usage.


### Container resources

For all containers `deploy-resources` section has `limits`=`reservations` to ensure isolation.

| Container             |  CPUs | Memory |
| --------------------- | ----- | ------ |
| katran                | 0.1   | 512M   |
| shared_subs_broker    | 0.1   | 512M   |
| real_.*               | 0.01  | 8M	 |
| client_.*             | 0.05  | 32M    |
| gateway               | 1     | 512M   |



### Observations - Test 9 A

Here we will only discuss the CPU and memory usage of the Load Balancer

`katran` container has **Max CPU Usage: 0.54%**

**Warning: This CPU Usage measured by cAdvisor is Katran userspace CPU usage and does not represent the CPU Usage of the XDP program - that's why we need phase B**

Katran shows a significant memory usage during initialization (39.1 MB) (perhaps due to the memory allocation of the BPF maps which are in the order of tens of MBs)


### Observations - Test 9 B

In phase B, we gather the xdp programs runtime by asking katran container via `docker exec katran bpftool ...` and export them to prometheus using `node_exporter`. Periodic Docker exec commands increase katran's userspace CPU usage and thus this will not be representing a real circumstance. In this experiment we compare the XDP program's vs Shared Subscriptions Broker CPU usage.


- In **Single Broker Test**, all messages are published to 
`real_0` (that's why it's **max CPU usage gets 99.8%** and **max received network traffic rate 171 KiB/s**)
- In **Load Balancer Test**, Load Balancing is done per client (katran sends the packet with the same 5-tuple to the same real if there exists a session in the LRU session eBPF map). So, clients publish to these reals (selected by the hash) and 

| Real     | clients  | Max CPU Usage  | Max Received Network Traffic Rate |
| -------- | -------- | -------------- | --------------------------------- |
| real_1   | 1,3      |   48.8%        |     64.8  KiB/s                   |
| real_2   | 0,2      |   47.2%        |     65.2  KiB/s                   |
| real_3   | 4,5,6,8  |   85.6%        | 	  134 KiB/s                    |
| real_4   | 7        |   23.6%        |      31 KiB/s                     |
| real_5   | 9        |   24.9%        |      32.1 KiB/s                   |

- In **Load Balancer Test**, `katran's xdp_root program` has **Max CPU Usage: 1.63 %** and katran container has **Max Received Network Traffic Rate 162 KiB/s** 

As expected the first message from each client is lost due to the fact that the `mqtt_fwd` program makes false prediction on the future MQTT topic (as this client IP has not been seen before and does not exist in the eBPF Map) 



- In **Shared Subscriptions Test**, Load Balancing is done per message and thus it is fairer. `shared_subs_broker` is a broker that maintains the subscription lists and e.g. each time a publish to `measurements/temperature` arrives, this should be forwarded to one of `real_1` or `real_2`, who have previously subscribed to `$share/vip_a/measurements/temperature` (shared subscription)

| Real     |  Max CPU Usage | Max Received Network Traffic Rate |
| -------- | -------------- | --------------------------------- |
| real_1   | 44.6%          |  34.4 KiB/s                        |
| real_2   | 46.3%          |  34.5 KiB/s                        |
| real_3   | 55.2%          |  43.4 KiB/s	                    |
| real_4   | 54.9%          |  43.1 KiB/s                        |
| real_5   | 31.1%          |  8.57 KiB/s                        |
| real_6   | 31.4%          |  8.69 KiB/s                        |

`shared_subs_broker` container has **Max CPU Usage: 27.7%** and **Max Received Network Traffic Rate 183 KiB/s** 








### Summary - Comparison

| Metric | eBPF Load Balancer | MQTT Shared Subscriptions Broker |
|--------|--------------------|----------------------------------|
| CPU Usage | **~2.2%** (1.63% (kern) +  0.54% (usr)) | 27.7% |
| Load Balancing | per client | **per message** (more balanced) |
| Brokers-Groups decided by | Katran config (VIP-reals) | Brokers by subscribing to `$share/<group_id>/<topic>` |
| Flexibility | many constraints (see below) | **conforms** with all MQTT rules | 

### Constraints

All the below constraints should be applied when using the eBPF Load Balancer (`mqtt_fwd` + `Katran`)
- **Each client publishes to a single topic** and a fixed destination IP: `MQTT_VIP`.
- **The first Publish message of each client may be lost**. Whenever client IP changes (e.g. once a day due to DHCP lease time expiry), again the first Publish may be lost.
- **All clients should have different IPs**. Client behind PAT have the same IP and consequently, their messages will be delivered successfully only if all of them publish to the same topic. More in this [issue](https://github.com/nickpapakon/load-balancer-eBPF/issues/11)
- MQTT topics up to 256 characters length are supported
- Clients should check the connection state and re-establish connection if needed.
- Only Unencrypted MQTT over **TCP/IPv4** is supported (Only QoS=0).

- `max_entries` of the eBPF Map `mqtt_client_ip_to_topic` specifies the max number of clients that can be supported. (This number can be up to 200,000 but not much more due to memory allocation limits - if you use enormous max_entries, you may experience error at the BPF loading phase - `libbpf: map 'mqtt_client_ip_to_topic': failed to create: -ENOMEM`). If more than the specified clients publish simultaneously, the topic prediction mechanism will possibly fail and the majority of messages will be lost. More in this [issue](https://github.com/nickpapakon/load-balancer-eBPF/issues/12)
- VIPs (in Katran project), `topic_to_vip` and `client_ip_to_topic` mappings are now configured with max 512 entries but can change in file `lb-n-reals/katran/bpf/mqtt_topic_based_fwd.h`

- Additionally, Katran constraints should be met also (no IP options set, no fragmented packets, L3 topology and Katran should be able to offload to the Default Gateway via his MAC, same NIC for ingress/egress, max packet size 3.5 k, DSR mode) [Katran Requirements](https://github.com/facebookincubator/katran?tab=readme-ov-file#environment-requirements-for-katran-to-run)



## Test 10

This test compares Katran without and with the `mqtt_fwd` XDP program
- `Simple Katran`: Only `balancer_ingress` XDP program running. It is not aware of the MQTT topic, just forwards based on the destination IP and VIP-reals mappings. So here, we have configured clients to publish to a different VIPs based on the topic  (in the other case, this differentiation was done by sniffing the MQTT headers in the Load Balancer's `mqtt_fwd` XDP program)

| Metric | Simple Katran | Katran + `mqtt_fwd` |
|--------|--------------------|----------------------------------|
| XDP progs CPU Usage | 0.73 % | 1.16 % |
| Clients should send to |  different VIPs based on topic | a fixed IP but there are constraints | 
| Each Client publishes to | any topic | one fixed topic |
| Topic-VIP mapping decided by | clients | `mqtt_fwd` eBPF Maps config |

This observation proposes that `mqtt_fwd` program affects the performance of Katran (but not dramatically)


## Test 11

Similar to test 9 but includes automated process to do both phases A and B.

### Run experiment

- ensure Docker Desktop up
- run the node_exporter  (see above for installation/config)
```bash
 ./node_exporter --collector.textfile.directory=/var/lib/node_exporter/textfile_metrics/
```

- run `./test.sh` like
```bash
sed -i 's/^SCRAPE_XDP=.*/SCRAPE_XDP=0/' .env  && ./test.sh &&  sed -i 's/^SCRAPE_XDP=.*/SCRAPE_XDP=1/' .env  && sleep 300  &&  ./test.sh
```

### Differences compared to test 9

- `SLEEP_TIME=0.2`
- 30 clients
- by this way we achieve a similar traffic (30 * 1/0.2   ~=   10 * 1 / 0.6) 
- clients with the same %10 result publish to the same topic

### Observations

- In phase A, Load Balancer Test: 8 of the clients loose more than the first message [open issue](https://github.com/nickpapakon/load-balancer-eBPF/issues/13) but not more than 50 messages each client.
- In all other tests the results are pretty similar with Test 9



## Test 12 

- 10 clients
- `TOTAL_MESSAGES` = 30000
- `SLEEP_TIME` = 0.01

### Observations

- Load Balancer **lost the majority of the messages** (received ~ 1000 / 30000  per client - roughly 3%)  whereas in the other experiments more than 95% of the messages are correctly delivered to the brokers. [issue reproduction-explanation](https://github.com/nickpapakon/load-balancer-eBPF/issues/13)
- Noticed publish period is 0.02 seconds for single-broker and shared_subs_broker  experiments, but 0.1 seconds for LB experiment
- Use the following time in the json dashboard to inspect the experiment (you can see the slow down and decrease in received network traffic in the LB experiments)
```bash
  "time": {
    "from": "2026-03-27T18:36:12.174Z",
    "to": "2026-03-27T21:43:53.078Z"
  },
```

## Test 13 

Shows that simple Katran also loses the majority of packets when there is increased traffic.
Related with [issue](https://github.com/nickpapakon/load-balancer-eBPF/issues/13)
