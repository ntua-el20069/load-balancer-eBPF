#!/bin/bash

# Prompt the user for input
echo -n "How many times would you like to Do MQTT PUB? "
read count

echo -n "Enter the QOS level (0, 1, or 2): "
read qos_level

# Validate that the input is a positive integer
if [[ ! "$count" =~ ^[0-9]+$ ]]; then
    echo "Please enter a valid positive number."
    exit 1
fi

# Validate QOS level
if [[ ! "$qos_level" =~ ^[0-2]$ ]]; then
    echo "Please enter a valid QOS level (0, 1, or 2)."
    exit 1
fi

echo "Starting loop..."

# Loop for the specified number of times
for (( i=1; i<=count; i++ ))
do
    echo "MQTT PUB,  to ${SCRATCH_LB_IP},   port  ${MQTT_PORT}       (It: $i)"
    mosquitto_pub -h ${SCRATCH_LB_IP}  -p ${MQTT_PORT} -m "motor temp, current, ..." -t sensors/ --qos $qos_level
    sleep 2

done

echo "Finished!"