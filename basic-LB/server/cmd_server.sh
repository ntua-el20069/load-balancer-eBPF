#!/bin/bash

# Check if an argument is provided
if [ $# -eq 0 ]; then
    echo "Usage: $0 <number from 1 to 10>"
    exit 1
fi

NUM=$1
PORT=$((8000 + NUM))

# Validate input is between 1-10
if [ "$NUM" -lt 1 ] || [ "$NUM" -gt 10 ]; then
    echo "Error: Number must be between 1 and 10"
    exit 1
fi

export NUM=$NUM
uvicorn app:app --host 0.0.0.0 --port $PORT