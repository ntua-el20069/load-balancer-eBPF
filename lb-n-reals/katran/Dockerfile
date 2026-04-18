FROM katran_base

USER simple_user

# if needed to re-build Katran because a src file changed
# change the argument below to invalidate docker build cache for the Katran build step
ARG KATRAN_BUILD_CACHE_BUSTER=100
RUN git pull

ARG KATRAN_BPF_DIR=/home/simple_user/katran/katran/lib/bpf

WORKDIR ${KATRAN_BPF_DIR}

WORKDIR /home/simple_user/katran

ENV ENABLE_KATRAN_BPF_PRINTK=0

# enable bpf_printk if env var is set
RUN if [ "$ENABLE_KATRAN_BPF_PRINTK" -eq 1 ]; then \
        sed -i 's/^#define DIPLOMA_DEBUG.*/#define DIPLOMA_DEBUG 1/' ${KATRAN_BPF_DIR}/balancer.bpf.c; \
        sleep 5; \
    fi


ENV DISABLE_MQTT_LB=1

# enable bpf_printk if env var is set
RUN if [ "$DISABLE_MQTT_LB" -eq 1 ]; then \
        sed -i 's/^#define MQTT_LB_MODE.*/#define MQTT_LB_MODE 0/' ${KATRAN_BPF_DIR}/balancer.bpf.c; \
        sleep 5; \
    fi

RUN echo "[Katran Build] build and test Katran" && \
    export BUILD_AND_TEST=1 && \
    ./build_katran.sh

# If changes in Go client done 
# change the argument here to invalidate docker build cache for the Go client build step
# ARG GO_CLIENT_BUILD_CACHE_BUSTER=1

WORKDIR /home/simple_user/katran/example_grpc

# Build the gRPC client
RUN go mod init example_grpc && \
    go mod tidy && \
    ./build_grpc_client.sh


WORKDIR /home/simple_user

## Use of xdp-tutorial repo to build userspace eBPF programs

# clone the xdp-tutorial repo inorder to get some common files for userspace eBPF programs
RUN git clone --recurse-submodules https://github.com/xdp-project/xdp-tutorial.git

RUN cd xdp-tutorial && \
    chmod +x ./configure && \
    ./configure && \
    rm -rf basic* packet* tracing* advanced*

# until here, if we do multiple docker builds, 
# the deps will be cached (Docker Build Cache)
# whatever file changes below this line will invalidate the cache

COPY ./userspace/update-map /home/simple_user/xdp-tutorial/basic00-update-map
# COPY ./userspace/update-prog-array /home/simple_user/xdp-tutorial/basic00-update-prog-array
# COPY ./userspace/loader /home/simple_user/xdp-tutorial/basic00-loader

USER root


RUN cd /home/simple_user/xdp-tutorial && \
    make

COPY . .
COPY ./bpf/* /home/simple_user/katran/katran/lib/bpf/

RUN chmod +x /home/simple_user/setup.sh && \
    chmod +x /home/simple_user/scripts/* && \
    chmod +x /home/simple_user/normal_katran_scripts/*
# Add a script to set up interfaces or other configurations at container startup (need root access)

# TODO: make a new setup.sh that based on env variable runs the appropriate script
ENTRYPOINT [ "/home/simple_user/setup.sh" ]
