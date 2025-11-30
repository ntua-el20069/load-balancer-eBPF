# load-balancer-eBPF
A Load Balancer handling traffic from IoT devices and MQTT brokers based on eBPF and XDP tools

## MQTT Setup 

[Installation Guide for Mosquitto MQTT Broker](https://mosquitto.org/download/)
To install it on Ubuntu, run the following commands:

```bash
sudo apt-add-repository ppa:mosquitto-dev/mosquitto-ppa
sudo apt-get update
sudo apt-get install mosquitto
sudo apt-get install mosquitto-clients
```

You can ensure that the Mosquitto service is running with the following command:

```bash
sudo systemctl status mosquitto.service
```

## Running Mosquitto Broker Manually

[Mosquitto Documentation](https://mosquitto.org/documentation/)
You can stop the Mosquitto service with:

```bash
sudo systemctl stop mosquitto.service
```

After stopping the service, you can run the Mosquitto broker manually in a terminal (listening on a port, e.g. `1111`), using the command:
```bash
mosquitto -v -p 1111
```

You can publish messages from a terminal using:

```bash
mosquitto_pub -h localhost -t motor -p 1111 -m "motor temp, current, ..."
mosquitto_pub -h localhost -t battery -p 1111 -m "battery temp, current, ..."
```

You can use another terminal to subscribe to the topics (use many times the `-t` option) and receive messages:

```bash
mosquitto_sub -h localhost -p 1111 -t motor -t battery
```

## Simple quide to understand pub/sub model

1. Start the Mosquitto broker:
```bash
mosquitto -v -p 1111
```
2. Open a new terminal and subscribe to topics:
```bash
mosquitto_sub -h localhost -p 1111 -t motor -t battery
   ```
3. On another terminal, subscribe to a specific topic:
```bash
mosquitto_sub -h localhost -p 1111 -t motor
```
4. Open another terminal and publish messages to the topics:
```bash
mosquitto_pub -h localhost -t motor -p 1111 -m "motor temp, current, ..."
mosquitto_pub -h localhost -t battery -p 1111 -m "battery temp, current, ..."
```
5. You should see the messages appear in the terminals where you subscribed to the topics.


