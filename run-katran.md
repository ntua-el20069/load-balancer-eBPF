# Run Katran Load Balancer - Server

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
git clone https://github.com/ntua-el20069/katran.git
cd katran/
```

- Run the script that collects dependencies and Build Katran
```bash
./build_katran.sh
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
sudo rm -rf /usr/local/go && sudo tar -C /usr/local -xzf go1.25.1.linux-amd64.tar.gz
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
sudo rm -rf /home/username/go
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
Now check that `go version` outputs the right version

- Install Go plugins for gRPC and protocol buffers
```bash
go install google.golang.org/protobuf/cmd/protoc-gen-go@latest
go install google.golang.org/grpc/cmd/protoc-gen-go-grpc@latest
```

- Get Go program dependencies / libraries
```
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
