#include <task.h>

/*
 * This file implements a resource queue.
 * For example, when a pipe that doesn't have data on it is attempted to
 * be read, the caller would wait until there's some data to read. That is
 * what a resource queue is trying to do.
 *
 * Queues are different than Semaphores in that a process will block until
 * an event occurs (in our previous example, until another process writes to
 * the pipe), while a semaphore will wait until the process can gain exclusive
 * access to a certain resource.
 */

void wait_queue(QUEUE *q) {
	/* This function simply puts the caller process in the queue, and blocks. */

	if (q == NULL) {
		return;
	}
	/* Queues should also have spinlocks when SMP is added. */
	lock_task_switches();

	q->amount_waiting++;
	if (q->first_task == NULL) {
		q->first_task = get_current_task();
		q->last_task = q->first_task;
	} else {
		q->last_task->next = get_current_task();
		q->last_task = q->last_task->next;
	}

	block_task();

	unlock_task_switches();
	return;
};


void signal_queue(QUEUE *q) {
	/* This function simply wakes up the next task on the queue to inform it
	 * that more data is available.
	 */

	if (q->amount_waiting == 0) {
		return;
	}

	lock_task_switches();

	q->amount_waiting--;
	if (q->first_task != NULL) {
		unblock_task(q->first_task);
		q->first_task = q->first_task->next;
		if (q->first_task == NULL) {
			q->last_task = NULL;
		}
	}

	unlock_task_switches();
	return;
}



