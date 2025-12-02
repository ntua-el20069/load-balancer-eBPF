#!/usr/bin/env bash

set -xeo pipefail

DEPS_DIR=./deps

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

# execute functions
get_clang
