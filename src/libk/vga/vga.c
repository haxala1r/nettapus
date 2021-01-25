
#include <stdint.h>
#include <vga.h>

uint64_t FB_ADDR;
uint32_t* FB;
uint64_t FB_width	= 0;	/* Amount of columns on the screen. 	*/
uint64_t FB_height 	= 0;	/* Amount of rows on the screen.     	*/
uint64_t FB_bpp		= 0;	/* How many bits to go one pixel right	*/
uint64_t FB_pitch	= 0;	/* How many bytes to go one pixel down	*/


void vga_fill_screen(uint32_t color) {
	for (size_t i = 0 ; i < FB_height; i++) {
		for (size_t j = 0; j < FB_width; j++) {
			FB[j * FB_bpp/32 + i * FB_pitch/4] = color;
		}
	}
};

void vga_put_pixel(uint64_t x, uint64_t y, uint32_t color) {
	FB[x * FB_bpp/32 + y * FB_pitch/4] = color;
};

uint8_t vga_init(uintptr_t fb_addr, uint64_t fb_width, uint64_t fb_height, uint64_t fb_bpp, uint64_t fb_pitch) {
	FB_ADDR 	= fb_addr;
	FB_width 	= fb_width;
	FB_height 	= fb_height;
	FB_bpp 		= fb_bpp;
	FB_pitch 	= fb_pitch;
	
	FB = (uint32_t*) FB_ADDR;
	
	vga_fill_screen(0xFFFFFFFF);
	
	return 0;
};

