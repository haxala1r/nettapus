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
	dd if=/dev/zero of=efi.img 	bs=512 count=$EFISIZE
	sync
	
	#we need to partition disk.img
	echo "label: gpt" | sfdisk disk.img
	
	echo "2048 ${EFISIZE} U -" | sfdisk disk.img
	echo "${ROOTSTART} ${ROOTSIZE} H -" | sfdisk -a disk.img
	
	
	#format root.img. we'll do the "burning" later on.
	mkfs.fat -F 16 -n "NETTAPUS" root.img
	sync
	
	#format efi.img
	mkfs.fat -F 32 -n "SYSTEM" efi.img
	sync
	
	
	
	#now we just need to install grub and move our files to the filesystem we created.
	#I'm using a loopback device for that.
	echo "[main] Root access is necessary for this next part in order to make a loopback device."
	sudo losetup /dev/loop0 disk.img
	sudo losetup /dev/loop1 efi.img
	sudo losetup /dev/loop2 root.img
	sync
	
	#mount the filesystem.
	sudo mount /dev/loop1 /mnt
	
	#install grub.
	# first we need to make a standalone EFI GRUB executable. NOTE: this assumes grub-mkstandalone is installed.
	grub-mkstandalone -O x86_64-efi -o BOOTX64.EFI "boot/grub/grub.cfg=image/boot/grub/grub.cfg"
	sudo mkdir /mnt/EFI
	sudo mkdir /mnt/EFI/BOOT
	sudo cp BOOTX64.EFI /mnt/EFI/BOOT/
	sync
	
	#copy all the files in image/ to the actual image.
	sudo umount /dev/loop1
	
	sudo mount /dev/loop2 /mnt
	sudo cp -r ./image/* /mnt/
	sudo umount /mnt
	
	#now we burn.
	sudo dd if=/dev/loop1 of=/dev/loop0 bs=512 seek=2048
	sync
	sudo dd if=/dev/loop2 of=/dev/loop0 bs=512 seek=$ROOTSTART
	sync
	
	
	sudo losetup -d /dev/loop0
	sudo losetup -d /dev/loop1
	sudo losetup -d /dev/loop2
	
	sudo rm root.img efi.img BOOTX64.EFI
	
	echo "[main] Image done. Now you can test it."
	
else 
	echo "[main] The kernel didn't come out as multiboot, can't generate image."
	
fi








