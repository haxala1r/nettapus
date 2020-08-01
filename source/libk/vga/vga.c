
#include <stdint.h>
#include <vga.h>







uint8_t vga_entry_color(enum VGA_color fg, enum VGA_color bg) {
	return fg | bg << 4;
}

uint16_t vga_entry(uint8_t uc, uint8_t color) {
	return (uint16_t) uc | (uint16_t) color << 8; 
}





