#include <task.h>
#include <string.h>
#include <mem.h>
#include <err.h>
#include <tty.h>


extern int64_t fork_ret();
extern void save_registers();
extern void kpanic();

/* This is a kernel-side implementation of the fork() syscall. */
int64_t kfork(void) {
	lock_scheduler();
	struct task *ptask = get_current_task(); /* Parent*/
	if (ptask == NULL) {
		unlock_scheduler();
		return -ERR_NO_RESULT;
	}

	struct task *ctask = copy_task(ptask);

	/* We have to temporarily map the kernel stack of the child task here. */
	map_memory(get_page_entry(ctask->pml4t, 0xFFFFFF7FFFFFF000), 0xFFFFFFFF98000000, 1, ptask->pml4t, 0);
	loadPML4T(getCR3());

	int64_t ret = fork_ret(ptask, ctask, 0xFFFFFFFF98000000);

	if (ret == 1) {
		/* Success, parent process. */
		unmap_memory(0xFFFFFFFF98000000, 1, ptask->pml4t);
		unlock_scheduler();
		loadPML4T(getCR3());

		return ctask->pid;
	} else if (ret == 0) {
		/* Success, child process. */
		unlock_scheduler();
		loadPML4T(getCR3());
		return 0;
	} else {
		/* Failure. */
		serial_puts("kfork() failed \r\n");
		kpanic();
	}

	return ret;
}

