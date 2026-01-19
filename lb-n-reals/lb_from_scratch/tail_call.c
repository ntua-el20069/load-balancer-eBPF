#include "xdp_lb_kern.h"
#include "MQTTPacket.h"

#define REAL_1_IP       (unsigned int)(10 + (1 << 8) + (3 << 16) + (201 << 24))
#define REAL_2_IP       (unsigned int)(10 + (1 << 8) + (3 << 16) + (202 << 24))
#define REAL_3_IP       (unsigned int)(10 + (1 << 8) + (3 << 16) + (203 << 24))
#define DEFAULT_CLIENT_IP       (unsigned int)(10 + (1 << 8) + (1 << 16) + (102 << 24))
#define SCRATCH_LB_IP   (unsigned int)(10 + (1 << 8) + (5 << 16) + (102 << 24))

#define GATEWAY_SCRATCH_LB_MAC_LAST_BYTE 0x01
#define SCRATCH_LB_MAC_LAST_BYTE 0x02

#define BPF_PRINT 1

#define ROOT_ARRAY_SIZE 3

struct {
  __uint(type, BPF_MAP_TYPE_PROG_ARRAY);
  __type(key, __u32);
  __type(value, __u32);
  __uint(max_entries, ROOT_ARRAY_SIZE);
} root_array SEC(".maps");

SEC("xdp")
int xdp_root(struct xdp_md* ctx) {

    if (BPF_PRINT) bpf_printk("In root XDP program, performing tail call to program 0\n");
    bpf_tail_call(ctx, &root_array, 0);

    if (BPF_PRINT) bpf_printk("Tail call failed in root XDP program\n");
    return XDP_PASS;
}

SEC("xdp")
int xdp_dummy_prog(struct xdp_md *ctx)
{
    if (BPF_PRINT) bpf_printk("In dummy XDP program\n");
    bpf_tail_call(ctx, &root_array, 1);

    if (BPF_PRINT) bpf_printk("Tail call failed in dummy XDP program\n");
    return XDP_PASS;
}


SEC("xdp")
int xdp_load_balancer(struct xdp_md *ctx)
{   
    // [Context]: take the pointer to packet data and data end
    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;

    if (BPF_PRINT) bpf_printk("\n\ngot something from NIC");



    // // TODO - CHANGEs
    // if (BPF_PRINT) bpf_printk("In XDP load balancer - return PASS\n");
    // return XDP_PASS;

    // [Ethernet header parsing]: (Based on EtherType, Pass to network stack any non-IPv4 packet)
    struct ethhdr *eth = data;
    if (data + sizeof(struct ethhdr) > data_end){
        if (BPF_PRINT) bpf_printk("[xdp_aborted] Not enough data for ethhdr");
        return XDP_ABORTED;
    }

    if (bpf_ntohs(eth->h_proto) != ETH_P_IP){
        if (BPF_PRINT) bpf_printk("[xdp_pass] Not an IPv4 packet, passing to network stack");
        return XDP_PASS;
    }

    // [IP header parsing]: (Based on Protocol, Pass to network stack any non-TCP packet), display src IP
    struct iphdr *iph = data + sizeof(struct ethhdr);
    if (data + sizeof(struct ethhdr) + sizeof(struct iphdr) > data_end){
        if (BPF_PRINT) bpf_printk("[xdp_aborted] Not enough data for iphdr");
        return XDP_ABORTED;
    }

    if (iph->protocol != IPPROTO_TCP){
        if (BPF_PRINT) bpf_printk("[xdp_pass] Not a TCP packet, passing to network stack");
        return XDP_PASS;
    }

    // [TCP header parsing]: display source and destination ports
    struct tcphdr *tcp = (void *)iph + sizeof(struct iphdr);
    if ((void *)tcp + sizeof(struct tcphdr) > data_end){
        if (BPF_PRINT) bpf_printk("[xdp_aborted] Not enough data for tcphdr");
        return XDP_ABORTED;
    }

    if (BPF_PRINT) bpf_printk("Got TCP packet from 0x%x", iph->saddr);
    if (BPF_PRINT) bpf_printk(" src Port: %d, dst Port: %d", bpf_ntohs(tcp->source), bpf_ntohs(tcp->dest));

    unsigned int mqtt_topic_wise_forwarding = 0;

    // [MQTT_VIP check]: Check if packet is destined to MQTT port
    if(bpf_ntohs(tcp->dest) != MQTT_PORT){
        if (BPF_PRINT) bpf_printk("Packet is not destined to MQTT port");
    }
    // [TCP payload]: Packet is destined to MQTT port, continue to topic prediction and parsing
    else {
        // [predicted topic based on src IP]: We suppose that the following message will be for the topic that was last sent by this client IP
        // []: lookup the predicted topic from BPF map based on source IP
        struct mqtt_topic_entry* predicted_topic = NULL;
        struct ip_addr_union src_ip = {};
        src_ip.ipv4 = iph->saddr;
        // [] TODO: IPv6 handling ?
        predicted_topic = bpf_map_lookup_elem(&mqtt_client_ip_to_topic, &src_ip);
        
        // [Forward packets destined to MQTT_VIP]: forward based on the predicted topic
        // []: If no predicted topic, no forwarding based on topic
        // []: If the packet is MQTT PUBLISH, the forwarding decision will be updated later based on the actual topic
        if (predicted_topic){
            struct vip_definition *vip_def;
            vip_def = bpf_map_lookup_elem(&mqtt_topic_to_vip, predicted_topic);
            if(vip_def){
                if (BPF_PRINT) bpf_printk("Forwarding packet based on predicted topic VIP");
                if (BPF_PRINT) bpf_printk("VIP: 0x%x, port: %d, proto: %d", vip_def->vip, bpf_ntohs(vip_def->port), vip_def->proto);
                mqtt_topic_wise_forwarding = vip_def->vip;
            } else {
                if (BPF_PRINT) bpf_printk("No VIP mapping found for predicted topic");
            }
        }
        else {
            if (BPF_PRINT) bpf_printk("No predicted topic for this client IP");
        }
        
        // [TCP Payload - MQTT Fixed header existence check]: Packet destined to MQTT port, check if it contains at least the MQTT fixed header
        MQTTHeader * mqtt_h = (MQTTHeader *) ( (void *)tcp + tcp->doff * 4 );
        unsigned char* curdata = (unsigned char*)mqtt_h;
        if((void *)(mqtt_h + 2) > data_end){
            if (BPF_PRINT) bpf_printk("Does not contain all the required MQTT hdr data (fixed header is 2 bytes min)");
        } 
        // [MQTT Fixed header parsing]: Packet contains Fixed Header, parse it (Control Header and Remaining Length)
        else {
            // [MQTT Control Header parsing]: get MQTT Packet type
            // MQTTHeader (mqtt_h) struct already points to the start of MQTT Fixed Header
            // 1st byte is control header byte (Control header fields parsed)
            if (BPF_PRINT) bpf_printk("This segment Contains MQTT packet");
            if (BPF_PRINT) bpf_printk("MQTT Packet type: %d", mqtt_h->bits.type);
            
            // forward pointer by a char (control header)
            readChar(&curdata);
            
            // [MQTT Remaining Length parsing]: parse Remaining Length field and safety checks
            int remaining_length = 0;
            // rc equals to the number of bytes needed to encode remaining length (up to 4 bytes)
            int rc = MQTTPacket_decodeBuf(curdata, (unsigned char *)data_end, &remaining_length); /* read remaining length */
            
            if (rc <= 0){
                if (BPF_PRINT) bpf_printk("[xdp_aborted] Error decoding remaining length rc=%d", rc);
                return XDP_ABORTED;
            }

            curdata += rc;
            if(curdata + remaining_length != (void *)data_end){
                if (BPF_PRINT) bpf_printk("[xdp_aborted] Remaining length does not match data length");
                return XDP_ABORTED;
            } 
            
            // [Check MQTT packet type]: Only process PUBLISH packets for LB decision
            // []: other packet types will be forwarded based on predicted topic (if any)
            if (mqtt_h->bits.type != PUBLISH){
                if (BPF_PRINT) bpf_printk("Not a PUBLISH packet type");
            }
            // [MQTT PUBLISH Variable header parsing]: checks enough length to read the 2 bytes for topic len?
            else if ((void*)curdata + 2 > (unsigned char *) data_end) {
                if (BPF_PRINT) bpf_printk("[xdp_aborted] Not enough data to read topic length");
                return XDP_ABORTED;
            }
            
            else {
                // [MQTT PUBLISH Variable header parsing]: parse the topic len and topic and safety checks
                __u16 topic_len = readInt(&curdata, data_end); /* increments pptr to point past length */
                if (BPF_PRINT) bpf_printk("Topic length: %d", topic_len);
                
                if(topic_len > MAX_SUPPORTED_TOPIC_LENGTH){
                    if (BPF_PRINT) bpf_printk("[xdp_aborted] Topic length exceeds MAX_SUPPORTED_TOPIC_LENGTH");
                    return XDP_ABORTED;
                } 
                else if (topic_len <= 0){
                    if (BPF_PRINT) bpf_printk("[xdp_aborted] Topic cannot have zero length");
                    return XDP_ABORTED;
                }
                else if ((void *)(curdata) + topic_len > (void *)data_end){
                    if (BPF_PRINT) bpf_printk("[xdp_aborted] Topic data exceeds packet boundary");
                    return XDP_ABORTED;
                }
                else {
                    // [MQTT PUBLISH Topic to VIP mapping]: lookup the topic in the BPF map to get the VIP to forward the packet to
                    struct vip_definition *vip_def;
                    struct mqtt_topic_entry topic_entry = {};
                    topic_entry.len = topic_len;
                    // clear topic_entry topic
                    memset(topic_entry.topic, 0, sizeof(topic_entry.topic));
                    // memcpy(topic_entry.topic, curdata, MAX_SUPPORTED_TOPIC_LENGTH);
                    for (int i = 0; i < MAX_SUPPORTED_TOPIC_LENGTH; i++){
                        if (i >= topic_len) break;
                        if ((void *)(curdata + i + 1) > (void *)data_end) break;
                        memcpy(&topic_entry.topic[i], &curdata[i], 1);
                    }
                    
                    
                    if (BPF_PRINT) bpf_printk("Topic: %s", topic_entry.topic);
                    if (BPF_PRINT) bpf_printk("Payload: %s", curdata);

                    // [Compare predicted topic with actual topic]: compare the actual topic with the predicted one from BPF map
                    int successful_prediction = 1;
                    if(!predicted_topic) successful_prediction = 0;
                    else if(topic_entry.len != predicted_topic->len) successful_prediction = 0;
                    else{
                        for (int i = 0; i < MAX_SUPPORTED_TOPIC_LENGTH; i++){
                            if (i >= topic_len) break;
                            if(topic_entry.topic[i] != predicted_topic->topic[i]){
                                successful_prediction = 0;
                                break;
                            }
                        }
                    }
                    
                    if (!successful_prediction){
                        if (BPF_PRINT) bpf_printk("Actual Topic does not match the predicted one based on BPF map - We update BPF map");
                        // [Update eBPF Map (key: client IP, value: last used topic)]: update the map with the new topic for this client IP
                        bpf_map_update_elem(&mqtt_client_ip_to_topic, &src_ip, &topic_entry, BPF_ANY);
                    }
                    

                    
                    // ensure that no forwarding is done based on predicted topic
                    mqtt_topic_wise_forwarding = 0;

                    vip_def = bpf_map_lookup_elem(&mqtt_topic_to_vip, &topic_entry);
                    if(vip_def){
                        if (BPF_PRINT) bpf_printk("Found VIP mapping for actual topic");
                        if (BPF_PRINT) bpf_printk("VIP: 0x%x, port: %d, proto: %d", vip_def->vip, bpf_ntohs(vip_def->port), vip_def->proto);
                        mqtt_topic_wise_forwarding = vip_def->vip;
                    } else {
                        if (BPF_PRINT) bpf_printk("No VIP mapping found for actual topic");
                    }
                }
                
            } // is a MQTT Publish packet
        } // contains at least Fixed Header
    } // packet destined to MQTT_PORT
    
    // Parse MQTT packet


    unsigned int CLIENT_IP = DEFAULT_CLIENT_IP;

    // *********************** This section is added to allow changing the client IP via BPF map ****************
    //                         this helps sending MQTT publish packets with different client IPs for testing 
    //   but the admin should modify the "client_ips" BPF map entry accordingly from userspace (e.g. withbpftool)
    // -> lookup client IP from BPF map 
    int key = 1;
    struct ip_addr_union *client_ip_struct;
    client_ip_struct = bpf_map_lookup_elem(&client_ips, &key);
    // -> (override default CLIENT_IP if you have info in the map)
    if (client_ip_struct){
        CLIENT_IP = client_ip_struct->ipv4;
        if (BPF_PRINT) bpf_printk("Using client IP from BPF map: 0x%x", CLIENT_IP);
    }
    // ****************** end of section (Ctrl+F client_ip_struct to see dependent sections) *******************

    if (iph->saddr == CLIENT_IP){
        if (mqtt_topic_wise_forwarding) iph->daddr = mqtt_topic_wise_forwarding;
        else                            iph->daddr = REAL_3_IP;
    }
    else{
        iph->daddr = CLIENT_IP;
    }
    iph->saddr = SCRATCH_LB_IP;
    eth->h_dest[5] = GATEWAY_SCRATCH_LB_MAC_LAST_BYTE;
    eth->h_source[5] = SCRATCH_LB_MAC_LAST_BYTE;
    
    iph->check = iph_csum(iph);

    return XDP_TX;
}

char _license[] SEC("license") = "GPL";