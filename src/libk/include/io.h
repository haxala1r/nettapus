
#ifndef _IO_H
#define _IO_H 1


#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>


uint8_t inb(uint16_t port);

uint16_t inw(uint16_t port);

uint32_t inl(uint16_t port);



void outb(uint16_t port, uint8_t val);

void outw(uint16_t port, uint16_t val);

void outl(uint16_t port, uint32_t val);



void io_wait(void);



#ifdef __cplusplus
}
#endif


#endif

