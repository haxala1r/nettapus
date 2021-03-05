#ifndef TASK_H
#define TASK_H 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>



#define TASK_DEFAULT_TIME 50

/* Some values for the task's state attribute. */

/* The task is currently being run. */
#define TASK_STATE_RUNNING	1

/* Ready to run. */
#define TASK_STATE_READY		2

/* The task is blocking. */
#define TASK_STATE_BLOCK		3



struct file_s;
struct Task_registers {
	/* General purpose registers. */
	uint64_t rax, rbx, rcx, rdx, rdi, rsi, r8, r9, r10, r11, r12, r13, r14, r15;
	
	/* Stack and base. */
	uint64_t rsp, rbp;
	
	/* RIP, CR3 etc.*/
	uint64_t rip, cr3, rflags;
	
	/* This is here to make sure the starting address of fxsave_area is 16-byte aligned.
	 * Though there isn't much point in that, as the area is still not guaranteed to be
	 * aligned if the starting address of the structure isn't 16-byte aligned. */
	uint64_t __filler;	
	
	char __attribute__((aligned(16))) fxsave_area[544];
} __attribute__((packed));


struct Task {
	struct Task_registers reg;
	
	/* File descriptors open for this process.  */
	struct file_s *files;		
	
	/* The amount of time this task has remaining (in ticks) */
	uint64_t ticks_remaining; 
	
	/* The current state of the task (whether it can run or not etc.) */
	uint64_t state;
	
	/* This is here for a linked list. */
	struct Task *next;
};


struct Semaphore {
	size_t max_count;
	size_t current_count;
	
	struct Task *first_waiting_task;
	struct Task *last_waiting_task;
};



typedef struct Task 			TASK;
typedef struct Semaphore 		SEMAPHORE;
typedef struct Task_registers 	TASK_REG;

uint8_t create_task(void (*)());
uint8_t init_scheduler();
TASK *get_current_task();
void scheduler_irq0();
void yield();

void block_task();
void unblock_task();


/* Some stuff to prevent being preempted in the middle of a critical section. */
void lock_scheduler();
void unlock_scheduler();
void lock_task_switches();
void unlock_task_switches();

/* Some stuff for process syncronization. */
SEMAPHORE *create_semaphore(int32_t max_count);
void acquire_semaphore(SEMAPHORE *semaphore);
void release_semaphore(SEMAPHORE *semaphore);



/* This is the low-level task switching function. It saves the registers to the first
 * parameter, and loads the new registers from the second parameter. */
extern void switch_task(TASK_REG *from, TASK_REG *to);


#ifdef DEBUG
void print_tasks();
#endif	


#ifdef __cplusplus
}
#endif

#endif