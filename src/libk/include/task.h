#ifndef TASK_H
#define TASK_H 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <fs/fs.h>

struct task_registers {
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
	struct task_registers reg;
	
	/* File descriptors open for this process.  */
	struct file_s *files;		
	
	struct Task *next;
};


typedef struct Task Task;
typedef struct task_registers task_reg;

uint8_t create_task(void (*)());
uint8_t init_scheduler();
Task *get_current_task();
void schedule();

/* Some stuff to prevent being preempted in the middle of a critical section. */
void lock_scheduler();
void unlock_scheduler();

/* This is the low-level task switching function. It saves the registers to the first
 * parameter, and loads the new registers from the second parameter. */
extern void switch_task(task_reg *from, task_reg *to);


#ifdef __cplusplus
}
#endif

#endif
