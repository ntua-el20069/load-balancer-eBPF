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
- eBPF Map ``


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