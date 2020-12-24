# nettapus
A beginner's OS project. Made to learn about OS development. 
As of now, it does not contain anything of interest or use. 
You can use the build script to get an image you can run on qemu, as well as real hardware. 
The "qemu.sh" shell script creates a loopback device and runs this image, though you have to run it as root.
This is to make the QEMU environment as close to real hardware as possible.
The clean script will clean everything. Quite simple, huh?

# Dependencies
This project requires you to have a i686 cross compiler, assembler, archiver 
and linker (I use gcc, as, ar and ld respectively) to build. 
Otherwise you can build it yourself as well, of course, by changing the necessary variables in "config.sh" shell script.
You also need to have grub and xorriso packages.installed, since the build script uses "grub-mkrescue" to create the disk image. 

# Goals
First of all, I'm currently trying my best to ensure that this OS will run both on QEMU and real hardware. 
My current goal is to make sure that this OS is actually usable after I'm done with it. 
Some more specific and short-term "goals":

	- Switch to higher half. Done.
	- Set up paging. Done.
	- Add a proper memory manager, as well as a heap. Done.
	- Set up interrupts.
	- Set up threading, multitasking, IPC and related things.	
	- Add PCI support. Done.
	- Add an ATA PIO driver. Done.
	- Add an ATA DMA driver.
	- Add a support for at least one filesystem.
	
	- Make a bootloader, instead of relying solely on GRUB to do everything.

Hopefully more "goals" and "to-dos" will appear soon.

# Contributions
Yeah, this project's still pretty immature as of now. 
Any contribution is really appreciated, but please make sure you use the same
coding style as I do. 
