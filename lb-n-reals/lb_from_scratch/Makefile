TARGET = xdp

BPF_OBJ = ${TARGET:=_lb_kern.bpf.o}

xdp: $(BPF_OBJ)
	bpftool net detach xdpgeneric dev eth0
	rm -f /sys/fs/bpf/$(TARGET)
	bpftool prog load $(BPF_OBJ) /sys/fs/bpf/$(TARGET)
	bpftool net attach xdpgeneric pinned /sys/fs/bpf/$(TARGET) dev eth0 

# $(BPF_OBJ): %.o: %.c
# 	clang -S \
# 	    -target bpf \
# 	    -D __BPF_TRACING__ \
# 	    -Ilibbpf/src\
# 	    -Wall \
# 	    -Wno-unused-value \
# 	    -Wno-pointer-sign \
# 	    -Wno-compare-distinct-pointer-types \
# 	    -Werror \
# 	    -O2 -emit-llvm -c -o ${@:.o=.ll} $<
# 	llc -march=bpf -filetype=obj -o $@ ${@:.o=.ll}

obj := .

DEBUGBPF = -DDEBUG
DEBUGFLAGS = -O0 -g -Wall
PFLAGS = $(DEBUGFLAGS)
CLANG ?= clang
LLC ?= llc

INCLUDEFLAGS = -I$(obj)/usr/include \
	       -I$(obj)/include \
	       -I$(obj)


$(BPF_OBJ): ${BPF_OBJ:.o=.c}
	$(CLANG) $(INCLUDEFLAGS) $(EXTRA_CFLAGS) \
	$(DEBUGBPF) -D__KERNEL__ -Wno-unused-value -Wno-pointer-sign \
		-Wno-compare-distinct-pointer-types \
		-O2 -emit-llvm -c -g $< -o -| $(LLC) -march=bpf -filetype=obj -o $@

clean:
	bpftool net detach xdpgeneric dev eth0
	rm -f /sys/fs/bpf/$(TARGET)
	rm $(BPF_OBJ)
	rm ${BPF_OBJ:.o=.ll}

################################
################################
################################
################################
################################


# obj := .
# src := .


# DEBUGBPF = -DDEBUG
# DEBUGFLAGS = -O0 -g -Wall
# PFLAGS = $(DEBUGFLAGS)

# INCLUDEFLAGS = -I$(obj)/usr/include \
# 	       -I$(obj)/include \
# 	       -I$(obj)


# always = %.bpf.o

# HOSTCFLAGS += $(INCLUDEFLAGS) $(PFLAGS)
# HOSTCFLAGS_bpf_load.o += $(INCLUDEFLAGS) $(PFLAGS) -Wno-unused-variable

# # Allows pointing LLC/CLANG to a LLVM backend with bpf support, redefine on cmdline:
# #  make samples/bpf/ LLC=~/git/llvm/build/bin/llc CLANG=~/git/llvm/build/bin/clang
# LLC ?= llc
# CLANG ?= clang

# # Trick to allow make to be run from this directory
# all: $(always)
# 	$(MAKE) -C .. $$PWD/

# clean:
# 	$(MAKE) -C .. M=$$PWD clean
# 	@rm -f *~

# $(obj)/%.bpf.o: $(src)/%.bpf.c
# 	$(CLANG) $(INCLUDEFLAGS) $(EXTRA_CFLAGS) \
# 	$(DEBUGBPF) -D__KERNEL__ -Wno-unused-value -Wno-pointer-sign \
# 		-Wno-compare-distinct-pointer-types \
# 		-O2 -emit-llvm -c -g $< -o -| $(LLC) -march=bpf -filetype=obj -o $@


