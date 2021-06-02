#ifndef IRQ_H
#define IRQ_H 1

#ifdef __cplusplus
extern "C" {
#endif


#define PIC_MASTER_CMD 	0x20
#define PIC_MASTER_DATA 	0x21
#define PIC_SLAVE_CMD	0xA0
#define PIC_SLAVE_DATA	0xA1



#include <stddef.h>
#include <stdint.h>


struct IDT_entry {
	uint16_t offset_low16;
	uint16_t selector;
	uint8_t ist;
	uint8_t type_attr;
	uint16_t offset_mid16;
	uint32_t offset_hi32;
	uint32_t zero32;

} __attribute__((packed));

struct IDT_descriptor {
	uint16_t size;
	uint64_t offset;
} __attribute__((packed));



extern void loadIDT(struct IDT_descriptor *);
extern void irq0();
extern void irq1();
extern void irq2();
extern void irq3();


extern void exception_divide_by_zero(void);
extern void exception_double_fault(void);
extern void exception_gpf(void);
extern void exception_ud(void);
extern void exception_ts(void);
extern void exception_np(void);
extern void exception_ss(void);
extern void exception_pf(void);

extern void syscall_interrupt(void);


void put_time();

uint8_t init_interrupts();

void kpanic();


#ifdef __cplusplus
}
#endif

#endif

