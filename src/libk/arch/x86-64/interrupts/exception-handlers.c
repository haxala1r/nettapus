
#include <interrupts.h>
#include <err.h>
#include <vga.h>

void kpanic() {
	/* This is a temporary panic function, that fills the entire screen red
	 * and halts. In the future, it should probably do a couple other things,
	 * like inform the user of the reason behind the panic, shut down other CPUs
	 * (when SMP is added), etc. etc.
	 */
	__asm__ volatile ("cli;");
	vga_fill_screen(0x00FF0000);
	serial_puts("\r\nPANIC!\r\n at the disco");
	while (1) {
		__asm__ volatile ("hlt;");
	}
}
