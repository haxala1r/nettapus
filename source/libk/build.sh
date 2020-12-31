#!/bin/sh
echo "[libk/build.sh] building libk..."
LIBS="vga tty mem string pci disk fs"

OBJS=""

for LIB in ${LIBS}; do
	cd "${SOURCEDIR}${LIBK}/${LIB}"
	for F in $(ls *.c); do
		${CC} -c "${F}" -o "${F}.o" ${CFLAGS} -ffreestanding -nostdlib --sysroot="${SOURCEDIR}" -isystem="/libk/include"
		OBJS="${OBJS} ${PWD}/${F}.o"
	done;

done

cd "${SOURCEDIR}${LIBK}"

${AR} rcs libk.a ${OBJS}

echo "[libk/build.sh] libk done."

