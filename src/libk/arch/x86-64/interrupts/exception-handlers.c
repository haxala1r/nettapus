
#include <interrupts.h>
#include <err.h>
#include <vga.h>

void kpanic(void) {
	/* This is a temporary panic function, that fills the entire screen red
	 * and halts. In the future, it should probably do a couple other things,
	 * like inform the user of the reason behind the panic, shut down other CPUs
	 * (when SMP is added), etc. etc.
	 */
	__asm__ volatile ("cli;");
	serial_puts("\r\nPANIC!\r\n at the disco");
	vga_fill_screen(0x00FF0000);

	while (1) {
		__asm__ volatile ("hlt;");
	}
}

void exception_gpf_handler(uint64_t addr) {
	serial_puts("GPF Handler was called ");
	serial_putx(addr);
	serial_puts(".\r\n");
	__asm__ volatile ("cli;hlt;");
}

void exception_ud_handler(void) {
	serial_puts("Invalid Opcode handler was called.\r\n");
}

void exception_ts_handler(void) {
	serial_puts("Invalid TSS handler was called.\r\n");
}

void exception_np_handler(void) {
	serial_puts("Invalid segment handler was called.\r\n");
}

void exception_ss_handler(void) {
	serial_puts("Stack Fault handler was called.\r\n");
}

void exception_pf_handler(uint64_t addr, uint64_t code) {
	serial_puts("Page Fault handler was called for addr ");
	serial_putx(addr);
	serial_puts(" with exception code ");
	serial_putx(code);
	serial_puts(".\r\n");
}
