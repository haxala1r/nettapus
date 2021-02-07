
# These are the tools we use when building.
# In order to use a different target, change these accordingly.
CC := x86_64-elf-gcc
LD := x86_64-elf-ld
AR := x86_64-elf-ar
AS := nasm

# This is the emulator to run the image on. Currently we use QEMU.
EMUL := qemu-system-x86_64 -cpu qemu64 

KERNELFLAGS := -O2 -std=gnu99 -Wall -Wextra -mcmodel=large -fno-pic -fno-stack-protector -mno-red-zone \
	-ffreestanding -nostdlib --sysroot="src/" -isystem="/libk/include/" 

# Uncomment this while debugging. 
KERNELFLAGS += -DDEBUG

KERNELLINK := -ffreestanding -lgcc  -nostdinc  -nostdlib -no-pie -static -mcmodel=kernel \
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
	@dd if=/dev/zero of=$(IMG) bs=1M count=128
	@echo -e "2048 196608 0x80 *\n198656 63488 0x06 -" | sfdisk $(IMG)
	@dd if=/dev/zero of=fat.img bs=512 count=196608 	# This is the first partition (FAT32) for the bootloader. 
	@mkfs.fat -F 32 -n "NETTAPUS" fat.img
	@mcopy -s -i fat.img root/boot/limine.cfg ::/
	@mcopy -s -i fat.img root/* ::/
	@mcopy -s -i fat.img root/boot/kernel.elf ::/kernel.elf # Limine breaks if we don't do this. No idea why.
	@dd if=fat.img of=$(IMG) bs=512 seek=2048 conv=notrunc
	@sync
	@dd if=/dev/zero of=fat.img bs=512 count=63488		# This is the second partition (FAT16).
	@mkfs.fat -F 16 -n "NETTAPUS" fat.img
	@mcopy -s -i fat.img root/* ::/
	@dd if=fat.img of=$(IMG) bs=512 seek=198656 conv=notrunc
	@limine-install $(IMG)					# Install limine.
	@$(RM) fat.img

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


clean:
	@$(RM) $(KERNELOBJ) $(LIBKOBJ) "root/boot/kernel.elf"
	@$(RM) *.o *.a


