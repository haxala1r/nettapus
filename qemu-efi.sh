#!/bin/sh

#this file is only different from the qemu-bios.sh script in that this one runs the OS with
#UEFI as the firmware, instead of BIOS.

qemu-system-x86_64 -cpu qemu64 -bios "./UEFI firmware/bios64.bin" -drive file=disk.img,format=raw 



