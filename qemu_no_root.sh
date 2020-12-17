#!/bin/sh


#this shell script basically does the same thing as "qemu.sh" found in the same
#directory, except this one does not require root access.

#the other script requires root simply because it creates a loopback device
#and launches QEMU by giving that loopback device as a disk.
#I did that because I wanted to make everything feel as close to real
#hardware as possible, but that is not necessary and it can be frustrating to
#have to use sudo every time you test the OS. and so this script exists
#remove some of that frustration by removing UNNECESSARY STEPS  I TOOK
#THAT HAVE NO IMPORTANCE WHATSOEVER. gee, I'm an idiot.


#yeah, this file *literally* executes one, single command. I know.
exec qemu-system-i386 -drive file=disk.iso,format=raw


