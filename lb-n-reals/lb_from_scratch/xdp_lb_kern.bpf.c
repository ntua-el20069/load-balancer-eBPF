#include "xdp_lb_kern.h"

#define REAL_1_IP       (unsigned int)(10 + (1 << 8) + (3 << 16) + (201 << 24))
#define REAL_2_IP       (unsigned int)(10 + (1 << 8) + (3 << 16) + (202 << 24))
#define CLIENT_IP       (unsigned int)(10 + (1 << 8) + (1 << 16) + (102 << 24))
#define SCRATCH_LB_IP   (unsigned int)(10 + (1 << 8) + (5 << 16) + (102 << 24))

#define GATEWAY_SCRATCH_LB_MAC_LAST_BYTE 0x01
#define SCRATCH_LB_MAC_LAST_BYTE 0x02

SEC("xdp")
int xdp_load_balancer(struct xdp_md *ctx)
{
    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;

    bpf_printk("got something from NIC");

    struct ethhdr *eth = data;
    if (data + sizeof(struct ethhdr) > data_end)
        return XDP_ABORTED;

    if (bpf_ntohs(eth->h_proto) != ETH_P_IP)
        return XDP_PASS;

    struct iphdr *iph = data + sizeof(struct ethhdr);
    if (data + sizeof(struct ethhdr) + sizeof(struct iphdr) > data_end)
        return XDP_ABORTED;

    if (iph->protocol != IPPROTO_TCP)
        return XDP_PASS;

    bpf_printk("Got TCP packet from %x", iph->saddr);

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