
#include <task.h>
#include <string.h>
#include <mem.h>

TASK *current_task;
TASK *first_task;
TASK *last_task;


/* This lock will be used to make sure only one task can access the task
 * structures at a time. This should be replaced with a proper semaphore 
 * in the future.
 */
volatile int32_t sched_lock = 0;

/* This lock will be used for miscellanious stuff to simply postpone task 
 * switches while the cpu is holding a (any) lock. It does not protect the
 * task structures.  
 */
volatile int32_t task_switch_lock = 0;

/* Flag to check whether a task switch was postponed while the above lock
 * was being held.
 */
int32_t task_switch_postponed = 0;		


TASK *get_current_task() {
	return current_task;
};


void initialise_task(TASK *task, void (*main)(), uint64_t pml4t, uint64_t flags, uint64_t stack) {
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
	task->ticks_remaining = TASK_DEFAULT_TIME;
	task->files = current_task->files;	/* VFS layer will initialise this. */
};


uint8_t create_task(void (*main)()) {
	if (main == NULL) {
		return 1;
	}
	
	/* Make sure we're the only one that can access these structures. */
	lock_scheduler();
	
	/* Allocate space for the task. */
	TASK *task = kmalloc(sizeof(TASK));
	
	if (task == NULL) {
		unlock_scheduler();
		return 1;
	}
	
	/* This should be changed with something to create a new address space,
	 * instead of working on the kernel address space. Otherwise, 
	 * there really isn't much point in using paging in the first place. 
	 */
	initialise_task(task, main, kgetPML4T()->physical_address, 0x202, alloc_pages(1, 0x80000000, -1) + 0x1000);
	
	/* Link the new task we created. */
	if (first_task != NULL) {
		task->next = first_task;
		first_task = task;
	} else {
		first_task = task;
		last_task = task;
	}
	
	/* Release the lock we got. */
	unlock_scheduler();
	return 0;
};


void block_task() {
	lock_scheduler();
	current_task->state = TASK_STATE_BLOCK;
	yield();
	unlock_scheduler();
};




void unblock_task(TASK *task) {
	
	lock_scheduler();
	
	task->state = TASK_STATE_READY;
	task->next = NULL;
	if (last_task == NULL) {
		first_task = task;
		last_task = task;
	} else {
		last_task->next = task;
		last_task = last_task->next;
	}
	
	unlock_scheduler();
};



void yield() {
	/* This is a function that switches to the next task on the queue.
	 * It does not check for the amount of time the task has. It does reset
	 * its remaining time though.
	 * 
	 * IRQ0 should call schedule() which is a wrapper around this function.
	 * This function can be called if a task wants to give up its remaining
	 * time.
	 * 
	 * Caller is responsible for locking the scheduler before calling this.
	 * This is to ensure that the task structures don't get modified by another
	 * task before this yield() returns.
	 */
	
	if ((first_task == NULL) || (last_task == NULL)) {
		return;		/* There aren't any tasks to switch to. 	*/
	}
	
	if (task_switch_lock) {
		task_switch_postponed++;
		return;		/* Task switches are being postponed. 		*/
	}
	
	
	/* This will point to the task to be switched from, i.e. the current task.
	 * current_task will be made to point to the task to be switched to.
	 */
	TASK *last;
	
	if (current_task->state == TASK_STATE_RUNNING) {
		/* Put the currently running task back to the list of ready-to-run
		 * tasks. The check makes it so that if the task's state was changed
		 * to blocked before this call, then it won't be added back into
		 * the list. This is useful when blocking tasks.
		 */
		current_task->next = NULL;
		current_task->ticks_remaining = TASK_DEFAULT_TIME;
		current_task->state = TASK_STATE_READY;
		
		
		last_task->next = current_task;
		last_task = last_task->next;
		
	} 
	
	/* Now unlink the first task. */
	last = current_task;
	current_task = first_task;
	if (first_task == last_task) {
		first_task = NULL;
		last_task = NULL;
	} else {
		first_task = first_task->next;
	}
	
	
	current_task->state = TASK_STATE_RUNNING;
	current_task->ticks_remaining = TASK_DEFAULT_TIME;
	current_task->next = NULL;
	
	/* Now we can switch. */
	switch_task(&(last->reg), &(current_task->reg));
};



void scheduler_irq0() {
	/* This function will be called every time irq0 fires. */
	
	if ((current_task == NULL) || (current_task->next == current_task)) {
		return;		
	}
	
	
	/* Decrement the current task's counter. The check is here
	 * because if it isnt made, then some factor (e.g. a lock) can make
	 * the remaining time decrement to zero, but not switch. This would
	 * cause an underflow that needs to be avoided.
	 */
	
	if (current_task->ticks_remaining != 0) {
		lock_scheduler();
		current_task->ticks_remaining--;
		unlock_scheduler();
	}
	
	
	/* Do not attempt a task switch if the scheduler is locked. Even if
	 * the current task has run out of time.
	 */
	if (sched_lock) {
		return;		
	}
	
	
	/* If the current task has run out of time, then switch tasks. */
	if (current_task->ticks_remaining == 0) {
		lock_scheduler();
		yield();
		unlock_scheduler();
	}
};



/* When SMP support is added, all of these lock/unlock functions
 * need to be changed to actually guarantee mutual exclusion.
 * For a single CPU however, this works well enough.
 * 
 * A spinlock could be good enough, as these structures aren't used 
 * for that long.
 */

void lock_scheduler() {
	/* This will hopefully be changed to a spinlock soon. */
	sched_lock++;
};

void lock_task_switches() {
	task_switch_lock++;
};



void unlock_scheduler() {
	if (sched_lock == 0) {
		return;
	}
	sched_lock--;
};

void unlock_task_switches() {
	if (task_switch_lock == 0) {
		return;
	}
	task_switch_lock--;
	if ((task_switch_lock == 0) && task_switch_postponed) {
		lock_scheduler();
		yield();
		unlock_scheduler();
	}
};


uint8_t init_scheduler() {
	
	/* Ensure that a task switch does not occur while we're in the middle of setting 
	 * everything up. */
	lock_scheduler();	
	
	
	/* This is the currently running (i. e. the one that called this function.) It does not
	 * need to be initialised, the state will be saved here when a task switch occurs anyway.
	 */
	current_task = kmalloc(sizeof(TASK));
	
	if (current_task == NULL) {
		return 1;
	}
	
	current_task->next = NULL;
	current_task->state = TASK_STATE_RUNNING;
	current_task->files = NULL;	/* The VFS layer will initialise this. */
	current_task->ticks_remaining = TASK_DEFAULT_TIME;
	
	/* Initialise the global variables. */
	last_task = NULL;
	first_task = NULL;
	
	/* Release the lock we acquired.*/
	unlock_scheduler();
	
	return 0;
};



#ifdef DEBUG
#include <tty.h>

void print_task(TASK *task) {
	kputx((uint64_t)task);
	kputs(": ");
	
	kputx(task->ticks_remaining);
	kputs(" ");
	kputx(task->state);
	kputs("\n");
};

void print_tasks() {
	kputs("TASK LIST {Address}: {Ticks remaining} {State}\n");
	
	print_task(current_task);
	
	TASK *i = first_task;
	while (i != NULL) {
		print_task(i);
		i = i->next;
	}
}

#endif
