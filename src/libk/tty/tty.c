#include <stddef.h>
#include <stdint.h>
#include <vga.h>
#include <tty.h>
#include <mem.h>
#include <fs/fs.h>
#include <task.h>

#define PSF_FONT_MAGIC 0x864ab572



struct PS_font *font_hdr;	/* Pointer to the header of the font file. */
char *font;	/* Pointer to the loaded font file. */

/* These are the default colors for the terminal. */
uint32_t foreground_color = 0x909090;
uint32_t background_color = 0x000000;

/* Cursor position, represents where we are on the screen, and where a new character will be put.*/
uint32_t cursorx = 0;
uint32_t cursory = 0;

void putchar(uint16_t c, uint32_t cx, uint32_t cy, uint32_t fg, uint32_t bg) {

	char *glyph = font + font_hdr->glyph_size * c;
	
	uint32_t pitch = vga_get_fb_pitch();
	uint32_t bpp = vga_get_fb_bpp();
	uint32_t x, y;
	uint32_t *dest = vga_get_fb() + (cx * font_hdr->width * bpp/32) + (cy * font_hdr->height * pitch/4);
	
	for (y = 0; y < font_hdr->height; y++) {
		for (x = 0; x < font_hdr->width; x++) {
			if ((glyph[y]) & (1 << x)) {		
				dest[(8 - x) * bpp/32 + y * pitch/4] = fg;
			} else {
				dest[(8 - x) * bpp/32 + y * pitch/4] = bg;
			}
		}
	}
	return;
};


void scroll(uint32_t bg) {
	/* Scrolls down by one character. */
	
	uint32_t height = vga_get_fb_height();
	uint32_t pitch = vga_get_fb_pitch();
	
	uint32_t *fb = vga_get_fb();
	
	/* Move everything except the last row up by one character. */
	for (uint32_t i = 0; i < ((height * pitch)/4 - pitch * font_hdr->height / 4); i++) {
		fb[i] = fb[i + pitch * font_hdr->height / 4 ];
	}
	
	/* Fill the last row with the requested color. */
	for (uint32_t i = ((height * pitch)/4 - pitch * font_hdr->height / 4); i < (height * pitch)/4 ; i++) {
		fb[i] = bg;
	}
	
	return;
};


void putc(char c, uint32_t fg, uint32_t bg) {
	/* Outputs a character to the screen. This is a wrapper around putchar, except it takes
	 * care of scrolling as well. 
	 */
	
	/* Handle newline. */
	if (c == '\n') {
		cursorx = 0;
		cursory++;
		if (cursory >= (vga_get_fb_height() / font_hdr->height)) {
			scroll(bg);
			cursory--;
		}
		return;
	}
	
	/* Handle backspace */
	if (c == '\b') {
		if (cursorx != 0) {
			cursorx--;
		}
		putchar(' ', cursorx, cursory, fg, bg);
		return;
	}
	
	/* Handle cases where the end of line has been reached. */
	if (cursorx >= (vga_get_fb_width() / font_hdr->width + 1)) {
		cursorx = 0;
		cursory++;
		if (cursory >= (vga_get_fb_height() / font_hdr->height)) {
			scroll(bg);
			cursory--;
		}
		return;
	}
	
	
	putchar(c, cursorx++, cursory, fg, bg);
	
};

void kput_data(char *data, uint64_t count) {
	for (uint64_t i = 0; i < count ; i++) {
		putc(data[i], foreground_color, background_color);
	}
};

void kputs_color(char *str, uint32_t fg, uint32_t bg) {
	while (*str) {
		putc(*str, fg, bg);
		str++;
	}
};

void kputs(char *str) {
	/* To avoid race conditions, the scheduler needs to be locked until the string is
	 * properly displayed. */
	lock_scheduler();
	
	kputs_color(str, foreground_color, background_color);
	
	unlock_scheduler();
};

void kputx(uint64_t num) {
	/* This is a simple wrapper. It simply turns the number into a string, and prints that. */
	lock_scheduler();
	
	char str[20] = {};
	xtoa(num, str);
	kput_data(str, 18);
	
	unlock_scheduler();
};


uint8_t tty_init(char *font_file) {
	/* The only parameter this function takes is the font file. It is the path of the
	 * font file on the root file system (or mount points). The fonts will be loaded
	 * from that file.
	 * 
	 * This function can be called multiple times, because the only thing it does is
	 * load a font file.
	 */
	
	if (font_hdr != NULL) {
		kfree(font_hdr);
	}
	if (font != NULL) {
		kfree(font);
	}
	
	
	int32_t fd = kopen(font_file, 0);
	font_hdr = kmalloc(sizeof(struct PS_font));
	
	/* Read font file header, and store it. */ 
	if (kread(fd, font_hdr, 32) != 32) {
		return 1;
	}
	 
	if (font_hdr->magic != PSF_FONT_MAGIC) {
		 return 1;
	}
	if (font_hdr->headersize != 32) {
		return 1;
	}
	
	/* Read the fonts, and store them. */
	font = kmalloc(font_hdr->glyph_count * font_hdr->glyph_size);
	if (kread(fd, font, font_hdr->glyph_count * font_hdr->glyph_size) != (int32_t)(font_hdr->glyph_count * font_hdr->glyph_size)) {
		return 1;
	}
	
	kclose(fd);
	
	return 0; 
};


