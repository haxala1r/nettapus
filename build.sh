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




echo "[main] builds done. Generating image file."




#go to the original place, and generate an ISO image of the image directory.
cd "${SYSROOT}/.."
if grub-file --is-x86-multiboot "${IMAGEDIR}/boot/kernel.bin";
then
	#if it is multiboot, then everything should work.
	#now we gotta create the disk image.
	dd if=/dev/zero of=disk.img bs=512 count=$DISKSIZE
	dd if=/dev/zero of=root.img bs=512 count=$ROOTSIZE
	sync
	
	#we need to partition disk.img
	echo "label: dos" | sfdisk disk.img
	echo "label_id: 0x0A" | sfdisk disk.img
	echo "2048 ${ROOTSIZE} 0x06 *" | sfdisk disk.img
	
	#format root.img, and burn it to disk.img's first partition.
	mkfs.fat -F 16 -n "NETTAPUS" root.img
	dd if=root.img of=disk.img bs=512 seek=2048
	sync
	rm root.img		#we can now get rid of this file.
	
	#now we just need to install grub and move our files to the filesystem we created.
	#I'm using a loopback device for that.
	echo "[main] Root access is necessary for this next part in order to make a loopback device."
	sudo losetup /dev/loop0 disk.img
	
	sudo losetup /dev/loop1 disk.img -o 1048576
	sync
	
	#mount the filesystem.
	sudo mount /dev/loop1 /mnt
	
	#install grub.
	sudo grub-install --root-directory=/mnt/ --no-floppy --target=i386-pc --recheck --force --modules="normal fat elf biosdisk part_msdos multiboot" /dev/loop0
	sync
	
	#copy all the files in image/ to the actual image.
	sudo cp -r ./image/* /mnt/
	sudo umount /mnt
	sudo losetup -d /dev/loop0
	sudo losetup -d /dev/loop1
	
	
	
	echo "[main] Image done. Now you can test it."
	
else 
	echo "[main] The kernel didn't come out as multiboot, can't generate image."
	
fi








