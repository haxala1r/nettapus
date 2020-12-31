#1/bin/sh

echo "[/kernel/build.sh] building kernel..."

#the compiler and the linker we will use.
CCtemp="${CC} -ffreestanding -Wall -Wextra ${CFLAGS}"
KLINKER="${CC} -ffreestanding -lgcc -O3 -nostdlib"

#compile the kernel.
${CCtemp} -c "${SOURCEDIR}${KERNELDIR}/kernel.c" -o "${SOURCEDIR}${KERNELDIR}/kernel.o" --sysroot="${SOURCEDIR}" -isystem="${LIBK}/include"

#assemble the boot.S file
${AS} "${SOURCEDIR}${KERNELDIR}/boot.S" -o "${SOURCEDIR}${KERNELDIR}/boot.o"

#now link the two. note that this also links the kernel with libk, 
#which is built by another script in the libk directory in the main script.
${KLINKER} -T "${SOURCEDIR}${KERNELDIR}/link.ld" -o "${SYSROOT}${BOOTDIR}/kernel.bin" "${SOURCEDIR}${KERNELDIR}/boot.o" "${SOURCEDIR}${KERNELDIR}/kernel.o" "${SOURCEDIR}${LIBK}/libk.a"



echo "[/kernel/build.sh] done building kernel."


