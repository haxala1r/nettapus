
#ifndef _IO_H
#define _IO_H 1


#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

//I know this is a header but these functions aren't even long anyway okay?
inline uint8_t inb(uint16_t port) {
	uint8_t ret;
	__asm__ volatile(
		"inb %1, %0"
		: "=a"(ret)
		: "Nd"(port)
	);
	
	return ret;
}

inline uint16_t inw(uint16_t port) {
	uint16_t ret;
	__asm__ volatile (
		"inw %1, %0"
		: "=a"(ret)
		: "Nd"(port)
	);
	
	return ret;
}

inline uint32_t inl(uint16_t port) {
	uint32_t ret;
	__asm__ volatile (
		"inl %1, %0"
		: "=a"(ret)
		: "Nd"(port)
	);
	
	return ret;
}



inline void outb(uint16_t port, uint8_t val) {
	__asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port) );
}

inline void outw(uint16_t port, uint16_t val) {
	__asm__ volatile (
		"outw %0, %1" 
		: 
		: "a"(val), "Nd"(port) );
}

inline void outl(uint16_t port, uint32_t val) {
	__asm__ volatile ("outl %0, %1" 
	: 
	: "a"(val), "Nd"(port) );
}




inline void io_wait(void) {
	__asm__ volatile ("outb %%al, $0x80" : : "a"(0));
};



#ifdef __cplusplus
}
#endif


#endif

