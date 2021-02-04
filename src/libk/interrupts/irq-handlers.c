
#include <interrupts.h>
#include <tty.h>
#include <io.h>
#include <keyboard.h>
#include <task.h>

uint64_t time = 0;

void sleep(uint32_t ticks) {
	volatile uint64_t cur = time;
	
	while (time < (cur + ticks)) {
		asm volatile ("nop;");
	}
	return;
};

void put_time() {
	kputx(time);
};

void irq0_handler() {
	time++;
	outb(PIC_MASTER_CMD, 0x20);
	schedule();	/* The function itself determines whether a task switch should take place.*/
};



void irq1_handler() {
	uint8_t key = inb(0x60);	/* Get the key  that was pressed. */
	kbd_handle_key(key);
	outb(PIC_MASTER_CMD, 0x20);
};


