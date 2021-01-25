# Nettapus
Nettapus is an OS project targeting the x86_64 platform. 

# Building
In order to build Nettapus, you need to have these installed:
	
	- A cross-compiler targeting x86_64.
	- The GNU binutils package, built to target x86_64.
	- Limine/TomatBoot depending on whether you plan on using BIOS/UEFI.
	- Echfs-utils package.
	
Here are the "commands" used in the shell scripts to invoke these tools:
	
	- x86_64-elf-gcc
	- x86_64-elf-as
	- x86_64-elf-ar
	- x86_64-elf-ld
	- limine-install
	- ecfs-utils

If you have these installed, then you should be able to build by executing the build-bios.sh
shell script. (or build-efi.sh if you use UEFI, but that script doesn't exist yet).

Also keep in mind that Nettapus relies on the stivale2 boot protocol, so a stivale2-compliant
bootloader is necessary, and using grub instead of Limine/TomatBoot will not work.

# Goals
One of the biggest goals of Nettapus is to be able to run it on real hardware, *and* 
to be able to use Nettapus without relying on any other OS. 


# To-Dos and Dones
Here are some things that have yet to be done:

	- Get rid of the ugly shell scripts and start using Makefiles.
	
	- Set up interrupts.
	- Set up threading, multitasking, IPC and related things.	
	
	- Add PCIe support. 
	
	- Add proper disk abstraction.
	- Add module loading.
	
	- Add an ATA DMA driver. (as PIO mode is too slow)
	- Add an SATA/AHCI driver.
	- Add USB support.
	- Add keyboard support.
	
	
	- Add support for another file system, because Tar doesn't count.
	- Refactor a huge chunk of the VFS, and adapt it to 64-bits.
	
	- Add UEFI support. I have experimented with this and it works -- but it will only be done after proper VGA support is made.
	- Add GPT support (instead of the traditional MBR).

# Dones
Here are some things from the above list that have been completed:
	
	- Switched to higher half.
	- Switched over to 64-bit mode.
	- Set up paging.
	- Added a proper memory manager, as well as a heap. 
	- Added support for at least one filesystem. (TAR)
	
	- Added PCI support. 
	- Added an ATA PIO driver. 
	
	- Added proper file abstraction.
	
	- Added proper VGA support.


# Contributions
This project is still pretty immature as of now (just as I am). As such, contributions 
are really appreciated, but please keep in mind that this project is made for me to learn about 
OSDev first and foremost. Because of this, unless the added piece of code is extremely clean,
I don't plan on accepting *too much*. 

Though, if you still plan on contibuting, try to follow these guidelines:

	- Asterisk to the right when declaring pointers.
	- snake_case is preferred, but there *are* cases it just doesn't work well.
	- Divide your code into segments of closely related things with whitespaces and newlines.
	- Divide stuff into different functions whenever possible.
	- Ignore all of the above and write shitty code like you're gonna die tomorrow.

