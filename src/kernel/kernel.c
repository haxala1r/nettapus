#if defined(__linux__)
#error "You ain't using a cross-compiler, brave adventurer. Come back when you have one."
#endif

#if !defined(__x86_64__)
#error "Yo, this is supposed to be compiled with a cross-compiler! Well I guess you *could* just comment this out, but... still."
#endif

/* These are the things included with the compiler. */
#include <stdint.h>
#include <stddef.h>

/* The stivale2 boot protocol's header file. */
#include <stivale2.h>

/*
 * These headers are in src/libk/include. If used correctly with the cross-compiler, they
 * should be available.
 */

#include <mem.h>
#include <pci.h>
#include <disk/ide.h>
#include <fs/fs.h>
#include <string.h>
#include <vga.h>
#include <tty.h>
#include <interrupts.h>
#include <task.h>
#include <keyboard.h>



static uint8_t stack[4096 * 2];

struct stivale2_header_tag_framebuffer framebuffer_hdr_tag = {
	.tag = {
		.identifier = STIVALE2_HEADER_TAG_FRAMEBUFFER_ID,
		.next = 0
	},

	.framebuffer_width 	= 1024,
	.framebuffer_height 	= 768,
	.framebuffer_bpp 	= 32
};


__attribute__((section(".stivale2hdr"), used))
struct stivale2_header stivale_hdr = {
	.entry_point = 0,
	.stack = (uintptr_t)stack + sizeof(stack),

	.flags = 0,

	.tags = (uint64_t)&framebuffer_hdr_tag
};



void* get_stivale_header(struct stivale2_struct* s, uint64_t id) {
	struct stivale2_tag* i = (void*)s->tags;
	while (i) {

		if (i->identifier == id) {
			return i;
		}


		i = (void*)i->next;
	}
	return NULL;
};


extern void loadGDT();
void second_task();
int32_t fds[2];
void _start(struct stivale2_struct *hdr) {
	/* We should load our own GDT as soon as possible. */
	loadGDT();

	/*
	 * It is important that we extract all the information from the bootloader
	 * we need before loading our own page tables. This is because when we
	 * load our own page tables, the stivale2 struct will be an invalid address.
	 */


	struct stivale2_struct_tag_framebuffer* fb_hdr_tag;
	fb_hdr_tag = get_stivale_header(hdr, STIVALE2_STRUCT_TAG_FRAMEBUFFER_ID);


	uintptr_t fb_addr = fb_hdr_tag->framebuffer_addr;
	uint64_t fb_width = fb_hdr_tag->framebuffer_width;
	uint64_t fb_height = fb_hdr_tag->framebuffer_height;
	uint64_t fb_bpp = fb_hdr_tag->framebuffer_bpp;
	uint64_t fb_pitch = fb_hdr_tag->framebuffer_pitch;

	struct stivale2_struct_tag_memmap* mm = get_stivale_header(hdr, STIVALE2_STRUCT_TAG_MEMMAP_ID);

	/* We have everything we need. Now initialise the memory manager. */
	init_memory(mm);


	/*
	 * We can initialise the vga driver right away. The console needs some fonts, so we'll
	 * need to initialise the file system etc. before the console is ready.
	 */
	uint64_t fb_len = fb_width * fb_bpp/8 + fb_height * fb_pitch;
	map_memory(fb_addr, 0xFFFFFFFFFB000000, fb_len/0x1000, kgetPML4T());

	krefresh_vmm();		/* Refresh the page tables. */

	vga_init(0xFFFFFFFFFB000000, fb_width, fb_height, fb_bpp, fb_pitch);



	/* Now initialise everything else. The screen will be filled with red on failure.
	 * This "failure report mechanism" is not perfect, and will not even work if an error
	 * occurs *before* VGA is initialised properly, but we don't have much choice as of now.
	 */

	/* First, interrupts. */
	if (init_interrupts()) {
		kpanic();
	}

	/* We can get the scheduler up as well. */
	if (init_scheduler()) {
		kpanic();
	}

	/* PCI. The IDE driver depends on this. */
	if (pci_scan_all_buses()) {
		kpanic();
	}

	/* IDE, a.k.a. ATA. This is currently our only way of accessing a disk. */
	if (init_ide()) {
		kpanic();
	}

	/* The file system drivers. */
	if (fs_init()) {
		kpanic();
	}

	/* The TTY driver takes a file name as its init function's only parameter. This file
	 * contains the fonts used by the driver itself. This is the reason we have to initialise
	 * the FS drivers *before* TTY.
	 */
	if (tty_init("/fonts/lat9-08.psf")) {
		kpanic();
	}

	if (init_kbd()) {
		kpanic();
	}

	kputs("Hello, world!\n");

	/* This is a simple test to see if pipes are working properly.
	 * Here's how it works:
	 * 	The first task continuously writes "Hello there!\n" to the write
	 * 	end of the pipe, and the second task reads from the pipe two bytes
	 * 	at a time and prints it. If everything is working properly,
	 * 	then you should see a continuos stream of the same string repeated
	 * 	over and over again.
	 */

	/* Create the pipe. Currently, when a task is created, the current
	 * task's file descriptors are copied over for this to work.
	 * TODO: That should be changed in the near future.
	 */
	if (pipeu(get_current_task(), fds)) {
		kpanic();
	}

	/* The string to be written. */
	char *str = "Hello there!\n";

	/* Create the second task. */
	create_task(second_task);

	/* The first task repeatedly writes the same string to the pipe. */
	while (1) {
		if (kwrite(fds[1], str, strlen(str)) == -1) {
			kpanic();
		}
	}
}

char buf[16];
void second_task() {
	unlock_scheduler();

	/* The second task repeatedly reads from the pipe, 2 bytes at a time. */
	while (1){
		memset(buf, 0, 16);
		if (kread(fds[0], buf, 2) == -1) {
			kpanic();
		}
		kputs(buf);
	}
}



