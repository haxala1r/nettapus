#include <fs/fs.h>
#include <task.h>

/* Now we need to include the headers for the FS drivers. */
#include <fs/ustar.h>
#include <fs/fat16.h>


int64_t write_file(struct file *f, void *buf, size_t bytes) {
	/* This is the generic write function for files. */

	/* Some NULL checks. we don't want the system to crash while writing to a file do we? */
	if (f == NULL) 				{ return -1; }
	if (f->node == NULL) 		{ return -1; }
	if (f->node->fs == NULL) 	{ return -1; }
	if (f->mode != 1) 			{ return -1; }

	acquire_semaphore(f->node->mutex);

	uint8_t status = f->node->fs->driver->write(f->node->fs, f->node->special, \
	                                            buf, f->position, bytes);

	/* Write should be complete, check for error, update stuff, then return. */
	if (status) {
		/* Error. */
		release_semaphore(f->node->mutex);
		return -status;
	}
	f->position += bytes;

	release_semaphore(f->node->mutex);
	return bytes;
};



int64_t write_pipe(struct file *f, void *buf, size_t bytes) {
	/* This is the generic write function for pipes. It writes to a pipe
	 * and blocks if there isn't space on the pipe.
	 */

	if (f == NULL) 						{ return -1; }
	if (f->node == NULL) 				{ return -1; }
	if (f->node->special == NULL) 		{ return -1; }
	if (f->node->mutex == NULL) 		{ return -1; }
	if (buf == NULL) 					{ return -1; }

	if (bytes <= 0) 					{ return  0; }
	if (f->node->reader_count == 0) 	{ return -1; }

	acquire_semaphore(f->node->mutex);

	/* The upcoming loop modifies "bytes".*/
	size_t to_read = bytes;

	/* Block if there isn't enough space. */
	while (bytes > (DEFAULT_PIPE_SIZE - f->node->last)) {
		if (f->flags & FILE_FLAG_NONBLOCK) {
			release_semaphore(f->node->mutex);
			return 0;
		}

		/* This waits until there's room on the pipe. */
		if (f->node->last == DEFAULT_PIPE_SIZE) {
			/* Add ourselves to the queue without blocking yet. */
			lock_task_switches();
			wait_queue(f->node->write_queue);

			release_semaphore(f->node->mutex);

			/* Now we can block as normal. */
			unlock_task_switches();

			/* We got unblocked, there's room to write to. */
			acquire_semaphore(f->node->mutex);
		}

		if (bytes > (DEFAULT_PIPE_SIZE - f->node->last)) {
			memcpy(f->node->special + f->node->last, buf, DEFAULT_PIPE_SIZE - f->node->last);

			buf += (DEFAULT_PIPE_SIZE - f->node->last);
			bytes -= (DEFAULT_PIPE_SIZE - f->node->last);
			f->node->last = DEFAULT_PIPE_SIZE;

			if (bytes <= 0) {
				release_semaphore(f->node->mutex);
				return 0;
			}
		}
	}

	/* This handles the rest */
	memcpy(f->node->special + f->node->last, buf, bytes);
	f->node->last += bytes;

	release_semaphore(f->node->mutex);
	signal_queue(f->node->read_queue);
	return to_read;
};

int64_t write(struct task *t, int32_t file_des, void *buf, size_t bytes) {
	/* This function writes to a file/node, at the position file_des indicates. */

	if (t == NULL)			{ return -1; };

	struct file *f = vfs_fd_lookup(t, file_des);
	if (f == NULL)				{ return -2; };
	if (f->node == NULL)		{ return -3; };
	if (f->mode != FD_WRITE) 	{ return -1; };

	return f->node->write(f, buf, bytes);
};

int64_t kwrite(int32_t file_des, void *buf, size_t bytes) {
	return write(get_current_task(), file_des, buf, bytes);
};
