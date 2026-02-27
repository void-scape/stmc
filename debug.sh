#!/bin/bash
set -e

make
arm-none-eabi-gdb build/stmc.elf \
  -ex "target extended-remote :4242" \
  -ex "load" \
  -ex "monitor reset halt" \
  -ex "break main" \
  -ex "continue"
