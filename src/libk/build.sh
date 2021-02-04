#!/bin/sh
echo "[libk/build.sh] building libk..."
LIBS="vga tty mem string pci disk fs interrupts kbd task"

OBJS=""

for LIB in ${LIBS}; do
	cd "${SOURCEDIR}${LIBK}/${LIB}"
	for F in $(ls *.c   2> /dev/null); do
		${CC} -c "${F}" -o "${F}.o" ${CFLAGS} -ffreestanding -nostdlib --sysroot="${SOURCEDIR}" -isystem="/libk/include"
		OBJS="${OBJS} ${PWD}/${F}.o"
	done;
	
	for F in $(ls *.asm 2> /dev/null); do
		${AS} "${F}" -o "${F}.o"
		OBJS="${OBJS} ${PWD}/${F}.o"
	done;
	
	
done

cd "${SOURCEDIR}${LIBK}"

${AR} rcs libk.a ${OBJS}

echo "[libk/build.sh] libk done."

