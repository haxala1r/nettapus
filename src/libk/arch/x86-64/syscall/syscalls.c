#include <syscall.h>
#include <task.h>
#include <tty.h>
#include <err.h>
#include <fs/fs.h>

/* This file implements all the system calls. The functions here (called by the
 * syscall interrupt) only check the given parameters and then call a function
 * that does the same thing from the kernel-side.
 * For specific implementations, you can check the functions called.
 */

/* the exit() syscall never returns. It always results in the termination of a task,
 * regardless of anything else.
 */
void exit() {
	terminate_task();
}

/* fork creates an exact copy of the task, with only the return value being different.
 * Currently, since there are no PIDs, the parent process always returns 1, the
 * child returns 0
 */
int64_t fork(void) {
	return kfork();
}

/* exec creates a new process from a given file, and kills the current task.
 * A successful exec never returns, but if the given parameters are invalid,
 * then exec will return an error code in rax.
 *
 * see doc/exec for more info about argv.
 */
int64_t exec(char *fname, char *argv[]) {
	struct task *t = get_current_task();
	if (!is_mapped((uintptr_t)fname, t->pml4t)) {
		return -ERR_INVALID_PARAM;
	}
	if ((uintptr_t)fname >= 0xFFFFFF7FFFFFF000) {
		return -ERR_INVALID_PARAM;
	}

	/* We need to do some extensive bounds-checking here, otherwise all kinds
	 * of vulnerabilities could pop up.
	 */
	if (argv != NULL) {
		if (!is_mapped((uintptr_t)argv, t->pml4t)) {
			return -ERR_INVALID_PARAM;
		}
		if ((uintptr_t)argv >= 0xFFFFFF7FFFFFF000) {
			return -ERR_INVALID_PARAM;
		}

		char **i = argv;
		while (*i != NULL) {
			if (!is_mapped((uintptr_t)*i, t->pml4t)) {
				return -ERR_INVALID_PARAM;
			}
			if ((uintptr_t)*i >= 0xFFFFFF7FFFFFF000) {
				return -ERR_INVALID_PARAM;
			}
			i++;
		}
		/* If any problems occur due to a page not being mapped, or a string
		 * not being terminated by a NUL character, the PF handler should kill
		 * the current task.
		 *
		 * TODO: Actually make PF handler kill the current process on a page
		 * fault. This is not yet implemented, and should be implemented soon
		 * if this system is supposed to be anything other than a joke.
		 */
	}
	serial_puts("After the argv check!\r\n");

	uintptr_t entry_addr;

	p_map_level4_table *pml4t = load_elf(fname, &entry_addr);
	if (pml4t == NULL){
		return -ERR_NOT_FOUND;
	}

	lock_scheduler();
	struct task *oldt = get_current_task();
	struct task *newt = create_task((void (*)())entry_addr, pml4t, 3, argv);
	newt->pid = oldt->pid;
	newt->fds = oldt->fds;
	newt->current_dir = oldt->current_dir;

	kfree(newt->wait_queue);
	newt->wait_queue = oldt->wait_queue;
	oldt->wait_queue = NULL;
	oldt->fds = NULL;
	oldt->current_dir = NULL;


	unlock_scheduler();

	exit(0);
	return GENERIC_SUCCESS;
}

int64_t wait(uint64_t pid) {
	struct task *t = find_task(pid);
	if (t == NULL) {
		return -ERR_INVALID_PARAM;
	}
	wait_queue(t->wait_queue);
	return 0;
}


int64_t open(char *fname, int64_t mode) {
	if (!is_mapped((uintptr_t)fname, get_current_task()->pml4t)) {
		return -ERR_INVALID_PARAM;
	}
	if ((uintptr_t)fname >= 0xFFFFFF7FFFFFF000) {
		return -ERR_INVALID_PARAM;
	}

	return kopen(fname, mode);
}


int64_t read(int64_t fd, void *buf, int64_t amount) {
	if (!is_mapped((uintptr_t)buf, get_current_task()->pml4t)) {
		return -ERR_INVALID_PARAM;
	}
	if ((uintptr_t)buf >= 0xFFFFFF7FFFFFF000) {
		return -ERR_INVALID_PARAM;
	}
	if (amount < 0) {
		return 0;
	}

	return kread(fd, buf, amount);
}

int64_t write(int64_t fd, void *buf, int64_t amount) {
	if (!is_mapped((uintptr_t)buf, get_current_task()->pml4t)) {
		return -ERR_INVALID_PARAM;
	}
	if ((uintptr_t)buf >= 0xFFFFFF7FFFFFF000) {
		return -ERR_INVALID_PARAM;
	}
	if (amount < 0) {
		return 0;
	}

	return kwrite(fd, buf, amount);
}

int64_t close(int32_t fd) {
	return kclose(fd);
}

int64_t pipe(int32_t *ret) {
	if (!is_mapped((uintptr_t)ret, get_current_task()->pml4t)) {
		return -ERR_INVALID_PARAM;
	}
	if ((uintptr_t)ret >= 0xFFFFFF7FFFFFF000) {
		return -ERR_INVALID_PARAM;
	}
	return kpipeu(get_current_task(), ret);
}

int64_t chdir(char *path) {
	if (!is_mapped((uintptr_t)path, get_current_task()->pml4t)){
		return -ERR_INVALID_PARAM;
	}
	if ((uintptr_t)path >= 0xFFFFFF7FFFFFF000) {
		return -ERR_INVALID_PARAM;
	}
	return kchdir(get_current_task(), path);
}

int64_t getarg(uint64_t arg, char *buf, uint64_t limit) {
	/* arg is the argument index (starts at 0)
	 * buf is the buffer the argument will be copied to
	 * limit is the maximum size of buf
	 */
	if (!is_mapped((uintptr_t)buf, get_current_task()->pml4t)) {
		return -ERR_INVALID_PARAM;
	}
	if ((uintptr_t)buf >= 0xFFFFFF7FFFFFF000) {
		return -ERR_INVALID_PARAM;
	}

	char *str = task_get_arg(get_current_task(), arg);
	if (str == NULL) {
		return -ERR_NOT_FOUND;
	}

	size_t len = strlen(str);
	size_t to_copy = (len >= limit) ? (limit - 1) : len;

	memcpy(buf, str, to_copy);

	return to_copy;
}




/* This array holds pointers to all system calls. The syscall handler (in syscall.asm)
 * references this table.
 */
uintptr_t syscall_table[128] = {
	(uintptr_t)&exit,    //  0
	(uintptr_t)&fork,    //  1
	(uintptr_t)&exec,    //  2
	(uintptr_t)&wait,    //  3
	(uintptr_t)&open,    //  4
	(uintptr_t)&read,    //  5
	(uintptr_t)&write,   //  6
	(uintptr_t)&close,   //  7
	(uintptr_t)&pipe,    //  8
	(uintptr_t)&chdir,   //  9
	(uintptr_t)&getarg,  //  10
};

/* Likewise, the syscall handler also accesses this. That is the sole reason we
 * need this one, actually.
 */
uint64_t syscall_count = 11;
