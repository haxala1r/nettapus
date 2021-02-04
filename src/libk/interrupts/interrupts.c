
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
	
	uint64_t addr;	/* A variable we (re)use to store the address of an irq handler.*/
	
	
	IDTR.size = (sizeof(struct IDT_entry) * 256);
	IDTR.offset = (uint64_t)IDT;
	
	/* First comes the exceptions. */
	
	addr = (uint64_t) exception_divide_by_zero;
	IDT[0x0].offset_low16 = (addr & 0xFFFF);
	IDT[0x0].offset_mid16 = ((addr >> 16) & 0xFFFF);
	IDT[0x0].offset_hi32 = ((addr >> 32) & 0xFFFFFFFF);
	IDT[0x0].zero = 0;
	IDT[0x0].zero32 = 0;
	IDT[0x0].type_attr = 0x8e;
	IDT[0x0].selector = 8;
	
	
	addr = (uint64_t) exception_double_fault;
	IDT[0x8].offset_low16 = (addr & 0xFFFF);
	IDT[0x8].offset_mid16 = ((addr >> 16) & 0xFFFF);
	IDT[0x8].offset_hi32 = ((addr >> 32) & 0xFFFFFFFF);
	IDT[0x8].zero = 0;
	IDT[0x8].zero32 = 0;
	IDT[0x8].type_attr = 0x8e;
	IDT[0x8].selector = 8;
	
	
	/* Now hardware IRQs*/
	
	addr = (uint64_t) irq0;
	IDT[0x20].offset_low16 = (addr & 0xFFFF);
	IDT[0x20].offset_mid16 = ((addr >> 16) & 0xFFFF);
	IDT[0x20].offset_hi32 = ((addr >> 32) & 0xFFFFFFFF);
	IDT[0x20].zero = 0;
	IDT[0x20].zero32 = 0;
	IDT[0x20].type_attr = 0x8e;
	IDT[0x20].selector = 8;
	
	
	addr = (uint64_t) irq1;
	IDT[0x21].offset_low16 = (addr & 0xFFFF);
	IDT[0x21].offset_mid16 = ((addr >> 16) & 0xFFFF);
	IDT[0x21].offset_hi32 = ((addr >> 32) & 0xFFFFFFFF);
	IDT[0x21].zero = 0;
	IDT[0x21].zero32 = 0;
	IDT[0x21].type_attr = 0x8e;
	IDT[0x21].selector = 8;
	
	
	PIC_remap(0x20, 0x28);
	
	loadIDT(&IDTR);
	
	
	/* Initialise the Programmable Interval Timer, so that we can keep track of time.*/
	init_pit(0xE90C);
	
	outb(PIC_MASTER_DATA, 0xFC);
	outb(PIC_SLAVE_DATA, 0xFF);
	
	
	
	
	return 0;
};


