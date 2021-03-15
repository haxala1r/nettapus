
#include <task.h>
#include <string.h>
#include <mem.h>

struct task *current_task;
struct task *first_task;
struct task *last_task;


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


struct task *get_current_task() {
	return current_task;
};


void initialise_task(struct task *t, void (*main)(), uint64_t pml4t, uint64_t flags, uint64_t stack) {
	t->reg.rax 	= 0;
	t->reg.rbx 	= 0;
	t->reg.rcx 	= 0;
	t->reg.rdx 	= 0;
	t->reg.rdi 	= 0;
	t->reg.rsi 	= 0;
	t->reg.r8 	= 0;
	t->reg.r9 	= 0;
	t->reg.r10 	= 0;
	t->reg.r11 	= 0;
	t->reg.r12 	= 0;
	t->reg.r13 	= 0;
	t->reg.r14 	= 0;
	t->reg.r15 	= 0;

	t->reg.rbp 	= stack;
	t->reg.rsp 	= stack;

	t->reg.cr3 	= pml4t;
	t->reg.rflags = flags;
	t->reg.rip 	= (uint64_t)main;

	memset(t->reg.fxsave_area, 0, 512);


	t->next = NULL;
	t->ticks_remaining = TASK_DEFAULT_TIME;
	t->files = current_task ? current_task->files : NULL;	/* VFS layer will initialise this. */
};


uint8_t create_task(void (*main)()) {
	if (main == NULL) {
		return 1;
	}

	lock_scheduler();
	struct task *t = kmalloc(sizeof(struct task));

	if (t == NULL) {

		unlock_scheduler();
		return 1;
	}

	/* The part to allocate a new stack should be improved. */
	initialise_task(t, main, kgetPML4T()->physical_address, 0x202, alloc_pages(1, 0x80000000, -1) + 0x1000);

	/* Link the new task we created. */
	if (first_task != NULL) {
		t->next = first_task;
		first_task = t;
	} else {
		first_task = t;
		last_task = t;
	}

	unlock_scheduler();
	return 0;
};


void block_task() {
	lock_scheduler();
	current_task->state = TASK_STATE_BLOCK;
	yield();
	unlock_scheduler();
};

void unblock_task(struct task *t) {
	lock_scheduler();

	t->state = TASK_STATE_READY;
	t->next = NULL;
	if (last_task == NULL) {
		first_task = t;
		last_task = t;
	} else {
		last_task->next = t;
		last_task = last_task->next;
	}

	unlock_scheduler();
};

void yield() {
	/* This is a function that switches to the next task on the queue.
	 *
	 * Caller is responsible for locking the scheduler before calling this.
	 * This is to ensure that the task structures don't get modified by another
	 * task before this yield() returns.
	 */

	if (task_switch_lock) {
		task_switch_postponed++;
		return;
	}

	if ((first_task == NULL) || (last_task == NULL)) {
		if ((current_task != NULL) && (current_task->state != TASK_STATE_RUNNING)) {
			/* The task is to be blocked, and there are no other tasks.
			 * We're essentially spending idle time here until there's
			 * some other task we can switch to.
			 */

			struct task *temp = current_task; /* We're "borrowing" the current task to wait. */
			current_task = NULL;

			/* Wait until a task gets unblocked. */
			while (first_task == NULL) {
				/* If another IRQ happens here, let it change task structures. */
				int32_t temp = sched_lock;
				sched_lock = 0;

				/* Wait until an IRQ occurs. */
				__asm__ ("hlt;");

				/* IRQ happened, check again. */
				sched_lock = temp;
			}

			/* A task must have been unblocked, proceed as normal. */
			current_task = temp;

		} else {
			return;
		}
	}

	/* Will keep the task to be switched from. */
	struct task *last;

	if (current_task->state == TASK_STATE_RUNNING) {
		/* Put the task back into the list of running tasks, only if the task
		 * was running as normal. If the task's state was changed to blocked,
		 * this won't be executed, effectively blocking the task.
		 */
		current_task->next = NULL;
		current_task->ticks_remaining = TASK_DEFAULT_TIME;
		current_task->state = TASK_STATE_READY;

		last_task->next = current_task;
		last_task = last_task->next;
	}

	last = current_task;

	/* Unlink the first task. */
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

	switch_task(&(last->reg), &(current_task->reg));
};



void scheduler_irq0() {
	/* This function will be called every time irq0 fires. */

	if (current_task == NULL) {
		/* The CPU is idle. */
		return;
	}

	/* Decrement the current task's counter. */
	if (current_task->ticks_remaining != 0) {
		lock_scheduler();
		current_task->ticks_remaining--;
		unlock_scheduler();
	}

	/* If the scheduler is locked, don't preempt. */
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
		task_switch_postponed = 0;
		lock_scheduler();
		yield();
		unlock_scheduler();
	}
};


uint8_t init_scheduler() {
	lock_scheduler();

	current_task = kmalloc(sizeof(struct task));
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

void print_task(struct task *t) {
	kputx((uint64_t)t);
	kputs(": ");

	kputx(t->ticks_remaining);
	kputs(" ");
	kputx(t->state);
	kputs("\n");
};

void print_tasks() {
	if (current_task == NULL) {
		kputs("The CPU is currently idle. \n");
		return;
	}
	kputs("TASK LIST {Address}: {Ticks remaining} {State}\n");

	print_task(current_task);

	struct task *i = first_task;
	while (i != NULL) {
		print_task(i);
		i = i->next;
	}
}

#endif
