#ifndef TASK_H
#define TASK_H 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <mem.h>


#define TASK_DEFAULT_TIME 50

/* Some values for the task's state attribute. */

/* The task is currently being run. */
#define TASK_STATE_RUNNING	1

/* Ready to run. */
#define TASK_STATE_READY		2

/* The task is blocking. */
#define TASK_STATE_BLOCK		3



struct file_descriptor; /* Definition in <fs/fs.h>*/
struct task_registers {
	/* General purpose registers. The offsets of all registers are listed to
	 * make things easier.
	 */
	uint64_t rax;  // 0x0
	uint64_t rbx;  // 0x8
	uint64_t rcx;  // 0x10
	uint64_t rdx;  // 0x18
	uint64_t rdi;  // 0x20
	uint64_t rsi;  // 0x28
	uint64_t r8;   // 0x30
	uint64_t r9;   // 0x38
	uint64_t r10;  // 0x40
	uint64_t r11;  // 0x48
	uint64_t r12;  // 0x50
	uint64_t r13;  // 0x58
	uint64_t r14;  // 0x60
	uint64_t r15;  // 0x68

	/* Stack and base. */
	uint64_t rsp;  // 0x70
	uint64_t rbp;  // 0x78

	/* RIP, CR3 etc.*/
	uint64_t rip;  // 0x80
	uint64_t cr3;  // 0x88
	uint64_t rflags;// 0x90

	/* These are saved as uint64_t because the task_switch function needs to
	 * push them as uint64_t's.
	 */
	uint64_t cs;   // 0x98
	uint64_t ds;   /* 0xA0 This is also used for ss, es, fs, gs.*/

	uint64_t kernel_rsp; /* 0xA8 Kernel stack for the task. */
	//char fxsave_area[544]; // 0xB0
} __attribute__((packed));


struct task {
	struct task_registers reg;

	/* File descriptors open for this process.  */
	struct file_descriptor *fds;

	uint64_t ticks_remaining;
	uint64_t state;
	size_t ring;

	p_map_level4_table *pml4t;
	struct task *next;
};


struct semaphore {
	size_t max_count;
	size_t current_count;

	struct task *first_waiting_task;
	struct task *last_waiting_task;
};


struct queue {
	/* The structure itself is pretty similar to Semaphore, but its purpose
	 * and the functions used to manipulate it are different.
	 */
	size_t amount_waiting;

	struct task *first_task;
	struct task *last_task;
};

/* These structs are opaque, so it's okay to typedef them. */
typedef struct semaphore SEMAPHORE;
typedef struct queue QUEUE;

uint8_t create_task(void (*)(), size_t ring);
uint8_t init_tss();
uint8_t init_scheduler();
struct task *get_current_task();
void scheduler_irq0();
void yield();

void block_task();
void unblock_task(struct task *t);


/* Some stuff to prevent being preempted in the middle of a critical section. */
void lock_scheduler();
void unlock_scheduler();
void lock_task_switches();
void unlock_task_switches();

/* Some stuff for process syncronization. */
SEMAPHORE *create_semaphore(int32_t max_count);
void acquire_semaphore(SEMAPHORE *s);
void release_semaphore(SEMAPHORE *s);
void destroy_semaphore(SEMAPHORE *s);

/* Some stuff to make it easier to have processes wait on a resource. */
void wait_queue(QUEUE *q);
void signal_queue(QUEUE *q);
void destroy_queue(QUEUE *q);


/* These two are the task-switching functions that load/save registers etc. */
extern void switch_task(struct task_registers *from, struct task_registers *to);
extern void task_loader(struct task *t);

#ifdef DEBUG
void print_tasks();
#endif


#ifdef __cplusplus
}
#endif

#endif
