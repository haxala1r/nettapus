# Nettapus
Nettapus is an OS project targeting the x86_64 platform.

# Building
In order to build Nettapus, you need to have these installed:

	- A GCC cross-compiler targeting x86_64.
	- The Netwide Assembler.
	- The GNU binutils package, built to target x86_64.
	- Limine/TomatBoot depending on whether you plan on using BIOS/UEFI.
	- (e2tools)[https://github.com/e2tools/e2tools]
	- GNU make.

If you have these installed, then you should be able to build by running `make`.

Also keep in mind that Nettapus relies on the stivale2 boot protocol. That means
if you'd like to replace the bootlader, a stivale2-compliant bootloader is necessary.

## Running
If you'd like to see Nettapus in action after you've built it,
use `make qemu`, which will simply launch qemu with the newly created disk image.

## Real Hardware
Nettapus works on real hardware as well! You can simply burn the created disk
image to a disk, and then boot off of it. Welcome to Nettapus!

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


# Contributions
If you'd like to contibute to Nettapus, please try to follow these guidelines:

	- Asterisk to the right when declaring pointers. Doing otherwise is a crime.
	- snake_case is preferred, but there *are* cases it just doesn't work well.
	- Divide your code into segments of closely related things with whitespaces and newlines.
	- Divide stuff into different functions whenever possible.
	- Ignore all of the above and write shitty code like you're gonna die tomorrow.

