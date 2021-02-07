
#include <task.h>
#include <tty.h>
#include <string.h>
#include <mem.h>

Task *current_task;
Task *first_task;
Task *last_task;


volatile int32_t sched_lock = 0;

Task *get_current_task() {
	return current_task;
};


void initialise_task(Task *task, void (*main)(), uint64_t pml4t, uint64_t flags, uint64_t stack) {
	task->reg.rax 	= 0;
	task->reg.rbx 	= 0;
	task->reg.rcx 	= 0;
	task->reg.rdx 	= 0;
	task->reg.rdi 	= 0;
	task->reg.rsi 	= 0;
	task->reg.r8 	= 0;
	task->reg.r9 	= 0;
	task->reg.r10 	= 0;
	task->reg.r11 	= 0;
	task->reg.r12 	= 0;
	task->reg.r13 	= 0;
	task->reg.r14 	= 0;
	task->reg.r15 	= 0;
	
	task->reg.rbp 	= stack;
	task->reg.rsp 	= stack;
	
	task->reg.cr3 	= pml4t;
	task->reg.rflags = flags;
	task->reg.rip 	= (uint64_t)main;
	
	memset(task->reg.fxsave_area, 0, 512);
	
	
	task->next = NULL;	
	task->files = NULL;	/* VFS layer will initialise this. */
};


uint8_t create_task(void (*main)()) {
	if (main == NULL) {
		return 1;
	}
	
	Task *task = kmalloc(sizeof( Task ));
	if (task == NULL) {
		return 1;
	}
	
	initialise_task(task, main, kgetPML4T()->physical_address, 0x202, alloc_pages(1, 0x80000000, -1) + 0x1000);
	
	last_task->next = task;
	last_task = last_task->next;
	last_task->next = first_task;
	
	return 0;
};



void schedule() {
	if ((current_task == NULL) || (current_task->next == current_task)) {
		return;		/* Do not attempt a task switch if it will accomplish nothing. */
	}
	if (sched_lock) {
		return;		/* Do not attempt a task switch if the scheduler is locked.  */
	}
	
	/* If we reach here, that means we need to switch to the next task on the list. For now,
	 * this is a round-robin scheduler, and it is easy to determine which task to switch to.
	 */
	 
	Task *last = current_task;
	current_task = current_task->next;
	
	switch_task(&(last->reg), &(current_task->reg));

};



void lock_scheduler() {
	/* For now, we don't actually have anything resembling a lock, so we actually just enable
	 * and disable interrupts. */
	sched_lock++;
};


void unlock_scheduler() {
	if (sched_lock == 0) {
		return;
	}
	sched_lock--;
};




uint8_t init_scheduler() {
	
	/* Ensure that a "task switch" does not occur while we're in the middle of setting 
	 * everything up. */
	lock_scheduler();	
	
	
	/* This is the currently running (i. e. the one that called this function.) It does not
	 * need to be initialised, the state will be saved here when a task switch occurs anyway.
	 */
	current_task = kmalloc(sizeof(Task));
	
	if (current_task == NULL) {
		return 1;
	}
	
	/* Initialise the global variables with valid values.*/
	current_task->next = current_task;
	current_task->files = NULL;	/* The VFS layer will initialise this. */
	last_task = current_task;
	first_task = current_task;
	
	/* Release the lock we acquired.*/
	unlock_scheduler();
	
	return 0;
};

