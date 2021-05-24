
#include <keyboard.h>
#include <tty.h>
#include <mem.h>

/* Whether the keyboard interrupts will work or not. It works similarly
 * to a lock, the keyboard won't be flushed immediately if this lock is
 * active.
 */
uint8_t keyboard_disabled = 0;

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


char *key_buf;
size_t key_buf_limit = 0x200;
size_t key_buf_cursor = 0;

/* This is the keyset we'll look up our characters on.
 * '#' represents invalid characters.
 * '*' represents characters that are valid, but not displayable. (e. g. the enter key.)
 */
char keyset1[0xFF] 		= "#*1234567890-=\b\tQWERTYUIOP[]\n*ASDFGHJKL;'`*\\ZXCVBNM,./*** *************789-456+1230.###**";
char keyset1_up[0xFF] 	= "#*!'^+%&/()=_?\b\tQWERTYUIOP[]\n*ASDFGHJKL;'`*\\ZXCVBNM,./*** *************789-456+1230.###**";
char keyset1_low[0xFF] 	= "#*1234567890-=\b\tqwertyuiop[]\n*asdfghjkl;'`*\\zxcvbnm,./*** *************789-456+1230.###**";


void disable_kbd_flush() {
	keyboard_disabled++;
}
void enable_kbd_flush() {
	if (keyboard_disabled == 0) {
		return;
	};
	keyboard_disabled--;
}

void kbd_flush() {
	/* This function flushes the keys that have been put into the buffer to
	 * the screen.
	 */

	if (keyboard_disabled) {
		return;
	}
	if (key_buf_cursor != 0) {
		if (key_buf_cursor >= key_buf_limit) {
			/* There must have been an error somewhere. Fix that. */
			key_buf_cursor = 0;
			return;
		}

		/* Print the data to screen.*/
		kput_data(key_buf, key_buf_cursor);

		/* Clear the buffer. */
		memset(key_buf, 0, key_buf_cursor);
		key_buf_cursor = 0;
	}
}


void kbd_handle_key(uint8_t key) {

	if ((keyset1_low[key] == '#')) {
		return;
	}

	if ((keyset1_low[key] == '*') || (keyset1[key] == '\0')) {

		/* Handle non-displayable characters */
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


			/* The *lock buttons are special. They should switch their
			 * state when pressed instead of being set only when pressed.
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

			/* Do nothing when the *lock keys are released. */
			case KBD_CAPSLOCK_RELEASED:
			case KBD_NUMLOCK_RELEASED:
			case KBD_SCROLLLOCK_RELEASED:
				break;

			default:
				break;
		}

		return;
	}

	/* Handle upper- and lower-case letters. */
	if (lshift || rshift || capslock) {
		key_buf[key_buf_cursor++] = keyset1_up[key];
	} else {
		key_buf[key_buf_cursor++] = keyset1_low[key];
	}

	return;
}

uint8_t init_kbd() {
	/* The keyboard driver keeps all pressed keys in a buffer, and this buffer
	 * is flushed periodically.
	 * TODO: complete this.
	 */
	key_buf = kmalloc(key_buf_limit);
	if (key_buf == NULL) {
		return 1;
	}
	return 0;
}

