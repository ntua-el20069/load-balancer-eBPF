#!/usr/bin/env bash

set -xeo pipefail



DEPS_DIR="$(pwd)/deps"
WORK_DIR="$(pwd)"

get_clang() {
    if [ -f "${DEPS_DIR}/clang_installed" ]; then
        return
    fi

    if [ -f /etc/redhat-release ]; then
        yum install -y clang llvm
    else
        CLANG_DIR=$DEPS_DIR/clang
        rm -rf "$CLANG_DIR"
        pushd .
        mkdir -p "$CLANG_DIR"
        cd "$CLANG_DIR"
        echo -e "${COLOR_GREEN}[ INFO ] Downloading Clang ${COLOR_OFF}"
        # download platform appropriate version (9.0+) of clang from https://github.com/llvm/llvm-project/releases/
        wget https://github.com/llvm/llvm-project/releases/download/llvmorg-12.0.0/clang+llvm-12.0.0-x86_64-linux-gnu-ubuntu-20.04.tar.xz
        tar xvf ./clang+llvm-12.0.0-x86_64-linux-gnu-ubuntu-20.04.tar.xz
        echo -e "${COLOR_GREEN}Clang is installed ${COLOR_OFF}"
        popd
    fi
    touch "${DEPS_DIR}/clang_installed"
}

get_bpftool() {
    if [ -f "${DEPS_DIR}/bpftool_installed" ]; then
        return
    fi
    BPFTOOL_DIR="${DEPS_DIR}/bpftool"
    rm -rf "${BPFTOOL_DIR}"
    pushd .
    cd "${DEPS_DIR}"
    echo -e "${COLOR_GREEN}[ INFO ] Cloning bpftool repo ${COLOR_OFF}"
    git clone --recurse-submodules https://github.com/libbpf/bpftool.git || true
    cd "${BPFTOOL_DIR}"/src
    make
    mkdir -p "${DEPS_DIR}/bin"
    cp "${BPFTOOL_DIR}"/src/bpftool "${DEPS_DIR}/bin/bpftool"
    ln -s "${DEPS_DIR}/bin/bpftool" /usr/local/bin/bpftool
    echo -e "${COLOR_GREEN}bpftool is installed ${COLOR_OFF}"
    popd
    touch "${DEPS_DIR}/bpftool_installed"
}

# execute functions
get_clang
get_bpftool