#include <err.h>
#include <io.h>
#include <string.h>

#define PORT 0x3f8

size_t init_serial() {
	outb(PORT + 1, 0x00);    // Disable all interrupts
	outb(PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
	outb(PORT + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
	outb(PORT + 1, 0x00);    //                  (hi byte)
	outb(PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
	outb(PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
	//outb(PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
	outb(PORT + 4, 0x1E);    // Set in loopback mode, test the serial chip
	outb(PORT + 0, 0xAE);    // Test serial chip (send byte 0xAE and check if serial returns same byte)

	// Check if serial is faulty (i.e: not same byte as sent)
	if(inb(PORT + 0) != 0xAE) {
		return 1;
	}

	outb(PORT + 4, 0x0F);
	return 0;
}

size_t is_transmit_empty() {
	return inb(PORT + 5) & 0x20;
}

void serial_putc(char c) {
	while (is_transmit_empty() == 0);

	outb(PORT, c);
}

void serial_puts(char *str) {
	if (str == NULL) { return; }
	while (*str) {
		serial_putc(*(str++));
	}
}

void serial_putx(uint64_t val) {
	char str[20];
	memset(str, 0, 20);
	xtoa(val, str);
	serial_puts(str);
}
