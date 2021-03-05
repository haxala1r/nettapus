
#include <task.h>
#include <mem.h>


SEMAPHORE *create_semaphore(int32_t max_count) {
	SEMAPHORE *s = kmalloc(sizeof(SEMAPHORE));
	
	if (s == NULL) { return NULL; };
	
	s->max_count 			= max_count;
	s->current_count 		= 0;
	s->first_waiting_task 	= NULL;
	s->last_waiting_task	= NULL;
	
	return s;
};


/* TODO: When SMP support is added, these should be guarded by a spinlock. */

void acquire_semaphore(SEMAPHORE *semaphore) {
	
	/* This is where the spinlock would go, but we don't have
	 * SMP, so this works well enough. 
	 */
	lock_task_switches();
	
	
	/* If the semaphore is already at its limit, block. */
	if (semaphore->current_count >= semaphore->max_count) {
	
		/* First, we add the currently running task to the list of waiting
		 * tasks for that semaphore.
		 */
		if (semaphore->last_waiting_task == NULL) {
			semaphore->first_waiting_task = get_current_task();
			semaphore->last_waiting_task = get_current_task();
		} else {
			semaphore->last_waiting_task->next = get_current_task();
			semaphore->last_waiting_task = get_current_task();
		}
		
		/* Now we block. When the task that is currently using this
		 * semaphore finishes its job, it will call release_semaphore(),
		 * and that will eventually unblock this task.
		 * 
		 * Note that we don't increment current_count here, this is because
		 * if we did, the task that unblocks this one is going to have to 
		 * decrement it and the value is going to stay the same. 
		 */
		
		block_task();
	} else {
		/* The semaphore is available. */
		semaphore->current_count++;
	}
	
	/* This is where the lock for this semaphore would be released. */
	unlock_task_switches();
	
	return;
};



void release_semaphore(SEMAPHORE *semaphore) {
	
	/* This is where the spinlock would go, but we don't have
	 * SMP, so this works well enough. 
	 */
	lock_task_switches();
	
	if (semaphore->first_waiting_task == NULL) {
		semaphore->current_count--;
	} else {
		/* Unblock the first waiting task. */
		unblock_task(semaphore->first_waiting_task);
		
		/* Unlink the task we just unblocked. */
		semaphore->first_waiting_task = semaphore->first_waiting_task->next;
		if (semaphore->first_waiting_task == NULL) {
			semaphore->last_waiting_task = NULL;
		}
	}
	
	/* This is where the lock for this semaphore would be released. */
	unlock_task_switches();
	
	return;
};
