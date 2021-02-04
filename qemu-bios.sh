#!/bin/sh
#this shell script runs the OS on QEMU. That's all it does. What did you expect?

qemu-system-x86_64 -drive file=disk.img,format=raw -cpu qemu64 -m 1G 
