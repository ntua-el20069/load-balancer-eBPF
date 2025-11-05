# Setting up the network

## Client
```bash
docker exec -it client sh
ping ${GATEWAY_CLIENT_IP} -c 1 -W 2
ping ${KATRAN_IP} -c 1 -W 2
ping ${REAL_IP} -c 1 -W 2

# ping ${VIP_1} -c 1 -W 2 # expect to get # From ${KATRAN_IP} icmp_seq=1 Time to live exceeded

curl http://${REAL_IP}:8000

```

## Real
```bash
docker exec -it real-server sh
ping ${GATEWAY_REAL_IP} -c 1 -W 2
ping ${CLIENT_IP} -c 1 -W 2
ping ${VIP_1} -c 1 -W 2 # expect this to work (loopback address)
```

## Katran 
```bash
docker exec -it katran sh
ping ${GATEWAY_KATRAN_IP} -c 1 -W 2
ping ${REAL_IP} -c 1 -W 2
# ping ${VIP_1} -c 1 -W 2 # should not work
```

## Gateway
```bash
docker exec -it gateway sh
ping ${CLIENT_IP} -c 1 -W 2
ping ${KATRAN_IP} -c 1 -W 2
ping ${REAL_IP} -c 1 -W 2
# ping ${VIP_1} -c 1 -W 2 # should not work
```