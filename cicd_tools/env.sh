#!/bin/bash

# Absolute path to the project root
export PROJECT_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)

# Toolchain setup
export TOOLCHAIN_PATH=/opt/gcc-arm-none-eabi/bin
export PATH=$TOOLCHAIN_PATH:$PATH

# Build output
export BUILD_DIR="$PROJECT_ROOT/build"
export ELF_FILE="$BUILD_DIR/output.elf"
# export BIN_FILE="$BUILD_DIR/output.bin"

# Optional debug prints
# echo "PROJECT_ROOT=$PROJECT_ROOT"
# echo "BUILD_DIR=$BUILD_DIR"
# echo "ELF_FILE=$ELF_FILE"
