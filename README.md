# Nettapus
Nettapus is an OS project targeting the x86_64 platform.

# Current State
Nettapus is currently in *very* early development.

## VFS
Nettapus is currently almost-fully capable of reading/writing files to disk.
There is no user support yet. Userspace programs can be run though, it is only
the concept of UIDs, GIDs, permissions and similar stuff that is missing.

There is some more stuff that needs to be added to the vfs before Nettapus can be
fully functional (mostly involving file creation and deletion, and as I said,
permission-checking).

## Scheduler
The scheduler is also almost-fully-functional, although it is really inefficient and will
be replaced (currently it is a round-robin scheduler, and no sleeping).

Semaphores (and by extension mutexes) are fully functional, as well as queues.

## libc
Currently, only a stub libc is provided. I haven't quite decided what to do about
libc, but chances are I'll probably port musl.

## Userspace
Currently, a very minimal userspace environment is provided (consisting of `sh`,
`ls` and `cat`, all my implementations). Very few system calls are provided.

More system calls (and hopefully a proper libc and shell) will be ported soon.

## Build system
Nettapus relies on `make` as a build system, and also plain shell scripts.
This however, ended up way uglier than I imagined when switching to make, so
it will be changed soon.

# Building
In order to build Nettapus, you need to have these installed:

	- A GCC cross-compiler targeting x86_64.
	- The Netwide Assembler.
	- The GNU binutils package, built to target x86_64.
	- Limine/TomatBoot depending on whether you plan on using BIOS/UEFI.
	- (e2tools)[https://github.com/e2tools/e2tools] - this is necessary!
	- GNU make.

If you have these installed, then you should be able to build by running `make`.

Also keep in mind that Nettapus relies on the stivale2 boot protocol.

## Running
If you'd like to see Nettapus in action after you've built it (or downloaded the disk image),
use `make qemu`, which will simply launch qemu with the newly created disk image.

## Real Hardware
Nettapus works on real hardware as well! You can simply burn the created disk
image to a disk, and then boot off of it.

Since Nettapus needs these files to exist on an IDE device somewhere, a USB stick
or a DVD will not work. Instead, you are advised to either a) Burn the resulting
disk.img file to the hard drive or b) If you don't want to disrupt another system,
make a new partition (on an IDE drive) and burn the ext2.img image file to that
partition (you could also copy all files in the root directory to the newly created
partition if you want a different size).

USB support will be added later on in development.

After booting, make sure to look around, and maybe `cat hello.txt` and stuff.

# Other things
## Testing
Nettapus compiles perfectly with all optimizations supported by GCC. It also
compiles & runs without issue when compiled with `-fsanitize=undefined` (all
sanitizers are set to panic()), so you can rest assured that there are no major
UBs.

I do plan on adding tests, but they're kind of difficult to test a whole OS. I
will probably add at least a program that stress-tests the system calls though.

# Contributions
See CONTRIBUTING.md

