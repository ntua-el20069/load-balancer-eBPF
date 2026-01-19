#include <stdio.h>
#include <stdlib.h>
#include <bpf/bpf.h>
#include <bpf/bpf_endian.h>
#include <bpf/libbpf.h>
#include <arpa/inet.h>  // For inet_pton


int main(int argc, char** argv) {

    if (argc != 4) {
        fprintf(stderr, "Usage: %s <root_array_map_id> <key_index> <id of xdp_prog>\n", argv[0]);
        fprintf(stderr, "Example: %s 43 0 65\n  The program assumes that map with ID 43 is the root_array map \
            \n\t and it will find the fd of the BPF program with ID 65 \
            \n\t and then it will update the root_array eBPF Map so that \
            \n\t       index 0 ->  fd of corresponding eBPF program", argv[0]);
        return 1;
    }

    // 1. Parse the BPF Map ID
    __u32 root_array_map_id = atoi(argv[1]);
    __u32 key_index = atoi(argv[2]);
    __u32 xdp_prog_id = atoi(argv[3]);

    // 2. Get the fd of the root_array map
    int root_array_map_fd = bpf_map_get_fd_by_id(root_array_map_id);
    if (root_array_map_fd < 0) {
        fprintf(stderr, "Error getting root_array map fd by id %u\n", root_array_map_id);
        return 1;
    }

    // 3. Get the fd of the xdp_prog
    int xdp_prog_fd = bpf_prog_get_fd_by_id(xdp_prog_id);
    if (xdp_prog_fd < 0) {
        fprintf(stderr, "Error getting xdp_prog fd by id %u\n", xdp_prog_id);
        return 1;
    }

    // 4. Update the root_array map
    if (bpf_map_update_elem(root_array_map_fd, &key_index, &xdp_prog_fd, BPF_ANY) != 0) {
        fprintf(stderr, "Error updating root_array map at index %u with xdp_prog fd %d\n", key_index, xdp_prog_fd);
        return 1;
    }

    return 0;
}
