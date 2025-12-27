#include <stdio.h>
#include <stdlib.h>
#include <bpf/bpf.h>
#include <bpf/bpf_endian.h>
#include <bpf/libbpf.h>
#include <arpa/inet.h>  // For inet_pton


#define MAX_TOPIC_LENGTH 256
#define FIXED_TOPIC_LENGTH 8  // e.g. "sensors/" TODO: adjust

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

/*
    struct mqtt_topic_entry key = {
        .topic = topic_string,
        .len = FIXED_TOPIC_LENGTH
    };
    struct vip_definition value = {
        .vip = bpf_htonl(0x0A0132C8),
        .port = bpf_htons(1883),
        .proto = IPPROTO_TCP
    };

*/

int main(int argc, char** argv) {

    if (argc != 4) {
        fprintf(stderr, "Usage: %s <map_id> <topic_string> <vip_ip_addr>\n", argv[0]);
        fprintf(stderr, "Example: %s 123 sensors/ 10.1.50.150\n", argv[0]);
        return 1;
    }

    struct mqtt_topic_entry key = {};
    struct vip_definition value = {};

    // 1. Parse the BPF Map ID
    __u32 map_id = atoi(argv[1]);

    // 2. Parse the mqtt topic string (Map key)
    memset(key.topic, 0, sizeof(key.topic));
    memcpy(key.topic, argv[2], strlen(argv[2]));
    key.len = (__u16) strlen(argv[2]);

    // 3. Parse the vip ip address (Map value)
    value.port = bpf_htons(1883); // MQTT default port
    value.proto = IPPROTO_TCP;
    if (inet_pton(AF_INET, argv[3], &value.vip) != 1) {
        fprintf(stderr, "Invalid IPv4 address: %s\n", argv[3]);
        return 1;
    }

    // Find the map fd from the map id
    int map_fd = bpf_map_get_fd_by_id((__u32)map_id);
    if (map_fd < 0) {
        printf("Error getting map fd from id %d\n", map_id);
        return 1;
    }

    // Update the map with the new key-value pair
    int bpfError = bpf_map_update_elem(map_fd, &key, &value, BPF_ANY);
    if (bpfError) {
        printf("Error while updating value in map: %d\n", bpfError);
    }


    return 0;
}