
#include <interrupts.h>
#include <io.h>

struct IDT_descriptor IDTR;

struct IDT_entry IDT[256];


void PIC_remap(uint8_t master_offset, uint8_t slave_offset) {
	
	uint8_t master_mask, slave_mask;	
	
	master_mask = inb(PIC_MASTER_DATA);
	slave_mask = inb(PIC_SLAVE_DATA);
	
	outb(PIC_MASTER_CMD, 0x10 | 0x01);
	io_wait();
	outb(PIC_SLAVE_CMD, 0x10 | 0x01);
	io_wait();
	
	outb(PIC_MASTER_DATA, master_offset);
	io_wait();
	outb(PIC_SLAVE_DATA, slave_offset);
	io_wait();
	
	outb(PIC_MASTER_DATA, 0x4);
	io_wait();
	outb(PIC_SLAVE_DATA, 0x2);
	io_wait();
	
	outb(PIC_MASTER_DATA, 0x1);
	io_wait();
	outb(PIC_SLAVE_DATA, 0x1);
	io_wait();
	
	outb(PIC_MASTER_DATA, master_mask);
	outb(PIC_SLAVE_DATA, slave_mask);

	
	return;
};




void init_pit(uint16_t reload_value) {
	
	outb(0x43, 0b00110100);
	outb(0x40, reload_value & 0xFF);
	outb(0x40, (reload_value >> 8) & 0xFF);
	
	return;
};



void set_IDT_entry(uint8_t index, void (*func_ptr)()) {
	uint64_t addr = (uint64_t) func_ptr;
	IDT[index].offset_low16 = (addr & 0xFFFF);
	IDT[index].offset_mid16 = ((addr >> 16) & 0xFFFF);
	IDT[index].offset_hi32 = ((addr >> 32) & 0xFFFFFFFF);
	IDT[index].zero = 0;
	IDT[index].zero32 = 0;
	IDT[index].type_attr = 0x8e;
	IDT[index].selector = 8;
};





uint8_t init_interrupts() {
	IDTR.size = (sizeof(struct IDT_entry) * 256);
	IDTR.offset = (uint64_t)IDT;
	
	
	/* First comes the exceptions. */
	set_IDT_entry(0, exception_divide_by_zero);
	set_IDT_entry(8, exception_double_fault);
	
	/* Now hardware IRQs*/
	set_IDT_entry(0x20, irq0);
	set_IDT_entry(0x21, irq1);
	
	/* Set up the PIC. */
	PIC_remap(0x20, 0x28);
	
	/* Now we can load the IDT we just prepared. */
	loadIDT(&IDTR);
	
	/* Initialise the Programmable Interval Timer, so that we can keep track of time.*/
	init_pit(0xE90);
	
	outb(PIC_MASTER_DATA, 0xFC);
	outb(PIC_SLAVE_DATA, 0xFF);
	
	__asm__("sti;");
	return 0;
};


