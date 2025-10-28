# Setting up the network

## Client
```bash
docker exec -it client sh
ip route add 10.1.0.0/16 via 10.1.1.101 dev eth0
ping 10.1.1.101 -c 1
ping 10.1.2.102 -c 1
ping 10.1.3.102 -c 1

curl http://10.1.3.102:8000
curl http://10.1.2.102:8000
```

## Real
```bash
docker exec -it real-server sh
ip route add 10.1.0.0/16 via 10.1.3.101 dev eth0
ping 10.1.3.101 -c 1
ping 10.1.1.102 -c 1
```

## Katran 
```bash
docker exec -it katran sh
ip route add 10.1.0.0/16 via 10.1.2.101 dev eth0
ping 10.1.2.101 -c 1
ping 10.1.3.102 -c 1
```

## Gateway
```bash
docker exec -it gateway sh
ping 10.1.1.102 -c 1
ping 10.1.2.102 -c 1
ping 10.1.3.102 -c 1
```