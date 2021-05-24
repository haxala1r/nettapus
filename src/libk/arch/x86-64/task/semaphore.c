
#include <task.h>
#include <mem.h>


SEMAPHORE *create_semaphore(int32_t max_count) {
	SEMAPHORE *s = kmalloc(sizeof(SEMAPHORE));

	if (s == NULL) { return NULL; }

	memset(s, 0, sizeof(*s));

	s->max_count 			= max_count;
	s->current_count 		= 0;
	s->first_waiting_task 	= NULL;
	s->last_waiting_task	= NULL;

	return s;
}


/* TODO: When SMP support is added, these should be guarded by a spinlock. */

void acquire_semaphore(SEMAPHORE *s) {
	if (s == NULL) { return; }
	/* This is where the spinlock would go, but we don't have
	 * SMP, so this works well enough.
	 */
	lock_scheduler();
	lock_task_switches();


	/* If the semaphore is already at its limit, block. */
	if (s->current_count >= s->max_count) {

		/* First, we add the currently running task to the list of waiting
		 * tasks for that semaphore.
		 */
		if (s->last_waiting_task == NULL) {
			s->first_waiting_task = get_current_task();
			s->last_waiting_task = get_current_task();
		} else {
			s->last_waiting_task->next = get_current_task();
			s->last_waiting_task = get_current_task();
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
		s->current_count++;
	}

	/* This is where the lock for this semaphore would be released. */
	unlock_scheduler();
	unlock_task_switches();

	return;
}



void release_semaphore(SEMAPHORE *s) {
	if (s == NULL) { return; }
	/* This is where the spinlock would go, but we don't have
	 * SMP, so this works well enough.
	 */
	lock_task_switches();

	if (s->first_waiting_task == NULL) {
		s->current_count--;
	} else {
		/* Unblock the first waiting task. */
		unblock_task(s->first_waiting_task);

		/* Unlink the task we just unblocked. */
		s->first_waiting_task = s->first_waiting_task->next;
		if (s->first_waiting_task == NULL) {
			s->last_waiting_task = NULL;
		}
	}

	/* This is where the lock for this semaphore would be released. */
	unlock_task_switches();

	return;
}



void destroy_semaphore(SEMAPHORE *s) {
	if (s == NULL) { return; }

	/* THERE IS A BUG HERE. FIXME!
	 * This is a serious race condition, if another process tries to acquire()
	 * this semaphore they will stay blocked forever. THIS SHOULD BE FIXED.
	 * see destroy_queue() in queues.c for ideas on how to fix it.
	 */

	acquire_semaphore(s);
	lock_scheduler();

	kfree(s);
}

