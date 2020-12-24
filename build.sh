#!/bin/sh



#handles the configuration stuff, like which compiler to use, where to build
#etc. etc.
. ./config.sh

echo "[main] starting build."


mkdir image
#build everything.
#simply executes all the build scripts for the specific parts of the system.
for DIR in ${DIRS}; do
	cd "${SYSROOT}/${DIR}/"
	echo "[main] reached target ${DIR}"
	"./build.sh"
done


for FOLDER in ${SYSDIRS}; do
	cp -R "${SYSROOT}/${FOLDER}" "${IMAGEDIR}/"
	
done;




echo "[main] builds done. generating ISO."




#go to the original place, and generate an ISO image of the image directory.
cd "${SYSROOT}/.."
if grub-file --is-x86-multiboot "${IMAGEDIR}/boot/kernel.bin";
then
	#if it is multiboot, then everything should work.
	#now we gotta create the disk image.
	grub-mkrescue "./image/" -o disk.iso
	
	#and now make a tarball out of the "image" direcory and burn that to
	#disk.iso and done
	
	
	echo "[main] ISO done. now you can test it."
	
else 
	echo "[main] it didn't come out as multiboot, can't generate ISO."
	
fi








