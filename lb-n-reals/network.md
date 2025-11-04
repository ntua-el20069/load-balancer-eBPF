# Setting up the network

## Client
```bash
docker exec -it client sh
ip route add 10.1.0.0/16 via 10.1.1.101 dev eth0
ping 10.1.1.101 -c 1 -W 2
ping 10.1.2.102 -c 1 -W 2
ping 10.1.3.102 -c 1 -W 2

# ping 10.1.50.50 -c 1 -W 2 # expect to get # From 10.1.2.102 icmp_seq=1 Time to live exceeded

curl http://10.1.3.102:8000

```

## Real
```bash
docker exec -it real-server sh
ip route add 10.1.0.0/16 via 10.1.3.101 dev eth0
ping 10.1.3.101 -c 1 -W 2
ping 10.1.1.102 -c 1 -W 2
ping 10.1.50.50 -c 1 -W 2 # expect this to work (loopback address)
```

## Katran 
```bash
docker exec -it katran sh
ip route add 10.1.0.0/16 via 10.1.2.101 dev eth0
ping 10.1.2.101 -c 1 -W 2
ping 10.1.3.102 -c 1 -W 2
# ping 10.1.50.50 -c 1 -W 2 # should not work
```

## Gateway
```bash
docker exec -it gateway sh
ip route add 10.1.50.0/24 via 10.1.2.102 dev eth1
ping 10.1.1.102 -c 1 -W 2
ping 10.1.2.102 -c 1 -W 2
ping 10.1.3.102 -c 1 -W 2
# ping 10.1.50.50 -c 1 -W 2 # should not work
```