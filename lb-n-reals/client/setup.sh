#!/bin/bash

# client setup

ip route add 10.1.0.0/16 via 10.1.1.101 dev eth0
sleep infinity