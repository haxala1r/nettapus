#ifndef _TTY_H
#define _TTY_H 1


#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <string.h>

struct PS_font {
	uint32_t magic;
	uint32_t version;
	uint32_t headersize;
	uint32_t flags;
	uint32_t glyph_count;
	uint32_t glyph_size;	/* In bytes. */
	
	/* In pixels.*/
	uint32_t height;		
	uint32_t width;
} __attribute__((packed));


void kputs(char*);
void kputs_color(char*, uint32_t, uint32_t);

uint8_t tty_init(char*);


#ifdef __cplusplus
}
#endif





#endif  

