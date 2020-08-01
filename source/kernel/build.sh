#1/bin/sh

echo "[/kernel/build.sh] building kernel..."

#the compiler and the linker we will use.
CCtemp="${CC} -ffreestanding -Wall -Wextra ${CFLAGS}"
KLINKER="${CC} -ffreestanding -lgcc -O2 -nostdlib"

#compile the kernel.
${CCtemp} -c "${SYSROOT}${KERNELDIR}/kernel.c" -o "${SYSROOT}${BOOTDIR}/kernel.o" --sysroot="${SYSROOT}" -isystem="${LIBK}/include"

#assemble the boot.S file
${AS} "${SYSROOT}${KERNELDIR}/boot.S" -o "${SYSROOT}${BOOTDIR}/boot.o"

#now link the two. note that this also links the kernel with libk, 
#which is built by another script in the libk directory in the main script.
${KLINKER} -T "${SYSROOT}${KERNELDIR}/link.ld" -o "${SYSROOT}${BOOTDIR}/kernel.bin" "${SYSROOT}${BOOTDIR}/boot.o" "${SYSROOT}${BOOTDIR}/kernel.o" "${SYSROOT}${LIBK}/libk.a"

echo "[/kernel/build.sh] done building kernel."


