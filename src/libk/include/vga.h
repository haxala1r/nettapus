#ifndef _VGA_H
#define _VGA_H 1


#ifdef __cplusplus
extern "C" {
#endif



//#include "../vga/vga.c"
enum VGA_color {
	VGA_BLACK = 0,
	VGA_BLUE = 1,
	VGA_GREEN = 2,
	VGA_CYAN = 3,
	VGA_RED = 4,
	VGA_MAGENTA = 5,
	VGA_BROWN = 6,
	VGA_LIGHT_GREY = 7,
	VGA_DARK_GREY = 8,
	VGA_LIGHT_BLUE = 9,
	VGA_LIGHT_GREEN = 10,
	VGA_LIGHT_CYAN = 11,
	VGA_LIGHT_RED = 12,
	VGA_LIGHT_MAGENTA = 13,
	VGA_LIGHT_BROWN = 14,
	VGA_WHITE = 15,
};


uint8_t vga_entry_color(enum VGA_color, enum VGA_color);
uint16_t vga_entry(uint8_t, uint8_t);



#ifdef __cplusplus
}
#endif





#endif 

