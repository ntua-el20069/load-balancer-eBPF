#!/bin/bash

echo "🧪 Testing MQTT Cluster Setup"
echo "================================"

# Check if mosquitto clients are installed
if ! command -v mosquitto_pub &> /dev/null; then
    echo "❌ mosquitto-clients not found. Installing..."
    sudo apt-get update && sudo apt-get install -y mosquitto-clients
fi

# Function to test MQTT messaging
test_mqtt_cluster() {
    local broker_host=$1
    local broker_port=$2
    local test_name=$3
    
    echo ""
    echo "🔍 Testing $test_name on $broker_host:$broker_port"
    echo "----------------------------------------"
    
    # Start subscriber in background
    echo "📡 Starting subscriber..."
    timeout 10 mosquitto_sub -h $broker_host -p $broker_port -t "test/cluster" -v &
    SUB_PID=$!
    
    # Wait a moment for subscriber to connect
    sleep 2
    
    # Publish messages
    echo "📤 Publishing messages..."
    for i in {1..5}; do
        mosquitto_pub -h $broker_host -p $broker_port -t "test/cluster" -m "Message $i from $test_name"
        echo "  ✅ Published: Message $i"
        sleep 1
    done
    
    # Wait for subscriber to receive messages
    sleep 3
    
    # Clean up
    kill $SUB_PID 2>/dev/null || true
    echo "  🏁 Test completed for $test_name"
}

# Test EMQX Cluster
echo ""
echo "🌟 EMQX Cluster Test"
echo "==================="
echo "1. Start the cluster: docker-compose -f docker-compose-emqx.yaml up -d"
echo "2. Wait for cluster formation (30 seconds)"
echo "3. Check cluster status:"
echo "   docker exec emqx1 emqx ctl cluster status"
echo ""

read -p "Press Enter when EMQX cluster is running..."

test_mqtt_cluster "localhost" "1111" "EMQX-Cluster"

echo ""
echo "🔗 Additional EMQX Tests:"
echo "========================"
echo "• Dashboard: http://localhost:18083 (admin/public)"
echo "• HAProxy Stats: http://localhost:8404"
echo ""

# Test cluster failover
echo "🔄 Testing Failover (Optional):"
echo "1. Stop one node: docker stop emqx2"
echo "2. Verify messages still work"
echo "3. Restart node: docker start emqx2"
echo "4. Check cluster rejoin: docker exec emqx1 emqx ctl cluster status"

echo ""
echo "✅ Cluster testing script completed!"
echo "📋 Key points to verify:"
echo "   - Messages published to any node reach subscribers on any node"
echo "   - Cluster status shows both nodes joined"
echo "   - Failover works when one node goes down"