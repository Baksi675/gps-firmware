#!/bin/bash
set -e

ROOT_DIR=$(cd "$(dirname "$0")/.." && pwd)
ELF_FILE="$ROOT_DIR/build/output.elf"

if [ ! -f "$ELF_FILE" ]; then
    echo "ERROR: ELF file not found: $ELF_FILE"
    exit 1
fi

openocd \
  -f interface/stlink.cfg \
  -f target/stm32f4x.cfg \
  -c "program $ELF_FILE verify reset exit"
