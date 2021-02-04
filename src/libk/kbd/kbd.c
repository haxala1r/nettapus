
#include <keyboard.h>
#include <tty.h>




/* Here are the global variables used to keep track of the states of
 *  non-displayable (e.g. shift) keys. 
 */
uint8_t lshift = 0; 
uint8_t rshift = 0;
uint8_t lctrl = 0;
//uint8_t rctrl = 0;
uint8_t lalt = 0;
//uint8_t ralt = 0;

uint8_t capslock = 0; 
uint8_t numlock = 0;
uint8_t scrolllock = 0;




/* This is the keyset we'll look up our characters on. 
 * '#' represents invalid characters.
 * '*' represents characters that are valid, but not displayable. (e. g. the enter key.)
 */
char keyset1[0xFF] 		= "#*1234567890-=\b\tQWERTYUIOP[]\n*ASDFGHJKL;'`*\\ZXCVBNM,./*** *************789-456+1230.###**";
char keyset1_up[0xFF] 	= "#*!'^+%&/()=_?\b\tQWERTYUIOP[]\n*ASDFGHJKL;'`*\\ZXCVBNM,./*** *************789-456+1230.###**";
char keyset1_low[0xFF] 	= "#*1234567890-=\b\tqwertyuiop[]\n*asdfghjkl;'`*\\zxcvbnm,./*** *************789-456+1230.###**";




void kbd_handle_key(uint8_t key) {

	if ((keyset1_low[key] == '#')) {
		return;
	}
	
	if ((keyset1_low[key] == '*') || (keyset1[key] == '\0')) {
		
		/* Don't let this scare you, it only manages the "non-displayable" characters. */
		switch (key) {
			case 0x1:
				break;
			case KBD_LCTRL_PRESSED:
				lctrl = 1;
				break;
			case KBD_LCTRL_RELEASED:
				lctrl = 0;
				break;
			case KBD_LSHIFT_PRESSED:
				lshift = 1;
				break;
			case KBD_LSHIFT_RELEASED:
				lshift = 0;
				break;
			case KBD_RSHIFT_PRESSED:
				rshift = 1;
				break;
			case KBD_RSHIFT_RELEASED:
				rshift = 0;
				break;
			case KBD_LALT_PRESSED:
				lalt = 1;
				break;
			case KBD_LALT_RELEASED:
				lalt = 0;
				break;
				
			
			/* The *lock buttons are special. They should switch their state when pressed
			 * instead of being set only when pressed. 
			 */
			case KBD_CAPSLOCK_PRESSED:
				
				capslock ^= 1;
				break;
			case KBD_NUMLOCK_PRESSED:
				numlock ^= 1;
				break;
			case KBD_SCROLLLOCK_PRESSED:
				scrolllock ^= 1;
				break;
			
			/* And do nothing when the key is released. */
			case KBD_CAPSLOCK_RELEASED:
			case KBD_NUMLOCK_RELEASED:
			case KBD_SCROLLLOCK_RELEASED:
				break;	
			
			default: 
				break;
		}
		
		return;
	}
	
	
	
	if (lshift || rshift || capslock) {
		kput_data(keyset1_up + key, 1);
	} else {
		kput_data(keyset1_low + key, 1);
	}
	return;
};



