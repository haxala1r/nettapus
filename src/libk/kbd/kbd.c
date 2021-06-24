
#include <keyboard.h>
#include <tty.h>
#include <task.h>
#include <fs/fs.h>
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


struct file_vnode *kbd_pipe;

/* This is a buffer where the key's pressed by the user will be kept in until
 * enter is pressed, in which case the data here is flushed to kbd_pipe.
 */
char key_buf[0x200];
size_t key_buf_cursor = 0; /* Where we are in the buffer.*/

/* This is the keyset we'll look up our characters on.
 * '#' represents invalid characters.
 * '*' represents characters that are valid, but not displayable. (e. g. the enter key.)
 */
char keyset1[0xFF] 		= "#*1234567890-=\b\tQWERTYUIOP[]\n*ASDFGHJKL;'`*\\ZXCVBNM,./*** *************789-456+1230.###**";
char keyset1_up[0xFF] 	= "#*!'^+%&/()=_?\b\tQWERTYUIOP[]\n*ASDFGHJKL;'`*\\ZXCVBNM,./*** *************789-456+1230.###**";
char keyset1_low[0xFF] 	= "#*1234567890-=\b\tqwertyuiop[]\n*asdfghjkl;'`*\\zxcvbnm,./*** *************789-456+1230.###**";


void kbd_flush() {
	/* This function flushes the keys that have been put into the buffer to
	 * the pipe.
	 */

	if (key_buf_cursor != 0) {
		size_t to_cpy = ((kbd_pipe->size + key_buf_cursor) > 0x1000) ? (0x1000 - kbd_pipe->size) : (key_buf_cursor);
		memcpy(((char *)kbd_pipe->pipe_mem) + kbd_pipe->size, key_buf, to_cpy);
		kbd_pipe->size += to_cpy;

		key_buf_cursor = 0;

		/* Wake up any task that might be waiting for input. */
		signal_queue(kbd_pipe->read_queue);
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
	if ((keyset1_up[key] == '\b') && (key_buf_cursor != 0)) {
		key_buf[--key_buf_cursor] = '\0';
		kputc('\b');
	} else if (keyset1_up[key] == '\b') {
		/* key_buf_cursor == 0 */

	} else if (lshift || rshift || capslock) {
		key_buf[key_buf_cursor++] = keyset1_up[key];
		kputc(keyset1_up[key]);
	} else {
		key_buf[key_buf_cursor++] = keyset1_low[key];
		kputc(keyset1_low[key]);
	}

	if (keyset1_up[key] == '\n') {
		key_buf[key_buf_cursor - 1] = '\0';
		kbd_flush();
	}

	return;
}

struct file_descriptor *init_kbd() {
	/* The keyboard driver creates a (custom) pipe, and returns a file descriptor
	 * to it. This descriptor can be added to any task's list of fds. When
	 * a keyboard IRQ is raised, the key will be written to this pipe.
	 */
	lock_scheduler();
	kbd_pipe = kmalloc(sizeof(*kbd_pipe));
	if (kbd_pipe == NULL) {
		return NULL;
	}
	memset(kbd_pipe, 0, sizeof(*kbd_pipe));

	kbd_pipe->pipe_mem = kmalloc(0x1000);
	kbd_pipe->mutex = create_semaphore(1);
	kbd_pipe->streams_open = 2;

	kbd_pipe->open = vfs_open_file;
	kbd_pipe->read = vfs_read_pipe;
	kbd_pipe->write = NULL; /* Writing is not permitted to this pipe. */
	kbd_pipe->close = vfs_close_file;

	kbd_pipe->read_queue = kmalloc(sizeof(QUEUE));
	memset(kbd_pipe->read_queue, 0, sizeof(QUEUE));
	kbd_pipe->write_queue = kmalloc(sizeof(QUEUE));;
	memset(kbd_pipe->write_queue, 0, sizeof(QUEUE));

	struct file_descriptor *ret = kmalloc(sizeof(*ret));
	memset(ret, 0, sizeof(*ret));

	ret->file = 1;
	ret->node = kbd_pipe;
	ret->mode = 0;
	ret->fd = 0;

	unlock_scheduler();
	return ret;
}

