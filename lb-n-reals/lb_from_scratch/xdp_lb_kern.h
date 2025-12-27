#ifndef __BPF_PROG_INCLUDES
#define __BPF_PROG_INCLUDES

#include <stddef.h>
#include <string.h>

#include <linux/in.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/tcp.h>

#include "include/bpf.h"
#include "include/bpf_helpers.h"
#include "include/bpf_endian.h"
#include "include/bpf_common.h"

// If you change MAX_TOPIC_LENGTH, 
// be careful about the `len` type in mqtt_topic_entry struct below
#define MAX_TOPIC_LENGTH 256
#define FIXED_TOPIC_LENGTH 8  // e.g. "sensors/" TODO: adjust for dynamic length topics
#define MAX_VIPS 512
#define NO_FLAGS 0

struct mqtt_topic_entry {
    // TODO: can I optimize the topic to a variable length array?
    char topic[MAX_TOPIC_LENGTH];
    __u16 len;
};


// vip's definition for lookup
struct vip_definition {
  union {
    __be32 vip;
    __be32 vipv6[4];
  };
  __u16 port;
  __u8 proto;
};

// map the mqtt topic to a vip
struct {
  __uint(type, BPF_MAP_TYPE_HASH);
  __type(key, struct mqtt_topic_entry);
  __type(value, struct vip_definition);
  __uint(max_entries, MAX_VIPS);
  __uint(map_flags, NO_FLAGS);
} mqtt_topic_to_vip SEC(".maps");


static __always_inline __u16
csum_fold_helper(__u64 csum)
{
    int i;
#pragma unroll
    for (i = 0; i < 4; i++)
    {
        if (csum >> 16)
            csum = (csum & 0xffff) + (csum >> 16);
    }
    return ~csum;
}

static __always_inline __u16
iph_csum(struct iphdr *iph)
{
    iph->check = 0;
    unsigned long long csum = bpf_csum_diff(0, 0, (unsigned int *)iph, sizeof(struct iphdr), 0);
    return csum_fold_helper(csum);
}

#endif /* __BPF_PROG_INCLUDES */