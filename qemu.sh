#!/bin/sh

#this shell script basically runs the OS on QEMU by creating a loopback device
#with the disk image and then running QEMU using that loopback device as a disk.

#NOTICE: This script *must* be run as root, or else it cannot create a loopback device.
#also note that you don't necessarily need this script, you can always just burn it 
#to a real disk and give that to QEMU, but this is more convenient and definitely faster.

#one last note, please make sure that /dev/loop0 is not being used for anything
#while running this script. the script should clean up after itself, meaning
#/dev/loop0 will be detached after use.
. ./config.sh



dd if=/dev/zero of=/tmp/tempzero count=${DISKSIZE}

losetup /dev/loop0 /tmp/tempzero

#after setting up a blank loopback device all we need to do is burn the disk image into it.
dd if=disk.img of=/dev/loop0
sync
#now we can do stuff.
qemu-system-x86_64 -drive file=/dev/loop0,format=raw

losetup -d /dev/loop0

rm /tmp/tempzero


#all that talk and it's 8 lines eh?
#programming really is like a joke sometimes. That's why I love it.

