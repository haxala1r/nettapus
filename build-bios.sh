#!/bin/sh



#handles the configuration stuff, like which compiler to use, where to build
#etc. etc.
. ./config.sh

echo "[main] starting build."



#build everything.
#simply executes all the build scripts for the specific parts of the system.
for DIR in ${DIRS}; do
	cd "${SOURCEDIR}/${DIR}/"
	echo "[main] reached target ${DIR}"
	"./build.sh"
done






echo "[main] builds done. Generating image file."




#go to the original place, and generate an ISO image of the image directory.
cd "${SYSROOT}/.."
if grub-file --is-x86-multiboot "${SYSROOT}/boot/kernel.bin";
then
	#if it is multiboot, then everything should work.
	#now we gotta create the disk image.
	dd if=/dev/zero of=disk.img bs=512 count=$DISKSIZE
	dd if=/dev/zero of=root_fat.img bs=512 count=$ROOTSIZE
	sync
	
	#we need to partition disk.img
	echo "label: dos" | sfdisk disk.img
	echo "2048 ${ROOTSIZE} 0x80 -" | sfdisk disk.img		#this one will be partitioned FAT.
	echo "133120 ${ROOTSIZE} 0x80 -" | sfdisk -a disk.img		#This will have the TAR image.
	
	mkfs.fat -F 16 -n "NETTAPUS" root_fat.img 
	
	echo "[main] Root access is necessary for this next part in order to make a loopback device."
	sudo losetup /dev/loop0 disk.img
	sudo losetup /dev/loop1 root_fat.img
	sudo mount /dev/loop1 /mnt
	sudo cp -r root/* /mnt
	sync 
	
	
	 
	
	#install grub.
	sudo grub-install --root-directory="/mnt" --no-floppy --target=i386-pc --recheck --force --modules="normal search configfile fat tar elf biosdisk part_msdos multiboot" /dev/loop0
	sync
	
	#fat 
	
	sudo umount /mnt
	sync
	sudo dd if=/dev/loop1 of=/dev/loop0 bs=512 seek=2048
	
	
	
	cd "${SYSROOT}"
	tar -cf root.tar *
	mv root.tar ../
	cd ../
	
	
	sudo dd if=root.tar of=/dev/loop0 bs=512 seek=133120
	sync
	
	sudo losetup -d /dev/loop0
	sudo losetup -d /dev/loop1
	rm root.tar
	rm root_fat.img
	
	
	
	echo "[main] Image done. Now you can test it."
	
else 
	echo "[main] The kernel didn't come out as multiboot, can't generate image."
	
fi








