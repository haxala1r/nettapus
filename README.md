Happy new year, everybody!

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


# To-Dos and Dones
Here are some things that I have yet to do, and things I have actually done so far.

	- Switch to higher half. Done.
	- Set up paging. Done.
	- Add a proper memory manager, as well as a heap. Done.
	- Set up interrupts.
	- Set up threading, multitasking, IPC and related things.
	
	- Add PCI support. Done.
	- Add PCIe support. 
	- Add an ATA PIO driver. Done.
	- Add an ATA DMA driver.
	- Add an SATA/AHCI driver.
	- Add USB support.
	- Add keyboard support.
	- Add support for at least one filesystem. Done -- Tar.
	- Add support for another file system, because Tar doesn't count.
	
	- Add UEFI support. I have experimented this -- this will only be done after proper VGA support is made.
	- Add proper VGA support.
	- Add GPT support (instead of the traditional MBR).

# Contributions
Yeah, this project's still pretty immature as of now. 
Any contribution is really appreciated, but please make sure you use the same
coding style as I do, which mainly consists of:

	- Asterisk to the left when declaring pointers.
	- snake_case is preferred, but there are cases it just doesn't work well.
	- Divide your code into segments of closely related things with whitespaces and newlines.
	- Divide stuff into different functions whenever possible.
	- Ignore all of the above and write shitty code like you're gonna die tomorrow.
	

