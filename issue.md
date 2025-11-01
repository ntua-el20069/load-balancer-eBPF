# Katran - Client - Gateway - Real Operation through Docker containers
I have tried 2 ways to build katran and client, real, gateway as containers in Docker Desktop using the same [code repository]() both times  and running on the same host physical machine (dual boot PC).
## Atempt on Windows host using WSL
On host Windows 11, using the WSL2 Ubuntu-22.04 distribution (entered using the command `wsl -d Ubuntu-22.04`). Under directory `lb-n-reals/`, `docker compose up --build -d` command starts all the containers. When the following commands are issued, they fail with the error messages provided:
```bash
$ sudo ip link add name ipip60 type ip6tnl external
>> Error: Unknown device type.

$ sudo ip link set up dev ipip60
>> Cannot find device "ipip60"

$ sudo /usr/sbin/ethtool --offload eth0 lro off
>> Cannot change large-receive-offload 
```
So, ipip60 interface cannot be set up for healthchecking.
```bash
sudo ./build/example_grpc/katran_server_grpc -balancer_prog ./deps/bpfprog/bpf/balancer.bpf.o  -forwarding_cores=0 -hc_forwarding=false -lru_size=10000 -default_mac 02:42:0a:00:02:01
```
Using `-hc_forwarding=false` I bypassed the problem and was able to run the katran server. I observed the readiness log `Server listening on 0.0.0.0:50051`.



## Atempt on Ubuntu 22.04 host
