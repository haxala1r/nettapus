


LIBS="vga tty mem string pci disk fs"


for LIB in ${LIBS}; do
	cd "${SYSROOT}${LIBK}/${LIB}"
	for F in $(ls *.o); do
		rm ${F}
	done;
done;

cd "${SYSROOT}${LIBK}"
rm "libk.a"
 
