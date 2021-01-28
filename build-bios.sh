#!/bin/sh



#handles the configuration stuff, like which compiler to use, where to build
#etc. etc.
. ./config.sh

echo "[main] starting build."



#build everything.
#simply executes all the build scripts for the specific parts of the system.
for DIR in ${DIRS}; do
	cd "${SOURCEDIR}/${DIR}/"
	echo "[main]  ${DIR}"
	"./build.sh"
done






echo "[main] builds done. Generating image file."
cd "${SYSROOT}/.."



dd if=/dev/zero of=disk.img bs=1M count=64

echo -e "2048 63488 0x80 *\n65536 65536 0x06 -" | sfdisk disk.img

echfs-utils -m -p0 disk.img quick-format 512
echfs-utils -m -p0 disk.img import root/boot/limine.cfg boot/limine.cfg 
echfs-utils -m -p0 disk.img import root/boot/kernel.elf boot/kernel.elf

# Background images are important.  
echfs-utils -m -p0 disk.img import root/boot/bg.bmp boot/bg.bmp  

# Now the TAR archive.
#cd "${SYSROOT}"
#tar cf ../root.tar *
#cd ..

#dd if=root.tar of=disk.img bs=512 seek=65536 conv=notrunc


# Now the FAT system.
dd if=/dev/zero of=root.fat bs=512 count=32768
mkfs.fat -F 16 -n "NETTAPUS" root.fat
mcopy -s -i root.fat root/* ::/

dd if=root.fat of=disk.img bs=512 seek=65536 conv=notrunc


limine-install disk.img

echo "[main] Done."
#rm root.tar
rm root.fat
sync

