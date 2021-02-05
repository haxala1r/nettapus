# Nettapus
Nettapus is an OS project targeting the x86_64 platform. 

# Building
In order to build Nettapus, you need to have these installed:
	
	- A GCC cross-compiler targeting x86_64.
	- The Netwide Assembler.
	- The GNU binutils package, built to target x86_64.
	- Limine/TomatBoot depending on whether you plan on using BIOS/UEFI.
	- Echfs-utils package.
	- MTools.
	- GNU make.

If you have these installed, then you should be able to build by executing `make`.

Also keep in mind that Nettapus relies on the stivale2 boot protocol, so if you'd like
to replace the bootlader, a stivale2-compliant bootloader is necessary, 
and using GRUB instead of Limine/TomatBoot will not work.

## Testing
If you'd like to see Nettapus in action after you've built it, 
use `make qemu`, which will simply launch qemu with the newly created disk image.


# To-Dos
Here are some things that have yet to be done:

	- Add more exception handlers.
	- Improve threading, multitasking, IPC etc, and change the scheduling algorithm.
	- Add PCIe support. 
	- Add better disk abstraction.
	- Add module loading.
	- Add an ATA DMA driver.
	
	- Add UEFI support.
	- Add GPT support.

# Dones
If you want to have an idea of what has been done on Nettapus so far, or what it looks like,
here are some things from the above list that have been completed:
	
	- Switched to higher half.
	- Switched over to 64-bit mode.
	- Set up paging.
	- Added a proper memory manager, as well as a heap. 
	- Added USTAR support.
	- Added FAT16 support.
	- Added PCI support. 
	- Added an ATA PIO driver. 
	- Added semi-proper file abstraction.	
	- Added proper VGA support.
	- Added keyboard support.
	- Set up interrupts to a minimal degree.
	- Added a basic scheduler.
	
# Contributions
If you'd like to contibute to Nettapus, please try to follow these guidelines:

	- Asterisk to the right when declaring pointers. Doing otherwise is a crime.
	- snake_case is preferred, but there *are* cases it just doesn't work well.
	- Divide your code into segments of closely related things with whitespaces and newlines.
	- Divide stuff into different functions whenever possible.
	- Ignore all of the above and write shitty code like you're gonna die tomorrow.

