#!/bin/sh

#the target. i686-elf by default, however you can easily change this as
#long as you make sure the appropriate gcc, as and ar binaries exist.
export TARGET="x86_64-elf"
export PWD=$(pwd)

export AS="${TARGET}-as"
export AR="${TARGET}-ar"
export CC="${TARGET}-gcc"
export LD="${TARGET}-ld"
#gcc but with the right options for linking the kernel



export CFLAGS=" -O2 -std=gnu99 -Wall -Wextra -mcmodel=large -fno-pic -fno-stack-protector -mno-red-zone"

#uncomment this while debugging.
CFLAGS="${CFLAGS} -DDEBUG"


export SYSROOT="${PWD}/root"
export SOURCEDIR="${PWD}/src"

#directories of stuff to build
export KERNELDIR="/kernel"
export BOOTDIR="/boot"
export LIBK="/libk" #libk will still be kept here
 
#directories to loop over and build (and clean). each of these has its own build and clean script
#that handles the entire directory, and puts important stuff to where they belong.
export DIRS=""
export DIRS="${DIRS} libk"
export DIRS="${DIRS} kernel"
#export DIRS="${DIRS} bootloader"



#the disk size in sectors. used by "build.sh" to make the image file.
export DISKSIZE="524288"
export EFISIZE="131072"
export ROOTSIZE="65536"
export ROOTSTART="133120"

