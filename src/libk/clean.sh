


LIBS="vga tty mem string pci disk fs interrupts kbd task"


for LIB in ${LIBS}; do
	cd "${SOURCEDIR}${LIBK}/${LIB}"
	for F in $(ls *.o); do
		rm ${F}
	done;
done;

cd "${SOURCEDIR}${LIBK}"
rm "libk.a"
 
