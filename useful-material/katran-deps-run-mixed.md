# OUTDATED - Run Katran Load Balancer - Server

## OUTDATED: OLD way - not preferrable
The following content may be **OUTDATED**.

The following setup of Katran LB Server was done on a computer with the following properties:
```bash
Operating System: Ubuntu 22.04.5 LTS              
Kernel: Linux 6.8.0-83-generic
Architecture: x86-64
```

## Build Katran Library

- Install basic required dependencies
```bash
sudo apt update
sudo apt install build-essential -y
sudo apt install git pkg-config libiberty-dev cmake clang-13 libelf-dev libfmt-dev
```

- Clone the forked `katran` repository (this repo contains changes in order the Katran Server to be able to interact with MQTT brokers and clients)
```bash
git clone https://github.com/nickpapakon/katran.git
cd katran/
```

- Run the script that collects dependencies and Build Katran
```bash
./build_katran.sh
```
or
```bash
./build_katran_for_root.sh
```
Additionally needed
```bash
apt install -y wget vim curl
```

last logs should be like ...
```txt
100% tests passed, 0 tests failed out of 2
Total Test time (real) =   ... sec
+ popd
~/path/to/katran/_build
```

## Construct Katran gRPC Server and Client

- make sure that BPF's jit is enabled
```bash
sudo sysctl net.core.bpf_jit_enable=1
```

### Go & Protobuf Installation
- Download [go toolchain](https://go.dev/doc/install), install, add to `PATH` env variable
```bash
cd ~/Downloads
wget https://go.dev/dl/go1.25.1.linux-amd64.tar.gz
sudo rm -rf /usr/local/go*
sudo rm -rf /home/simple_user/go
sudo rm -rf /usr/bin/go
printenv
export PATH=... # path var without go path
sudo tar -C /usr/local -xzf go1.25.1.linux-amd64.tar.gz
export PATH=$PATH:/usr/local/go/bin
export PATH="$PATH:$(go env GOPATH)/bin"
```

- Install protocol buffers compiler (requuired for gRPC)
```bash
sudo apt install -y protobuf-compiler
protoc --version  # Ensure compiler version is 3+
```

- (Optional) If the output of `go version` command does not return the version 1.25 and returns an old version, then some tools may not be supported and the Katran setup process may be disrupted. So, here are some steps to uninstall any older distribution (you should then proceed to the installation of Go step again)
```bash
sudo rm -rf /usr/local/go*
sudo rm -rf /home/simple_user/go
sudo rm -rf /usr/bin/go
export PATH=... # path var without go path

# 
```
Repeat installation of Go step ...
if `go version` now shows that go is not recognized, add a command to `~/.profile` file
```bash
nano ~/.profile
# ~/.profile should include the instruction thst sets PATH so it includes user's go directory if it exists
if [ -d "/usr/local/go/bin" ] ; then
    PATH="/usr/local/go/bin:$PATH"
fi
# Save the file and run the following
source ~/.profile
```

```bash
chmod +x ~/.profile
. ~/.profile
```

Now check that `go version` outputs the right version

- Install Go plugins for gRPC and protocol buffers
```bash
go install google.golang.org/protobuf/cmd/protoc-gen-go@latest
go install google.golang.org/grpc/cmd/protoc-gen-go-grpc@latest
```

```bash
cp $(go env GOPATH)/bin/protoc-gen-go-grpc $(go env GOPATH)/bin/protoc-gen-go_grpc
```

- Get Go program dependencies / libraries
```bash
cd path/to/katran/example_grpc
go mod init example_grpc
go mod tidy
```

- modify the paths in two following files

- `example_grpc/goclient/src/katranc/katranc/katranc.go`:
lb_katran "example_grpc/goclient/src/katranc/lb_katran"

- `example_grpc/goclient/src/katranc/main/main.go`:
"example_grpc/goclient/src/katranc/katranc"

### Run the script for Go get and build
```bash
./build_grpc_client.sh
```
last commands should be like
```bash
+ go build 
+ popd
```

### Set up parameters and interfaces
- Find MAC address of Default Gateway
```bash
$ ip route  | grep default
default via 192.168.1.1 dev wlp0s20f3 proto dhcp metric 600 
$ ip n show | grep 192.168.1.1
192.168.1.1 dev wlp0s20f3 lladdr 12:34:56:78:9a:bc REACHABLE
```


- Set up interfaces for forwarding healthchecking routes, and prepare the basic interface
```bash
sudo ip link add name ipip0 type ipip external
sudo ip link add name ipip60 type ip6tnl external
sudo ip link set up dev ipip0
sudo ip link set up dev ipip60
sudo tc qd add  dev wlp0s20f3 clsact

sudo apt install ethtool
sudo /usr/sbin/ethtool --offload wlp0s20f3 lro off
sudo /usr/sbin/ethtool --offload wlp0s20f3 gro off
```

- back to base directory, run katran server with parameters
```bash
cd ..
cd _build

sudo ./build/example_grpc/katran_server_grpc -balancer_prog ./deps/bpfprog/bpf/balancer.bpf.o  -forwarding_cores=0 -healthchecker_prog ./deps/bpfprog/bpf/healthchecking_ipip.o -intf=wlp0s20f3 -ipip_intf=ipip0 -ipip6_intf=ipip60 -lru_size=10000 -default_mac 12:34:56:78:9a:bc
```
logs should be like
```bash
Starting Katran
# possibly error and warning logs
Server listening on 0.0.0.0:50051
```


## Configuring Forwarding plane

### Configure real - server

The following steps are done on the real-server

- Setup the ipip interfaces (katran is using ipip as encapsulation for packet forwarding)
```bash
sudo ip link add name ipip0 type ipip external
sudo ip link add name ipip60 type ip6tnl external
sudo ip link set up dev ipip0
sudo ip link set up dev ipip60
```

- Specific to the linux is that for ipip interface to work - it must have at least single ip configured. We are going to configure an ip from 127.0.0.0/8 network as this is somehow artificial IP (it has local significance) - we could reuse the same IP across the fleet -
```bash
sudo ip a a 127.0.0.42/32 dev ipip0
```

- Since most of the time server is connected w/ a single interface - we don't need rp_filter feature:
```bash
sudo su
for sc in $(sysctl -a | awk '/\.rp_filter/ {print $1}'); do  echo $sc ; sudo sysctl ${sc}=0; done
exit
```

<!-- ```bash
for sc in $(sysctl -a | awk '/\.rp_filter/ {print $1}'); do  echo $sc ; sysctl ${sc}=0; done
``` -->

- We configure the VIP as loopback on server/real
```bash
sudo ip a a 192.168.1.219/32 dev lo
```

### Katran configuration using Go client

The following steps configure a VIP with a real on Katran, using the goclient
```bash
cd example_grpc/goclient/src/katranc/main
```

```bash
./main -A -t 192.168.1.219:8001
./main -a -t 192.168.1.219:8001 -r 192.168.1.38
./main -l


```

<!--
./main -A -t 10.1.2.102:8000
./main -a -t 10.1.2.102:8000 -r 10.1.3.102
./main -l
-->

## Test Topology

- Ensure Katran is running and listening on the right interface and has obtained as parameter the right gateway MAC address
- You have used goclient to configure VIP and real(s) for Katran
- You have configured real forwarding plane so that it receives IPIP encap packets.
- Real is running an http server on port `8001` and can reply to `GET /`

### Generate traffic from client
From a 3rd device use this command to send packets to Katran and receive response from real.
```bash
curl http://192.168.1.219:8001
```
<!--
# curl http://10.1.2.102:8000
-->

### Inspect eBPF - XDP program and trace log
```bash
sudo su
bpftool prog list
```

```txt
52: xdp  name balancer_ingress  tag c570f82acca29df4  gpl
	loaded_at 2025-10-11T19:59:25+0300  uid 0
	xlated 22168B  jited 12650B  memlock 24576B  map_ids 18,10,12,11,19,24,22,14,26,9,15,13,17,16,29
	btf_id 219
	pids katran_server_g(9476)
55: sched_cls  name healthcheck_encap  tag bcdf913b9987caf7  gpl
	loaded_at 2025-10-11T19:59:25+0300  uid 0
	xlated 912B  jited 496B  memlock 4096B  map_ids 31,32,33
	btf_id 220
	pids katran_server_g(9476)
```

```bash
bpftool prog tracelog
```


<!--
#####  ipip0 and ipip6tnl interfaces on Katran (needed only for healthchecking)

# setup interfaces for ipip encapsulation
ip link add name ipip0 type ipip external
ip link set up dev ipip0
ip a a ${LOCAL_IP_FOR_IPIP}/32 dev ipip0

# if you want to set down and delete interface
# ip link set down dev ipip0
# ip link del  ipip0

# the following interface type is not supported on WSL
# ip link add name ipip60 type ip6tnl external
# ip link set up dev ipip60

##### Traffic control and Queueing Discipline

# attach clsact qdisc on egress interface for usage in case of health checks
tc qd add  dev eth0 clsact

# if you want to show or delete the traffic control qdisc
# tc qd show dev eth0
# tc qd del dev eth0 clsact

-->



<!-- 
ip route
ip route add 10.1.0.0/16 via 10.1.1.102 dev eth0
ip route add 10.1.0.0/16 via 10.1.2.102 dev eth0
ip route add 10.1.0.0/16 via 10.1.3.102 dev eth0


ip route add 10.1.0.0/16 via 10.1.1.102 dev eth0


chmod +x /home/simple_user/go/bin/protoc-gen-go-grpc


tcpdump -i any -#XXtttt -w 2025_10_28__21_05_capture.pcap -C 3 -G 600


tcpdump -n -i eth1 -nnXXtttt -w /tmp/capture.pcap -C 3 -G 600 

tcpdump -n -i eth0 -nnXXtttt -w /tmp/gateway_eth0_capture.pcap -C 3 -G 600 

tcpdump -n -i eth1 -nnXXtttt -w /tmp/gateway_eth1_capture.pcap -C 3 -G 600 

tcpdump -n -i eth2 -nnXXtttt -w /tmp/gateway_eth2_capture.pcap -C 3 -G 600 

tcpdump -n -i eth3 -nnXXtttt -w /tmp/gateway_eth3_capture.pcap -C 3 -G 600 

# ip route del 10.1.1.0/24 dev eth0
# ip route del 10.1.2.0/24 dev eth1
# ip route del 10.1.3.0/24 dev eth2

tcpdump -n -i eth0 -nnXXtttt -w /tmp/mqtt_client_eth0_capture.pcap -C 3 -G 600 

cat katran/lib/bpf/balancer.bpf.c

# ip route add 10.1.1.0/24 via 10.1.1.102 dev eth0 src 10.1.1.101 onlink
# 
# 
# ip route add 10.1.2.0/24 via 10.1.2.102 dev eth1 src 10.1.2.101 onlink
# 
# ip route add 10.1.3.0/24 via 10.1.3.102 dev eth2 src 10.1.3.101 onlink

-->

<!--

- When I run the docker compose on my Ubuntu 22.04 host, sometimes the compose waits infinitely as `Docker Engine stopped` happens and I do not get any log. In fact I have set the resources of Docker Engine as  
```txt
- CPU limit: 12 
- Memory limit: 8 GB
- Swap: 2GB
- Disk Usage Limit: 1 TB
```
and if I run `free -h`  I got this output once:
```txt
               total        used        free      shared  buff/cache   available
Mem:            15Gi       6,0Gi       352Mi       7,2Gi       9,0Gi       2,4Gi
Swap:           15Gi       3,0Gi        12Gi
```
When not enough memory, Docker stops without any log ? Is this ok ?

-->