#!/bin/sh

#This file is responsible for compiling/assembling/linking the bootloader,
#and also creating a disk image that can be booted from, using the image directory
#as the sysroot that will be in the actual disk image.



${AS} stage1.S -o stage1.o
${AS} stage2.S -o stage2.o

#this might seem wrong, but I really don't wanna type stuff twice, so I just let the bootloader
#benefit from libk.
${CC} -c loadermain.c -o loadermain.o -ffreestanding ${CFLAGS} --sysroot="${SYSROOT}" -isystem="${LIBK}/include"


${LD} -T "link.ld" -o mbr_bootloader.elf -nostdlib stage1.o 

#as I said, the bootloader could also use some of the libk stuff, so we probably wanna link
#it against that as well. Might change this later, as this can impose some stuff in the future, but who cares lmao.
${LD} -T "link2.ld" -o stage2.elf stage2.o loadermain.o "${SYSROOT}${LIBK}/libk.a" -nostdlib 


objcopy mbr_bootloader.elf mbr.img -O binary
rm mbr_bootloader.elf	#we don't need this file after we just got the binary of it.

objcopy stage2.elf stage2.img -O binary
rm stage2.elf	#same thing with this one.



#write the MBR to the start of the disk.
dd if=mbr.img of=disk.img
sync
#seek to the start of the first partition and write the stage2 code there.
dd if=stage2.img of=disk.img seek=2048
sync


mv disk.img "${SYSROOT}/.."




echo "Done"
