


//#ifdef __STDC_HOSTED__
//#error "O ho ho, brave adventurer. Surely, you jest when you try to compile a FUCKING KERNEL FOR A HOSTED ENVIRONMENT."
//#endif

#if defined(__linux__)
#error "You ain't using a cross-compiler, brave adventurer. Come back when you have one."
#endif 

#if !defined(__x86_64__)
#error "Oi, this is supposed to be compiled with a cross-compiler! Well I guess you *could* just comment this out, but... still."
#endif

//stuff the compiler automatically has.
#include <stdint.h>
#include <stddef.h>


#include <stivale2.h>

/* 
 * These headers are in src/libk/include. If used correctly with the cross-compiler, they
 * should be available 
 */

#include <mem.h>
#include <pci.h>
#include <disk/ide.h>
#include <fs/fs.h>
#include <string.h>
#include <vga.h>
#include <tty.h>

extern char kernel_b[];



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







void _start(struct stivale2_struct *hdr) {
	/* 
	 * It is important that we extract all the information we need before 
	 * loading our own page tables. This is because that when we load our own
	 * page tables, the stivale2 struct will be an invalid address.
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
	map_memory(fb_addr, 0xFFFFFFFF00000000 + fb_addr, fb_len/0x1000, kgetPML4T());	
	
	vga_init(0xFFFFFFFF00000000 + fb_addr, fb_width, fb_height, fb_bpp, fb_pitch);
	
	/* Now initialise everything else. They all make a unique color if they fail. */
	
	/* PCI. IDE driver depends on this. */
	if (pci_scan_all_buses()) {
		vga_fill_screen(0x00FF0000);
		while (1) { asm volatile("hlt;"); };
	}
	
	/* IDE. This is currently our only way of accessing a disk. */
	if (init_ide()) {
		vga_fill_screen(0x0000FF00);
		while (1) { asm volatile("hlt;"); };
	}
	
	/* The file system drivers. */
	if (fs_init()) {
		vga_fill_screen(0x000000FF);
		while (1) { asm volatile("hlt;"); };
	}
	
	if (tty_init("fonts/lat9-08.psf")) {
		vga_fill_screen(0xFFFF00);
		while (1) { asm volatile("hlt;"); };
	}
	
	kputs("Hello, VGA world!\n");
	
	int32_t fd = kopen("hello.txt", 0);
	char *buf = kmalloc(32);
	memset(buf, 0, 32);
	
	kread(fd, buf, 18);
	
	kputs(buf);
	
	while (1) {
		asm ("hlt;");
	}
}


