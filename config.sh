#!/bin/sh

#the target. i686-elf by default, however you can easily change this as
#long as you make sure the appropriate gcc, as and ar binaries exist.
export TARGET="i686-elf"

export AS="${TARGET}-as"
export AR="${TARGET}-ar"
export CC="${TARGET}-gcc"
export LD="${TARGET}-ld"
#gcc but with the right options for linking the kernel



export CFLAGS=" -O3 -g -std=gnu99"

#note: this is slightly misnamed, as this is the *source* system root.
#for the actual sysroot that is generated for iso production, see $IMAGEDIR
export SYSROOT="${PWD}/source"
export IMAGEDIR="${PWD}/image"

#directories of stuff to build
export KERNELDIR="/kernel"
export BOOTDIR="/boot"
export PREFIX="/usr"
export INCLUDEDIR="${PREFIX}/include" #libc will be here
export LIB="${PREFIX}/lib" #will keep all of our .o .a and .c files.
export LIBK="/libk" #libk will still be kept here
 
#directories to loop over and build (and clean). each of these has its own build and clean script
#that handles the entire directory, and puts important stuff to where they belong.
export DIRS=""
export DIRS="${DIRS} libk"
export DIRS="${DIRS} kernel"
#export DIRS="${DIRS} bootloader"

#directories that will be copied over to the image directory.
#consists of the boot directory for now.
export SYSDIRS=""
export SYSDIRS="${SYSDIRS} boot"

#the disk size in sectors. used by "build.sh" to make the image file.
export DISKSIZE="131072"
#root partition size in sectors. this is equivalent to 32M
export ROOTSIZE="65536"
