#ifndef _TTY_H
#define _TTY_H 1


#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <string.h>

//#include "../vga_tty/tty.h"



void init_terminal();

void terminal_printscreen(uint16_t*);

void putentry_at(char, uint8_t, size_t, size_t);

void terminal_scroll();

void terminal_putc(char);

void terminal_put(char*, size_t);


void terminal_puts(char *);

#ifdef __cplusplus
}
#endif





#endif  

