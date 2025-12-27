#include "xdp_lb_kern.h"
#include "MQTTPacket.h"

#define REAL_1_IP       (unsigned int)(10 + (1 << 8) + (3 << 16) + (201 << 24))
#define REAL_2_IP       (unsigned int)(10 + (1 << 8) + (3 << 16) + (202 << 24))
#define CLIENT_IP       (unsigned int)(10 + (1 << 8) + (1 << 16) + (102 << 24))
#define SCRATCH_LB_IP   (unsigned int)(10 + (1 << 8) + (5 << 16) + (102 << 24))

#define GATEWAY_SCRATCH_LB_MAC_LAST_BYTE 0x01
#define SCRATCH_LB_MAC_LAST_BYTE 0x02

SEC("xdp")
int xdp_load_balancer(struct xdp_md *ctx)
{   
    // [Context]: take the pointer to packet data and data end
    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;

    bpf_printk("\n\ngot something from NIC");

    // [Ethernet header parsing]: (Based on EtherType, Pass to network stack any non-IPv4 packet)
    struct ethhdr *eth = data;
    if (data + sizeof(struct ethhdr) > data_end)
        return XDP_ABORTED;

    if (bpf_ntohs(eth->h_proto) != ETH_P_IP)
        return XDP_PASS;

    // [IP header parsing]: (Based on Protocol, Pass to network stack any non-TCP packet), display src IP
    struct iphdr *iph = data + sizeof(struct ethhdr);
    if (data + sizeof(struct ethhdr) + sizeof(struct iphdr) > data_end)
        return XDP_ABORTED;

    if (iph->protocol != IPPROTO_TCP)
        return XDP_PASS;

    // [TCP header parsing]: display source and destination ports
    struct tcphdr *tcp = (void *)iph + sizeof(struct iphdr);
    if ((void *)tcp + sizeof(struct tcphdr) > data_end)
        return XDP_ABORTED;

    bpf_printk("Got TCP packet from %x", iph->saddr);
    bpf_printk(" src Port: %d, dst Port: %d", bpf_ntohs(tcp->source), bpf_ntohs(tcp->dest));

    // [MQTT_VIP check]: Check if packet is destined to MQTT port
    if(bpf_ntohs(tcp->dest) != MQTT_PORT){
        bpf_printk("Not an MQTT packet");
    }
    // [TCP payload - MQTT Fixed header existence check]: Packet destined to MQTT port, check if it contains at least the MQTT fixed header
    else {
        MQTTHeader * mqtt_h = (MQTTHeader *) ( (void *)tcp + tcp->doff * 4 );
        unsigned char* curdata = (unsigned char*)mqtt_h;
      
        if((void *)(mqtt_h + 2) > data_end){
            bpf_printk("Does not contain all the required MQTT hdr data (fixed header is 2 bytes min)");
        } 
        // [MQTT Fixed header parsing]: Packet contains Fixed Header, parse it (Control Header and Remaining Length)
        else {
            // [MQTT Control Header parsing]: get MQTT Packet type
            // MQTTHeader (mqtt_h) struct already points to the start of MQTT Fixed Header
            // 1st byte is control header byte (Control header fields parsed)
            bpf_printk("This segment Contains MQTT packet");
            bpf_printk("MQTT Packet type: %d", mqtt_h->bits.type);
            
            // forward pointer by a char (control header)
            readChar(&curdata);
            
            // [MQTT Remaining Length parsing]: parse Remaining Length field and safety checks
            int remaining_length = 0;
            // rc equals to the number of bytes needed to encode remaining length (up to 4 bytes)
            int rc = MQTTPacket_decodeBuf(curdata, (unsigned char *)data_end, &remaining_length); /* read remaining length */
            
            if (rc <= 0){
                bpf_printk("Error decoding remaining length rc=%d", rc);
                return XDP_ABORTED;
            }

            curdata += rc;
            if(curdata + remaining_length != (void *)data_end){
                bpf_printk("Remaining length does not match data length");
            } 
            
            // [Check MQTT packet type]: Only process PUBLISH packets for LB decision
            if (mqtt_h->bits.type != PUBLISH){
                bpf_printk("Not a PUBLISH packet type");
            }
            // [MQTT PUBLISH Variable header parsing]: checks enough length to read the 2 bytes for topic len?
            else if ((void*)curdata + 2 > (unsigned char *) data_end) {
                bpf_printk("Not enough data to read topic length");
                return XDP_ABORTED;
            }
            
            else {
                // [MQTT PUBLISH Variable header parsing]: parse the topic len and topic and safety checks
                __u16 topic_len = readInt(&curdata, data_end); /* increments pptr to point past length */
                bpf_printk("Topic length: %d", topic_len);
                
                // TODO: handle variable length topics
                if(topic_len > MAX_TOPIC_LENGTH){
                    bpf_printk("Topic length exceeds MAX_TOPIC_LENGTH");
                } 
                else if (topic_len <= 0){
                    bpf_printk("Topic cannot have zero length");
                }
                else if (topic_len != FIXED_TOPIC_LENGTH){
                    bpf_printk("Topic length is not equal to FIXED_TOPIC_LENGTH");
                }
                else if ((void *)(curdata) + FIXED_TOPIC_LENGTH > (void *)data_end){
                    bpf_printk("Topic data exceeds packet boundary");
                }
                else {
                    // [MQTT PUBLISH Topic to VIP mapping]: lookup the topic in the BPF map to get the VIP to forward the packet to
                    struct vip_definition *vip_def;
                    struct mqtt_topic_entry topic_entry = {};
                    memcpy(topic_entry.topic, curdata, FIXED_TOPIC_LENGTH);
                    topic_entry.len = topic_len;
                    bpf_printk("Topic: %s", topic_entry.topic);
                    bpf_printk("Payload: %s", curdata);
                    vip_def = bpf_map_lookup_elem(&mqtt_topic_to_vip, &topic_entry);
                    if(vip_def){
                        bpf_printk("Found VIP mapping for topic");
                        bpf_printk("VIP: %x, port: %d, proto: %d", vip_def->vip, bpf_ntohs(vip_def->port), vip_def->proto);
                    } else {
                        bpf_printk("No VIP mapping found for topic");
                    }
                }
                
            } // is a MQTT Publish packet
        } // contains at least Fixed Header
    } // packet destined to MQTT_PORT
    
    // Parse MQTT packet




    if (iph->saddr == CLIENT_IP){
        iph->daddr = (bpf_ktime_get_ns() % 2)? REAL_1_IP : REAL_2_IP;
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