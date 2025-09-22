# Brainstorming

The basic idea is to utilize [Katran](https://github.com/facebookincubator/katran) Library (Level 4 Load Balancer leveraging eBPF and XDP) for managing traffic between IoT devices and MQTT brokers. Root `README`, `USAGE`, `EXAMPLE` markdown documents provide a general explanation for the project and its possible usage

## Load Balancer main program

The main program that uses `KatranLb` class - specified  in [katran/lib/KatranLb.f]() - includes the following steps:
1. Initialization of `KatranLb` instance w/ config - check the explicit constructor of the class - `TODO`: [a]()
2. Load and Attach **eBPF programs** - utilizing `void loadBpfProgs();` and `void attachBpfProgs();`
- `TODO`: [b]()
3. Optionally add healthchecking programs - using `addHealthCheckerDst` function
4. Add **Virtual IP addresses** for each separate service - using `bool addVip(vip, flags)` - `TODO`: [c]()
5. Add the MQTT Brokers as **reals** to a certain **VIP** - using `bool addRealForVip(real, vip)` or `bool modifyRealsForVip(action, reals, vip)` - `TODO`: [d]()
6. Main program should periodically be informed about the status of MQTT brokers (CPU usage, Memory consumption, etc.). This could be done by making a request to a specific route (HTTP / gRPC) of Brokers or by subscribing the load balancer to a specific topic providing this info. When a broker is out of service or the CPU usage or memory consumption is too high, the program removes the `Real` (broker) from the `VIPs` (services) in which belongs - using `delRealForVip(real, vip)`. Alternatively, we can reduce the weight of a real so that it receives less traffic. This should be examined further. A function that seems that may be used for this operation is `bool modifyRealsForVip(real, flags, set)`.

## TODOs - EPICs
- `a`: KatranLb Config Parameters
- `b`: which eBPF programs should be loaded and attached?
- `c`: segragation of services - e.g. distinguish sub, pub and data fwd messages
- `d`: Design the VIP - reals grouping, e.g. the VIP corresponding to sub/pub should have MQTT Brokers as reals 
- `e`: Develop a loop in which requests are made to Brokers inorder to inspect CPU and memory usage metrics and the weights of the reals are updated based on the gained knowledge.