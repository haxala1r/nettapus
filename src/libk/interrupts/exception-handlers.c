
#include <interrupts.h>
#include <tty.h>
#include <vga.h>

void kpanic() {
	/* This is a temporary panic function, that fills the entire screen red
	 * and halts.
	 */
	__asm__ volatile ("cli;");
	vga_fill_screen(0x00FF0000);
	
	while (1) {
		__asm__ volatile ("hlt;");
	};
};
