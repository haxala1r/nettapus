


LIBS="vga tty mem string"


for LIB in ${LIBS}; do
	cd "${SYSROOT}${LIBK}/${LIB}"
	for F in $(ls *.o); do
		rm ${F}
	done;
done;

cd "${SYSROOT}${LIBK}"
rm "libk.a"
 