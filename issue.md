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
So, ipip60 interface cannot be set up for healthchecking. I tried to bypass this problem and just run katran with the command (omitting the `-healthchecker_prog` and `-ipip_intf` and `-ipip6_intf` arguments)
```bash
sudo ./build/example_grpc/katran_server_grpc -balancer_prog ./deps/bpfprog/bpf/balancer.bpf.o  -forwarding_cores=0  -intf=eth0  -lru_size=10000 -default_mac 02:42:0a:00:02:01 
>> ....
>>  E1030 21:22:27.986582   121 BaseBpfAdapter.cpp:144] libbpf: elf: failed to open ./healthchecking_ipip.o: -ENOENT
>> E1030 21:22:27.986606   121 BpfLoader.cpp:127] Error while opening bpf object: ./healthchecking_ipip.o, error: No such file or directory
>> terminate called after throwing an instance of 'std::invalid_argument'
>>  what():  can't load healthchecking bpf program, error: No such file or directory
>> *** Aborted at 1761859347 (Unix time, try 'date -d @1761859347') *** ...
```
However, `TODO`: I should check using `-hc_forwarding`

## Atempt on Ubuntu 22.04 host
