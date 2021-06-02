
# These are the tools we use when building.
# In order to use a different target, change these accordingly.
CC := x86_64-elf-gcc
LD := x86_64-elf-ld
AR := x86_64-elf-ar
AS := nasm

# This is the emulator to run the image on. Currently we use QEMU.
EMUL := qemu-system-x86_64 -cpu qemu64 

KERNELFLAGS := -O3 -std=c99 -Wall -Wextra -mcmodel=large -fno-pic -fno-stack-protector -mno-red-zone \
	-ffreestanding -nostdlib --sysroot="./src/" -isystem="/libk/include/" -pedantic -mno-sse -msoft-float

# Uncomment this while debugging. 
#KERNELFLAGS += -DDEBUG -fsanitize=undefined

KERNELLINK := -ffreestanding -lgcc  -nostdinc  -nostdlib -static -mcmodel=kernel \
	-z max-page-size=0x1000

LIBKDIR := src/libk/
KERNELDIR := src/kernel/


LIBKASM := $(shell find $(LIBKDIR) -type f -name "*.asm")
LIBKC	:= $(shell find $(LIBKDIR) -type f -name "*.c")
KERNELASM := $(shell find $(KERNELDIR) -type f -name "*.asm")
KERNELC := $(shell find $(KERNELDIR) -type f -name "*.c")

LIBKOBJ  := $(patsubst %.c,%.o,$(LIBKC)) $(patsubst %.asm,%.o,$(LIBKASM))
KERNELOBJ := $(patsubst %.c,%.o,$(KERNELC)) $(patsubst %.asm,%.o,$(KERNELASM))

IMG := disk.img

.PHONY: all clean bios qemu

all: bios

bios: kernel
	@dd if=/dev/zero of=$(IMG) bs=1M count=32
	@echo -e "2048 63488 0x80 *\n" | sfdisk $(IMG)
	# The only partition (EXT2)
	@dd if=/dev/zero of=ext2.img bs=512 count=63488
	@mkfs.ext2 ext2.img
	@./ext2.sh
	@dd if=ext2.img of=$(IMG) bs=512 seek=2048 conv=notrunc
	@sync
	@limine-install $(IMG)					# Install limine.
	@$(RM) temp.img
	@echo "Build done, you can run using 'make qemu'"

kernel: $(KERNELOBJ) libk.a
	@$(CC) $(KERNELLINK) -T "src/kernel/link.ld" -o "root/boot/kernel.elf" $?

libk.a: $(LIBKOBJ) 
	@$(AR) rcs "libk.a" $?

# The rule to compile C files.
%.o: %.c Makefile
	@$(CC) $(KERNELFLAGS) -c $< -o $@

# The rule to assemble assembly files.
%.o: %.asm Makefile
	@$(AS) -f elf64 $< -o $@

qemu: 
	@$(EMUL) -drive file=$(IMG),format=raw

qemu-debug:
	@$(EMUL) -drive file=$(IMG),format=raw -s -S

clean:
	@$(RM) $(KERNELOBJ) $(LIBKOBJ) "root/boot/kernel.elf"
	@$(RM) *.o *.a *.img


