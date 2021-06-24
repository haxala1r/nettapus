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
#include <disk/disk.h>
#include <pci.h>
#include <fs/fs.h>
#include <string.h>
#include <vga.h>
#include <tty.h>
#include <interrupts.h>
#include <task.h>
#include <keyboard.h>
#include <config.h>
#include <err.h>

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



void *get_stivale_header(struct stivale2_struct *s, uint64_t id) {
	struct stivale2_tag* i = (void*)s->tags;
	while (i) {

		if (i->identifier == id) {
			return i;
		}


		i = (void*)i->next;
	}
	return NULL;
}


extern void loadGDT();

void _start(struct stivale2_struct *hdr) {
	/* We will turn on interrupts as well as task switching etc. when
	 * everything is properly initialised.
	 */
	__asm__("cli;");

	/* We should load our own GDT as soon as possible. */
	loadGDT();
	init_serial();
	/*
	 * It is important that we extract all the information from the bootloader
	 * we need before loading our own page tables.
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
	if (init_memory(mm)) {
		serial_puts("Memory init failed.\r\n");
		kpanic();
	}
	serial_puts("Memory manager OK\r\n");


	/*
	 * We can initialise the vga driver right away. The console needs some fonts, so we'll
	 * need to initialise the file system etc. before the console is ready.
	 */
	uint64_t fb_len = fb_width * fb_bpp/8 + fb_height * fb_pitch;
	map_memory(fb_addr, 0xFFFFFFFFFB000000, fb_len/0x1000 * 2, kgetPML4T(), 0);

	krefresh_vmm();		/* Refresh the page tables. */
	if (vga_init(0xFFFFFFFFFB000000, fb_width, fb_height, fb_bpp, fb_pitch)) {
		serial_puts("VGA init failed.\r\n");
		kpanic();
	}
	serial_puts("VGA OK\r\n");

	/* Now initialise everything else. The screen will be filled with red on failure.
	 * This "failure report mechanism" is not perfect, and will not even work if an error
	 * occurs *before* VGA is initialised properly, but we don't have much choice as of now.
	 */

	/* First, interrupts. */
	if (init_interrupts()) {
		serial_puts("Interrupt setup failed.\r\n");
		kpanic();
	}

	/* We can get the scheduler up as well. */
	if (init_scheduler()) {
		serial_puts("Scheduler failed to initialise.\r\n");
		kpanic();
	}
	serial_puts("Scheduler OK\r\n");

	/* PCI. The IDE driver depends on this. */
	if (pci_scan_all_buses()) {
		serial_puts("PCI failed initialise.\r\n");
		kpanic();
	}
	serial_puts("PCI OK\r\n");

	/* Disk drivers. */
	if (init_disk()) {
		serial_puts("Disk driver failed to initialise.\r\n");
		kpanic();
	}
	serial_puts("Disk OK\r\n");

	/* The file system drivers. */
	if (!init_fs()) {
		serial_puts("FS drivers failed to initialise and/or no FS was found.\r\n");
		kpanic();
	}
	serial_puts("FS & VFS OK\r\n");

	/* Read the configuration file, and load settings from it. */
	if (reload_config("/boot/nettapus.cfg")) {
		serial_puts("Config file not found.\r\n");
	} else {
		serial_puts("Config file loaded.\r\n");
		#ifdef DEBUG
		config_put_variables();
		#endif
	}

	/* TTY. This will ask for a config variable name font_file. */
	if (tty_init()) {
		serial_puts("TTY failed.\r\n");
		kpanic();
	}
	serial_puts("TTY OK\r\n");
	__asm__("sti;");

	/* We need to create the stdin and stdout fds for the first task.
	 *
	 * This will hold the fds for the pipe we created. See, the std output file
	 * descriptor is actually a pipe, and another task simply reads from this pipe
	 * and outputs it.
	 * stdin is returned by init_kbd.
	 */
	int32_t stdpipe[2];
	kpipeu(get_current_task(), stdpipe);
	struct file_descriptor *wfd = vfs_find_fd(get_current_task(), stdpipe[1]); /* We're copying the write end. */

	struct file_descriptor *nfd = kmalloc(sizeof(*nfd));

	if (nfd == NULL) {
		serial_puts("OUT OF MEMORY\r\n");
		kpanic();
	}

	memcpy(nfd, wfd, sizeof(*nfd));
	nfd->next = NULL;
	nfd->fd = 1;


	/* Increase the referance count of the vnode. */
	struct file_vnode *node = nfd->node;
	node->streams_open++;

	/* Now create the file descriptor for standard input. */
	struct file_descriptor *rfd = init_kbd();
	if (rfd == NULL) {
		serial_puts("KBD error\r\n");
		kpanic();
	}
	serial_puts("KBD OK\r\n");
	rfd->next = nfd;
	rfd->fd = 0;

	kclose(stdpipe[1]); /* The write end is useless for the current task. */

	/* The first program to be executed. */
	char *init = config_get_variable("init");

	uintptr_t entry_addr;
	p_map_level4_table *pml4t = load_elf(init, &entry_addr);
	if (pml4t == NULL) {
		kputs("Could not find file '");
		kputs(init);
		kputs("'\n");
		__asm__("cli;hlt;");
	}

	lock_scheduler();
	struct task *t = create_task((void (*)())entry_addr, pml4t, 3, NULL);
	t->fds = rfd;
	unlock_scheduler();

	while (1) {
		char buf[2] = {'\0', '\0'};
		if (kread(stdpipe[0], buf, 1) != 1) {
			serial_puts("kread() failed.\r\n");
			continue;
		}
		kputs(buf);
		__asm__("hlt;");
	}
}

