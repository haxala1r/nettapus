#!/bin/sh
echo "[libk/build.sh] building libk..."
LIBS="vga tty mem string pci"

OBJS=""

for LIB in ${LIBS}; do
	cd "${SYSROOT}${LIBK}/${LIB}"
	for F in $(ls *.c); do
		${CC} -c "${F}" -o "${F}.o" ${CFLAGS} -ffreestanding -nostdlib --sysroot="${SYSROOT}" -isystem="/libk/include"
		OBJS="${OBJS} ${PWD}/${F}.o"
	done;

done

cd "${SYSROOT}${LIBK}"

${AR} rcs libk.a ${OBJS}

echo "[libk/build.sh] libk done."

