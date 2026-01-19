#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <net/if.h>
#include <bpf/libbpf.h>
#include "tail_call.skel.h"

#ifndef libbpf_is_err
#define libbpf_is_err(ptr) ((unsigned long)(ptr) > (unsigned long)-1000L)
#endif

int main(int argc, char **argv)
{
    struct tail_call *skel;
    struct bpf_link *link = NULL;
    int err, key, prog_fd;
    int ifindex;
    const char *ifname;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <interface>\n", argv[0]);
        return 1;
    }
    ifname = argv[1];

    skel = tail_call__open();
    if (!skel) {
        fprintf(stderr, "Failed to open BPF skeleton\n");
        return 1;
    }

    err = tail_call__load(skel);
    if (err) {
        fprintf(stderr, "Failed to load BPF skeleton: %d\n", err);
        goto cleanup;
    }

    key = 0;
    prog_fd = bpf_program__fd(skel->progs.xdp_dummy_prog);
    if (prog_fd < 0) {
        fprintf(stderr, "Invalid FD for xdp_dummy_prog\n");
        goto cleanup;
    }
    err = bpf_map__update_elem(skel->maps.root_array,
                               &key, sizeof(key),
                               &prog_fd, sizeof(prog_fd),
                               0);
    if (err) {
        fprintf(stderr, "Failed to update root_array for xdp_dummy_prog: %d\n", err);
        goto cleanup;
    }

    key = 1;
    prog_fd = bpf_program__fd(skel->progs.xdp_load_balancer);
    if (prog_fd < 0) {
        fprintf(stderr, "Invalid FD for xdp_load_balancer\n");
        goto cleanup;
    }
    err = bpf_map__update_elem(skel->maps.root_array,
                               &key, sizeof(key),
                               &prog_fd, sizeof(prog_fd),
                               0);
    if (err) {
        fprintf(stderr, "Failed to update root_array for xdp_load_balancer: %d\n", err);
        goto cleanup;
    }

    ifindex = if_nametoindex(ifname);
    if (!ifindex) {
        perror("if_nametoindex");
        goto cleanup;
    }

    link = bpf_program__attach_xdp(skel->progs.xdp_root, ifindex);
    if (libbpf_is_err(link)) {
        err = libbpf_get_error(link);
        fprintf(stderr, "Failed to attach XDP program on %s (ifindex: %d): %d\n",
                ifname, ifindex, err);
        link = NULL;
        goto cleanup;
    }

    printf("XDP program loaded and tail calls configured on interface %s (ifindex: %d).\n",
           ifname, ifindex);
    printf("Press Ctrl+C to exit...\n");

    while (1)
        sleep(1);

cleanup:
    if (link)
        bpf_link__destroy(link);
    tail_call__destroy(skel);
    return err < 0 ? -err : 0;
}
