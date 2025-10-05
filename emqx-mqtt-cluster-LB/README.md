# EMQX MQTT Cluster
In this section, services include a load balancer in front of an MQTT - EMQX cluster (two MQTT brokers that communicate).

## Set up with Docker
Just run the compose command:
```bash
docker compose up --build -d
```

## Dashboard
The dashboard of EMQX can be accessed at `localhost:18083` using the following credentials
- username: `admin`
- password: `public`

## Testing
The load balancer listens on port `1111`, 
so you can run additional containers / local clients to test the mqtt load balancing:
```bash
# subscribe
mosquitto_sub -h localhost -p 1111 -t motor -t battery
```
```bash
# publish
mosquitto_pub -h localhost -t battery -p 1111 -m "battery temp, current, ..."
```

