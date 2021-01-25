#1/bin/sh

echo "[/kernel/build.sh] building kernel..."

#the compiler and the linker we will use.
CCtemp="${CC} -ffreestanding ${CFLAGS} "
KLINKER="${CC} -ffreestanding -lgcc -O0 -nostdinc  -nostdlib -no-pie -static -mcmodel=kernel -z max-page-size=0x1000"

#compile the kernel.
${CCtemp} -c "${SOURCEDIR}${KERNELDIR}/kernel.c" -o "${SOURCEDIR}${KERNELDIR}/kernel.o" --sysroot="${SOURCEDIR}" -isystem="${LIBK}/include"

#assemble the boot.S file
${AS} "${SOURCEDIR}${KERNELDIR}/asm.S" -o "${SOURCEDIR}${KERNELDIR}/asm.o"

#now link the two. note that this also links the kernel with libk, 
#which is built by another script in the libk directory in the main script.
${KLINKER} -T "${SOURCEDIR}${KERNELDIR}/link.ld" -o "${SYSROOT}${BOOTDIR}/kernel.elf" "${SOURCEDIR}${KERNELDIR}/kernel.o" "${SOURCEDIR}${KERNELDIR}/asm.o" "${SOURCEDIR}${LIBK}/libk.a"



echo "[/kernel/build.sh] done building kernel."


