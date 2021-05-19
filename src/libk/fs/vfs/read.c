#include <fs/fs.h>
#include <err.h>
#include <task.h>
#include <string.h>

int64_t vfs_read_file(struct file_descriptor *fdes, void *buf, int64_t amount) {
	if (fdes == NULL)       { return -ERR_INVALID_PARAM; }
	if (buf == NULL)        { return -ERR_INVALID_PARAM; }
	if (fdes->node == NULL) { return -ERR_INVALID_PARAM; }

	struct file_vnode *node = fdes->node;
	acquire_semaphore(node->mutex);

	/* Determine the exact amount we can read. */
	int64_t to_read = amount;
	if ((to_read + fdes->pos) > node->size) {
		to_read = node->size - fdes->pos;
	}

	/* Request the filesystem driver to read from disk. */
	int64_t stat = node->fs->driver->read(node->fs, node->inode_num, buf, fdes->pos, to_read);

	release_semaphore(node->mutex);
	if (stat > 0) {
		fdes->pos += stat;
	}
	return stat;
};



int64_t vfs_read_pipe(struct file_descriptor *fdes, void *buf, int64_t amount) {
	if (fdes == NULL)       { return -ERR_INVALID_PARAM; }
	if (buf == NULL)        { return -ERR_INVALID_PARAM; }
	if (fdes->node == NULL) { return -ERR_INVALID_PARAM; }

	struct file_vnode *node = fdes->node;
	acquire_semaphore(node->mutex);
	int64_t ret = 0;
	while (amount > 0) {
		/* For pipes, the size attribute indicates the amount of data stored. */
		int64_t to_read = ((int64_t)node->size > amount) ? amount : (int64_t)node->size;

		if (to_read == 0) {
			/* There isn't enough data on the pipe yet. */
			lock_task_switches();

			wait_queue(node->read_queue);
			release_semaphore(node->mutex);
			signal_queue(node->write_queue);

			unlock_task_switches();
			acquire_semaphore(node->mutex);
			continue;
		}

		/* Copy the data from the pipe to the buffer. */
		memcpy(buf, node->pipe_mem, to_read);

		/* Remove the data read from the pipe. */
		memcpy(node->pipe_mem, node->pipe_mem + to_read, DEFAULT_PIPE_SIZE - to_read);

		amount -= to_read;
		node->size -= to_read;
		buf += to_read;
		ret += to_read;
	}

	signal_queue(node->write_queue);
	release_semaphore(node->mutex);
	return ret;
};



int64_t kread(int32_t fd, void *buf, int64_t amount) {
	if (buf == NULL) { return -ERR_INVALID_PARAM; }
	if (fd < 0)      { return -ERR_INVALID_PARAM; }

	struct file_descriptor *fdes = vfs_find_fd(get_current_task(), fd);
	if (fdes == NULL) { return -ERR_INVALID_PARAM; }
	if (!fdes->file)   { return -ERR_INVALID_PARAM; }

	struct file_vnode *fnode = fdes->node;

	return fnode->read(fdes, buf, amount);
};

