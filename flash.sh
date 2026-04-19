#!/bin/bash
set -e

make -j8
st-flash write build/stmc.bin 0x8000000
# sudo cu -s 115200 -l /dev/cu.usbmodem103
