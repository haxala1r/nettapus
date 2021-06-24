LIBC="../lib/libc.a"
for FILE in $(ls *.c)
do	
	mkdir -p "${ROOT}/bin"
	x86_64-elf-gcc -ffreestanding -nostdlib -c "${FILE}" -o "${FILE}.o"
	ld "${FILE}.o" ${LIBC} -o "${FILE%.c}"
	rm "${FILE}.o"
	cp "${FILE%.c}" "${ROOT}/bin/"
done
