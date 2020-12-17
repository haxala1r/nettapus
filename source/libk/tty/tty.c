#include <stddef.h>
#include <stdint.h>
#include <vga.h>
#include <tty.h>

const size_t VGA_WIDTH = 80;
const size_t VGA_HEIGHT = 25;







struct {
	size_t row;
	size_t column;
	uint8_t color;
	uint16_t entries[25][80];
	uint16_t *buff;
} terminal;

void init_terminal() {
	terminal.row = 0;
	terminal.column = 0;
	terminal.buff = (uint16_t*) 0xC00B8000;
	terminal.color = vga_entry_color(VGA_LIGHT_GREY, VGA_BLACK);
	
	for (size_t i = 0; i < VGA_HEIGHT; i++) {
		for (size_t i2 = 0; i2 < VGA_WIDTH; i2++) {
			size_t index = i * VGA_WIDTH + i2;
			terminal.buff[index] = vga_entry(' ', terminal.color);
		}
	}
	
	
}


void terminal_printscreen(uint16_t *entries) {
	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			terminal.buff[y * VGA_WIDTH + x] = entries[y * VGA_WIDTH + x];
		}
	}
}



void putentry_at(uint8_t c, uint8_t color, size_t x, size_t y) {
	if (!(x < VGA_WIDTH) || !(y < VGA_HEIGHT)) {
		return;
	}
	size_t index = (VGA_WIDTH) * y + x;
	terminal.buff[index] = vga_entry(c, color);
	terminal.entries[y][x] = vga_entry(c, color);
}

void terminal_scroll() {
	for (size_t iy = 0; iy < VGA_HEIGHT-1; iy++) {
		for (size_t ix = 0; ix < VGA_WIDTH; ix++) {
			terminal.entries[iy][ix] = terminal.entries[iy + 1][ix];
		}
	}
	for (size_t ix = 0; ix < VGA_WIDTH; ix++) {
		terminal.entries[VGA_HEIGHT-1][ix] = vga_entry(' ', terminal.color);
	}
	terminal_printscreen((uint16_t *)terminal.entries);
	terminal.row--;
}

void terminal_putc(uint8_t c) {
	if (c == '\n') {
		terminal.column = 0;
		if (++terminal.row >= VGA_HEIGHT) {
			terminal_scroll();
		}
		return;
	}
	putentry_at(c, terminal.color, terminal.column, terminal.row);
	if (++terminal.column >= VGA_WIDTH) {
		terminal.column = 0;
		if (++terminal.row >= VGA_HEIGHT) {
			terminal_scroll();
		}
	} 
	
}

void terminal_put(uint8_t *data, size_t size) {
	for (size_t i = 0; i < size; i++) {
		terminal_putc(data[i]);
	}
}


void terminal_puts(char *s) {
	terminal_put(s, strlen(s));
}



