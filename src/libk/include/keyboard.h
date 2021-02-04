#ifndef KEYBOARD_H
#define KEYBOARD_H

#ifdef __cplusplus
extern "C" {
#endif

#define KBD_LCTRL_PRESSED 		0x1D
#define KBD_LCTRL_RELEASED 		0x9D

#define KBD_LSHIFT_PRESSED 		0x2A
#define KBD_LSHIFT_RELEASED 		0xAA

#define KBD_RSHIFT_PRESSED 		0x36
#define KBD_RSHIFT_RELEASED		0xB6

#define KBD_LALT_PRESSED			0x38
#define KBD_LALT_RELEASED		0xB8

#define KBD_CAPSLOCK_PRESSED		0x3A
#define KBD_CAPSLOCK_RELEASED	0xBA

#define KBD_NUMLOCK_PRESSED		0x45
#define KBD_NUMLOCK_RELEASED		0xC5

#define KBD_SCROLLLOCK_PRESSED	0x46
#define KBD_SCROLLLOCK_RELEASED	0xC6

#include <stddef.h>
#include <stdint.h>



void kbd_handle_key(uint8_t key);






#ifdef __cplusplus
}
#endif

#endif

