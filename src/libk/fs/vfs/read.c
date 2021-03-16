
#include <fs/fs.h>
#include <mem.h>
#include <task.h>

/* Now we need to include the headers for the FS drivers. */
#include <fs/ustar.h>
#include <fs/fat16.h>


int64_t read_file(struct file *f, void *buf, size_t bytes) {
	/* This is the generic read function for files. It loads the data from disk,
	 * and copies it to buf.
	 */

	if (f == NULL)                    { return -1; }
	if (f->node == NULL)              { return -1; }
	if (f->node->fs == NULL)          { return -1; }
	if (f->node->fs->driver == NULL)  { return -1; }
	if (f->node->mutex == NULL)       { return -1; }
	if (f->mode != 0)                 { return -1; }

	acquire_semaphore(f->node->mutex);

	/* Determine the amount of bytes that can be read */
	size_t to_read = bytes;
	if ((f->position + to_read) > f->node->last) {
		to_read = f->node->last - f->position;
	}

	uint8_t status = f->node->fs->driver->read(f->node->fs, f->node->special, buf, f->position, to_read);

	if (status) {
		release_semaphore(f->node->mutex);
		return -status;
	}

	/* Return the amount of bytes that were actually read.*/
	f->position += to_read;

	release_semaphore(f->node->mutex);
	return to_read;
};


int64_t read_pipe(struct file *f, void *buf, size_t bytes) {
	/* This is the generic read function used for pipes. If the request
	 * size exceeds the pipe's size, or the requested amount of data has
	 * not been written to the pipe, the process will be blocked.
	 */

	/* Some checks to see if the request is valid. */
	if (f == NULL)                      { return -1; }
	if (f->node == NULL)                { return -1; }
	if (f->node->special == NULL)       { return -1; }
	if (f->node->mutex == NULL)         { return -1; }
	if ((f->node->type != FILE_PIPE_NAMED) && (f->node->type != FILE_PIPE_UNNAMED)) {
		return -1;
	}
	if (buf == NULL)                    { return -1; }
	if (bytes <= 0)                     { return  0; }
	/* TODO: Handle broken pipes better. */
	if (f->node->writer_count == 0)     { return -1; }

	acquire_semaphore(f->node->mutex);

	/* The upcoming loop modifies "bytes". */
	size_t to_read = bytes;

	/* Block if there isn't enough data to read. */
	while (bytes > f->node->last) {
		if (f->flags & FILE_FLAG_NONBLOCK) {
			release_semaphore(f->node->mutex);
			return 0;
		}

		/* This waits until data has been written to the pipe. */
		if (f->node->last == 0) {

			/* Add ourselves to the queue without blocking yet. */
			lock_task_switches();
			wait_queue(f->node->read_queue);

			release_semaphore(f->node->mutex);

			/* Now we can block as normal. */
			unlock_task_switches();

			/* We got unblocked, and there's data to read. */
			acquire_semaphore(f->node->mutex);
		}

		if (bytes > f->node->last) {
			memcpy(buf, f->node->special, f->node->last);
			buf += f->node->last;
			bytes -= f->node->last;
			f->node->last = 0;
		}
	}

	/* Copy the data to caller's buffer. */
	memcpy(buf, f->node->special, bytes);

	/* Remove the data that was read from the pipe.*/
	memcpy(f->node->special, f->node->special + bytes, DEFAULT_PIPE_SIZE - bytes);

	f->node->last -= bytes;

	release_semaphore(f->node->mutex);
	signal_queue(f->node->write_queue);
	return to_read;
};


int64_t read(struct task *t, int32_t file_des, void* buf, size_t bytes) {
	/* This function simply finds the node associated with the given
	 * file descriptor and calls its read function.
	 */

	if (t == NULL) 			{ return -1; }

	struct file *f = vfs_fd_lookup(t, file_des);
	if (f == NULL) 				{ return -1; }
	if (f->node == NULL)		{ return -1; }
	if (f->mode != FD_READ)		{ return -1;}

	return f->node->read(f, buf, bytes);
};

int64_t kread(int32_t file_des, void *buf, size_t bytes) {
	return read(get_current_task(), file_des, buf, bytes);
};
