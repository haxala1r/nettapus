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
image to a disk, and then boot off of it.

Keep in mind, however, that nettapus isn't capable of reading from any disk that
isn't an IDE disk (yet!). Nettapus also depends on the existance of such a device,
because at boot time it attempts to read from a font file (you can find it in
root/fonts/) and, in the very near future, nettapus will attempt to read from a
configuration file. The OS will iterate through every single valid filesystem it
can drive (only ext2 at the time) until it finds a filesystem with ALL of the
aforementioned files. If none are found, the kernel panics.

Since Nettapus needs these files to exist on an IDE device somewhere, a USB stick
or a DVD will not work. Instead, you are advised to either a) Burn the resulting
disk.img file to the hard drive or b) If you don't want to disrupt another system,
make a new partition (on an IDE drive) and burn the ext2.img image file to that
partition (you could also copy all files in the root directory to the newly created
partition if you want a different size).

# Contributions
See CONTRIBUTING.md

