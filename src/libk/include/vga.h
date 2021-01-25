#ifndef _VGA_H
#define _VGA_H 1


#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>


static uint64_t FB_ADDR;
static uint32_t* FB;
static uint64_t FB_width	= 0;	/* Amount of columns on the screen. 	*/
static uint64_t FB_height 	= 0;	/* Amount of rows on the screen.     	*/
static uint64_t FB_bpp		= 0;	/* How many bits to go one pixel right	*/
static uint64_t FB_pitch	= 0;	/* How many bytes to go one pixel down	*/

void vga_fill_screen(uint32_t);
uint8_t vga_init(uintptr_t, uint64_t, uint64_t, uint64_t, uint64_t);
void vga_put_pixel(uint64_t, uint64_t, uint32_t);


#ifdef __cplusplus
}
#endif





#endif 

