# Basic Load Balancer

This folder contains a docker compose stack of backend and Load balancer containers.

## HAProxy
- load balancer, configuration: `haproxy/haproxy.cfg`
- accessible at localhost and the external port defined on `compose.yaml`

## Backend
- backend containers run FastAPI Python Servers (uvicorn)
- they are not accessible from browser (port mapping has been commented)

## Setup
```bash
cd basic-LB/
docker compose up --build -d
```

## Test
- You can now make requests to the load balancer and each time a different server responds
```bash
curl http://localhost:1111/
```
- You can test the operation of the system via the `test.sh` script. You can see that backends reply alternatively as roundrobin load balancing technique indicates.
```bash
chmod +x test.sh
./test.sh
```
- Inspect server logs inside each container
```bash
docker container logs --follow backend-1
```