#!/bin/bash
set -e

make
st-flash write build/stmc.bin 0x8000000
sudo cu -s 9600 -l /dev/cu.usbmodem103
