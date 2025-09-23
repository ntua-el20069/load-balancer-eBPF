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

sudo ./build/example_grpc/katran_server_grpc -balancer_prog ./deps/bpfprog/bpf/balancer.bpf.o -default_mac 12:34:56:78:9a:bc -forwarding_cores=0 -healthchecker_prog ./deps/bpfprog/bpf/healthchecking_ipip.o -intf=wlp0s20f3 -ipip_intf=ipip0 -ipip6_intf=ipip60 -lru_size=10000
```
logs should be like
```bash
Starting Katran
# possibly error and warning logs
Server listening on 0.0.0.0:50051
```