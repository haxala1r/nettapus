#ifndef _VGA_H
#define _VGA_H 1


#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

void vga_fill_screen(uint32_t);
uint8_t vga_init(uintptr_t, uint64_t, uint64_t, uint64_t, uint64_t);
void vga_put_pixel(uint64_t, uint64_t, uint32_t);


#ifdef __cplusplus
}
#endif





#endif 

