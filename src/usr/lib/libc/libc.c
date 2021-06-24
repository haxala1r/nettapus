
#include <stddef.h>
#include <stdint.h>

/* This "libc" isn't anywhere near complete yet, and what little functionality
 * it provides is here.
 *
 * No, I don't plan on using this forever. I'll probably port musl when I have
 * the necessary syscalls.
 */

extern void exit(uint64_t err);

extern int32_t open(char *fname, uint64_t mode);
extern int64_t read(int64_t fd, char *buf, int64_t amount);
extern int64_t write(int64_t fd, char *buf, int64_t amount);
extern int32_t close(int64_t fd);

extern int32_t pipe(int32_t *ret);
extern int64_t fork(void);
extern int64_t wait_syscall(int64_t);
extern int64_t exec(char *fname);

extern int64_t getarg(int64_t arg, char *buf, int64_t limit);
extern int64_t chdir(char *buf);

int64_t wait(uint64_t pid) {
	return wait_syscall(pid);
}

/* These are standard file descriptors, set up by the kernel. */
#define STDIN  0
#define STDOUT 1
/* There's no stderr yet. */

int64_t strlen(char *str) {
	if (str == NULL) {
		return -1;
	}
	int64_t ret = 0;
	while (*(str++)) {
		ret++;
	}
	return ret;
}

int64_t strcmp(char *s1, char *s2) {
	int64_t a = 0;
	int64_t b = 0;
	while (1) {
		if (s1[a] == s2[b]) {
			if (s1[a] == '\0') {
				return 0;
			}
			a++; b++;
			continue;
		}
		if (s1[a] > s2[b]) {
			return 1;
		} else {
			return -1;
		}
	}
}

char getc() {
	char c;
	if (read(STDIN, &c, 1) != 1) {
		c = '\0';
	}
	return c;
}

int64_t gets(char *buf, int64_t limit) {
	for (size_t i = 0; i < limit; i++) {
		buf[i] = getc();
		if (buf[i] == '\0') {
			return i;
		}
	}
	return limit;
}

int64_t puts(char *str) {
	int64_t len = strlen(str);
	if (write(STDOUT, str, len) == len) {
		return 0;
	}
	return -1;
}
