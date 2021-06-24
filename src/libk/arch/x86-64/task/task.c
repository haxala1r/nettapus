
#include <task.h>
#include <string.h>
#include <mem.h>
#include <err.h>
#include <fs/fs.h>
#include <tty.h>

struct task *current_task;
struct task *first_task;
struct task *last_task;

/* Task termination is done as follows:
 * -a process calls terminate_process, which puts the process into this queue
 * and  unblocks the terminator task.
 * -The terminator task wakes up, frees the task and its allocated structures
 * -The terminator task blocks itself again, until another task needs to be
 * terminated.
 */
QUEUE termination_queue;
struct task *terminator_task;

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

/* This is here to keep track of which PID to use next. */
uint64_t last_pid = 0;


struct task *get_current_task() {
	return current_task;
}

struct task *find_task(uint64_t pid) {
	if (current_task->pid == pid) {
		return current_task;
	}
	struct task *i = first_task;
	while (i != NULL) {
		if (i->pid == pid) {
			return i;
		}
		i = i->next;
	}
	return NULL;
}

int64_t kchdir(struct task *t, char *fname) {
	if (t == NULL)     { return -ERR_INVALID_PARAM; }
	if (fname == NULL) { return -ERR_INVALID_PARAM; }

	int64_t file = 1;
	struct folder_vnode *n = vfs_search_vnode(fname, &file);

	if (file) {
		return -ERR_INVALID_PARAM;
	}

	if (n == NULL) {
		return -ERR_INVALID_PARAM;
	}

	t->current_dir = n;

	return GENERIC_SUCCESS;
}

void initialise_task(struct task *t, void (*main)(), p_map_level4_table *pml4t,
uint64_t flags, uint64_t stack, uint64_t kernel_stack, size_t ring, uint64_t argc) {
	t->reg.rax 	= argc;
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
	t->reg.rsp 	= stack-8;

	/* OK, this is important.
	 * The main function isn't set as the RIP. instead a task loader function is
	 * set, and the main function's address is put on the task's stack.
	 * The loader function simply pops this value, determines segment registers
	 * etc, unlocks the scheduler,
	 * and then switches to itself, which causes the real task to start execution.
	 */

	size_t stack_paddr = get_page_entry(pml4t, stack - 1);
	map_memory(stack_paddr, 0xFFFFFFFF98000000, 1, pml4t, 0);
	loadPML4T(getCR3());

	uint64_t *return_ptr = (uint64_t*)(0xFFFFFFFF98001000 - 8);
	*return_ptr = (uint64_t)main;

	unmap_memory(0xFFFFFFFF98000000, 1, pml4t);
	loadPML4T(getCR3());

	/* The kernel stack is used for system calls and interrupts for each task.
	 * It's very unhealthy to use the task's stack, as it may become invalid.
	 * Thus, a seperate kernel stack is kept for each task.
	 * The stack in the TSS can't be used either, because irq0 isn't guaranteed
	 * to return. It might task switch, in which case another IRQ will ruin the
	 * stack. Since no more than one IRQ can be using a task's kernel stack by
	 * definition, this works flawlessly.
	 */
	t->reg.kernel_rsp = kernel_stack;

	t->reg.cr3 	= pml4t->physical_address;
	t->reg.rflags = flags;
	t->reg.rip 	= (uint64_t)task_loader;

	/* The loader function sets the correct segments according to the ring value.*/
	t->reg.cs = 0x08;
	t->reg.ds = 0x10;

	t->next = NULL;
	t->ticks_remaining = TASK_DEFAULT_TIME;
	t->ring = ring ? 3: 0;
	t->fds = NULL;	/* VFS layer will initialise this. */
}


p_map_level4_table *create_address_space() {
	/* This creates a blank address space with a stack and the kernel mapped. */
	p_map_level4_table *pml4t = alloc_page_struct();
	memset(pml4t, 0, 0x2000);
	pml4t->child[511] = kgetPDPT();
	pml4t->entries[511] = kgetPDPT()->physical_address | 2 | 1;

	/* The magic addresses are explained in doc/memory_map.txt and doc/kernel_stack.txt */
	size_t stack_base = allocpp() * 0x1000; // user stack.
	map_memory(stack_base, 0xFFFFFF7000001000, 1, pml4t, 1);

	size_t kernel_stack_base = allocpp() * 0x1000;  // kernel stack.
	map_memory(kernel_stack_base, 0xFFFFFF7FFFFFF000, 1, pml4t, 0);

	return pml4t;
}

struct task *create_task(void (*main)(), p_map_level4_table *pml4t, size_t ring, char *argv[]) {
	if (main == NULL) {
		return NULL;
	}
	/* This function takes a pointer to the start of an executable, creates an
	 * address space for the new task, and then executes that task.
	 */

	lock_scheduler();
	struct task *t = kmalloc(sizeof(struct task));
	memset(t, 0, sizeof(*t));

	if (t == NULL) {
		unlock_scheduler();
		return NULL;
	}

	uint64_t argc = 0;
	if (argv != NULL){
		/* Create the arguments. */

		while (*argv != NULL) {
			size_t len = strlen(*argv);
			struct task_arg *i = kmalloc(sizeof(*i));
			i->str = kmalloc(len + 1);
			memcpy(i->str, *argv, len + 1);
			i->next = NULL;

			if (t->first_arg == NULL) {
				t->first_arg = i;
				t->last_arg = i;
			} else {
				t->last_arg->next = i;
				t->last_arg = t->last_arg->next;
			}
			argv++;
			argc++;
		}
	}

	/* Now set the address space. */
	t->pml4t = pml4t;

	/* Only assign a PID to user tasks, kernel tasks don't need pids. */
	if (ring) {
		t->pid = last_pid++;
	}

	/* A queue so that tasks can wait for other tasks. */
	t->wait_queue = kmalloc(sizeof(QUEUE));
	memset(t->wait_queue, 0, sizeof(QUEUE));

	/* This sets most registers. */
	initialise_task(t, main, t->pml4t, 0x202, 0xFFFFFF7000001000 + 0x1000, 0xFFFFFF7FFFFFF000 + 0x1000, ring, argc);

	/* Link the new task we created. */
	if (first_task != NULL) {
		t->next = first_task;
		first_task = t;
	} else {
		first_task = t;
		last_task = t;
	}

	unlock_scheduler();
	return t;
}

struct task *copy_task(struct task *t) {
	/* This function creates an exact copy of the given task, all of its virtual
	 * memory etc, execpt with different physical pages,
	 * and returns the copy. (for purposes of kfork(), mostly)
	 */
	if (t == NULL) { return NULL; }

	struct task *nt = kmalloc(sizeof(*nt));
	if (nt == NULL) { return NULL; }

	/* It's important for this part to be safe. */
	lock_scheduler();

	memcpy(nt, t, sizeof(*nt));
	nt->next = NULL;
	nt->fds = NULL;
	nt->ticks_remaining = TASK_DEFAULT_TIME;
	nt->state = TASK_STATE_READY;

	/* Assign a PID. */
	nt->pid = last_pid++;

	nt->wait_queue = kmalloc(sizeof(QUEUE));
	memset(nt->wait_queue, 0, sizeof(QUEUE));

	/* This function makes an exact copy of the address space. */
	nt->pml4t = copy_addr_space(t->pml4t);
	if (nt->pml4t == NULL) {
		/* This shouldn't happen, but just in case. */
		unlock_scheduler();
		kfree(nt);
		return NULL;
	}
	nt->reg.cr3 = nt->pml4t->physical_address;

	/* Create a proper copy of the file descriptors. */
	struct file_descriptor *i = t->fds;
	while (i != NULL) {
		struct file_descriptor *nfd = vfs_create_fd(nt, i->node, i->file, i->mode);
		nfd->pos = i->pos;
		i = i->next;
	}

	/* Copy the current working directory. */
	nt->current_dir = t->current_dir;

	/* Copy the arguments. */
	struct task_arg *it = t->first_arg;
	while (it != NULL) {
		struct task_arg *j = kmalloc(sizeof(*j));
		j->str = kmalloc(strlen(it->str) + 1);
		memcpy(j->str, it->str, strlen(it->str) + 1);

		/* Add it to the list. */
		if (nt->first_arg == NULL) {
			nt->first_arg = j;
			nt->last_arg = j;
		} else {
			nt->last_arg->next = j;
			nt->last_arg = nt->last_arg->next;
		}

		it = it->next;
	}

	if (first_task == NULL) {
		first_task = nt;
		last_task = nt;
	} else {
		last_task->next = nt;
		last_task = last_task->next;
	}


	unlock_scheduler();
	return nt;
}

void terminate_task() {

	/* Close all open file descriptors. */
	struct file_descriptor *i = current_task->fds;

	while (i != NULL) {
		kclose(i->fd);
		i = i->next;
	}

	lock_task_switches();

	wait_queue(&termination_queue);
	if ((terminator_task != NULL) && (terminator_task->state == TASK_STATE_BLOCK)) {
		unblock_task(terminator_task);
	}
	unlock_task_switches();
}

void block_task() {
	lock_scheduler();
	current_task->state = TASK_STATE_BLOCK;
	yield();
	unlock_scheduler();
}

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
}

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
		return;
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
}



void scheduler_irq0() {
	/* This function will be called every time irq0 fires. */

	if (current_task == NULL) {
		/* The CPU is idle. */
		return;
	}

	/* Decrement the current task's counter. */
	if (current_task->ticks_remaining != 0) {
		lock_scheduler();	/* For protection in SMP. */
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
}



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
}

void lock_task_switches() {
	task_switch_lock++;
}


void unlock_scheduler() {
	if (sched_lock == 0) {
		return;
	}
	//serial_putx(sched_lock);
	//serial_puts("\r\n");
	sched_lock--;
}

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
}

void terminator_task_main() {
	//__asm__("cli;hlt;");
	terminator_task = current_task;

	lock_task_switches();
	while (1) {

		while (termination_queue.amount_waiting == 0) {
			block_task();
			unlock_task_switches(); /* This will trigger the block */
			lock_task_switches();   /* If we reach here, we must have been unblocked. */
		}

		/* Unlink the first task on the queue. */
		termination_queue.amount_waiting--;
		struct task *quitter = termination_queue.first_task;
		if (quitter == NULL) {
			continue;
		}

		termination_queue.first_task = termination_queue.first_task->next;
		if (termination_queue.first_task == NULL) {
			termination_queue.last_task = NULL;
		}

		while ((quitter->wait_queue != NULL) && (quitter->wait_queue->amount_waiting > 0)) {
			signal_queue(quitter->wait_queue);
		}
		kfree(quitter->wait_queue);

		if (quitter->first_arg != NULL){
			struct task_arg *i = quitter->first_arg;

			while (i != NULL) {
				if (i->str != NULL) {
					kfree(i->str);
				}
				struct task_arg *j = i;
				i = i->next;

				kfree(j);
			}

		}

		/* Now reclaim the memory of the quitter. */
		for (size_t i = 0; i < 511; i++){
			/* The last PDPT belongs to the kernel, and thus must not be freed.*/
			pd_ptr_table *pdpt = quitter->pml4t->child[i];
			if (pdpt == NULL) {
				continue;
			}
			for (size_t j = 0; j < 512; j++) {
				page_dir *pd = pdpt->child[j];
				if (pd == NULL) {
					continue;
				}
				for (size_t k = 0; k < 512; k++) {
					page_table *pt = pd->child[k];
					if (pt == NULL) {
						continue;
					}
					/* Free the physical pages as well. */
					for (size_t l = 0; l < 512; l++) {
						uint64_t entry = pt->entries[l];
						if (entry == 0) {
							continue;
						}
						uint64_t page = entry / 0x1000;
						freepp(page);

						pt->entries[i] = 0;
					}
					free_page_struct((struct page_struct*)pt);
				}
				memset(pd->entries, 0, 0x1000);
				memset(pd->child, 0, 0x1000);
				free_page_struct(pd);
			}
			memset(pdpt->entries, 0, 0x1000);
			memset(pdpt->child, 0, 0x1000);
			free_page_struct(pdpt);
		}
		memset(quitter->pml4t->entries, 0, 0x1000);
		memset(quitter->pml4t->child, 0, 0x1000);
		free_page_struct(quitter->pml4t);


		kfree(quitter);
	}
}

void idle_task_main(void) {
	while (1) {
		current_task->ticks_remaining = 1;
		__asm__("hlt;");
	}
}

uint8_t init_scheduler() {
	uint8_t stat;
	if ((stat = init_tss())) {
		return stat;
	}
	lock_scheduler();

	current_task = kmalloc(sizeof(*current_task));
	if (current_task == NULL) {
		return 1;
	}
	memset(current_task, 0, sizeof(*current_task));

	current_task->state = TASK_STATE_RUNNING;
	current_task->next = current_task;
	current_task->fds = NULL;	/* The VFS layer will initialise this. */
	current_task->ticks_remaining = TASK_DEFAULT_TIME;
	current_task->ring = 0;
	current_task->reg.kernel_rsp = (uint64_t)kmalloc(0x1000) + 0x1000 ;

	/* Initialise the global variables. */
	last_task = NULL;
	first_task = NULL;

	/* Create the terminator task, that only frees terminated tasks. */
	create_task(terminator_task_main, create_address_space(), 0, NULL);
	create_task(idle_task_main, create_address_space(), 0, NULL);

	/* Release the lock we acquired.*/
	unlock_scheduler();

	return 0;
}



#ifdef DEBUG
#include <tty.h>

void print_task(struct task *t) {
	kputx((uint64_t)t);
	kputs(": ");

	kputx(t->ticks_remaining);
	kputs(" ");
	kputx(t->state);
	kputs("\n");
}

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
