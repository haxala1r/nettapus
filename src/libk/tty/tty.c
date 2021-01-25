#include <stddef.h>
#include <stdint.h>
#include <vga.h>
#include <tty.h>
#include <mem.h>
#include <fs/fs.h>

#define PSF_FONT_MAGIC 0x864ab572

struct PS_font *font_hdr;	/* Pointer to the header of the font file. */
char *font;	/* Pointer to the loaded font file. */

uint32_t foreground_color = 0x909090;
uint32_t background_color = 0x000000;
uint32_t cursorx = 0;
uint32_t cursory = 0;

void putchar(uint16_t c, uint32_t cx, uint32_t cy, uint32_t fg, uint32_t bg) {

	char *glyph = font + font_hdr->glyph_size * c;
	
	uint32_t x, y;
	//uint32_t *dest = FB;// + cx * font_hdr->width * FB_bpp/32 + cy * font_hdr->height * FB_pitch/4;
	
	for (y = 0; y < font_hdr->height; y++) {
		for (x = 0; x < font_hdr->height; x++) {
			if ((glyph[y]) & (1 << x)) {
				vga_put_pixel(cx * font_hdr->width + (8 - x), cy * font_hdr->height + y, fg);				
			} else {
				vga_put_pixel(cx * font_hdr->width + (8 - x), cy * font_hdr->height + y, bg);
			}
		}
	}
	
};

void putc(char c, uint32_t fg, uint32_t bg) {
	if (c == '\n') {
		cursorx = 0;
		cursory++;
		return;
	}
	putchar(c, cursorx++, cursory, fg, bg);
};



void kputs(char *str) {
	while (*str) {
		
		putc(*str, foreground_color, background_color);
		str++;
	}
};

void kputs_color(char *str, uint32_t fg, uint32_t bg) {
	while (*str) {
		putc(*str, fg, bg);
		str++;
	}
};


uint8_t tty_init(char *font_file) {
	/* The only parameter this function takes is the font file. It is the path of the
	 * font file on the root file system (or mount points). The fonts will be loaded
	 * from that file.
	 */
	 
	int32_t fd = kopen(font_file, 0);
	font_hdr = kmalloc(sizeof(struct PS_font));
	 
	if (kread(fd, font_hdr, 32) != 32) {
		return 1;
	}
	 
	if (font_hdr->magic != PSF_FONT_MAGIC) {
		 return 1;
	}
	if (font_hdr->headersize != 32) {
		return 1;
	}
	
	
	font = kmalloc(font_hdr->glyph_count * font_hdr->glyph_size);
	if (kread(fd, font, font_hdr->glyph_count * font_hdr->glyph_size) != (int32_t)(font_hdr->glyph_count * font_hdr->glyph_size)) {
		return 1;
	}
	 
	return 0; 
};


