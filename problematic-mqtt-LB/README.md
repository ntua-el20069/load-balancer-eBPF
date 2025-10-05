# Problematic MQTT Load Balancing

Simple idea about load balancing MQTT brokers. The issue we're experiencing here is a fundamental problem with load balancing stateful protocols like MQTT. When subscribers connect to one broker and publishers connect to another, the brokers don't share subscription information, so messages get "lost."

## Explanation of the problem

**split-brain** syndrome in MQTT:
- Subscriber connects to mqtt5-1 and registers for topic /sensor/data
- Publisher connects to mqtt5-2 and publishes to /sensor/data
- mqtt5-2 has no knowledge of the subscriber on mqtt5-1
- Message is lost! 



## Configuring a mosquitto MQTT cluster using Docker

- Confirm that you have the following project  structure
```bash
$ tree
.
├── broker-1
│   ├── config
│   │   ├── mosquitto.conf
│   │   └── pwfile
│   ├── data
│   │   └── mosquitto.db
│   └── log
├── broker-2
│   ├── config
│   │   ├── mosquitto.conf
│   │   └── pwfile
│   ├── data
│   │   └── mosquitto.db
│   └── log
├── docker-compose.yaml
├── haproxy
│   └── haproxy.cfg
├── README.md
```
Make the directories that do not exist
- In order to identify the username/password use the command
```bash
touch config/pwfile
```
then run the docker compose command
```bash
docker compose up --build -d
```
run the password generate command interactively into each of the broker-* containers (repeat the process for the two containers)
```bash
sudo docker exec -it mqtt5-1 sh # then repeat for mqtt5-2
mosquitto_passwd -c /mosquitto/config/pwfile labuser    # provide password labuser
exit
```

- You can run additional containers / local clients to test the mqtt load balancing:
```bash
# subscribe
mosquitto_sub -h localhost -p 1111 -t motor -t battery -u labuser  -P labuser
```
```bash
# publish
mosquitto_pub -h localhost -t battery -p 1111 -m "battery temp, current, ..." -u labuser -P labuser
```