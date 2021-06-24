#ifndef H
#define H

#include <stddef.h>
#include <stdint.h>

/* These are standard file descriptors, set up by the kernel. */
#define STDIN  0
#define STDOUT 1
/* There's no stderr yet. */

#define O_READ  0
#define O_WRITE 1


struct dirent {
	uint64_t inode;
	uint64_t offset;
	uint64_t len;
	uint64_t type;
	char name[1];
};


/* Syscalls. */
extern void exit(uint64_t err);

extern int32_t open(char *fname, uint64_t mode);
extern int64_t read(int64_t fd, char *buf, int64_t amount);
extern int64_t write(int64_t fd, char *buf, int64_t amount);
extern int32_t close(int64_t fd);
extern int64_t getarg(int64_t arg, char *buf, uint64_t limit);

extern int64_t wait_syscall(uint64_t);
extern int32_t pipe(int32_t *ret);
extern int64_t fork(void);
extern int64_t exec(char *fname, char *argv[]);

extern int64_t chdir(char *buf);

int64_t wait(uint64_t);

int64_t strlen(char *str);
int64_t strcmp(char *s1, char *s2);

char getc();
int64_t gets(char *buf, int64_t limit);
int64_t puts();


#endif
