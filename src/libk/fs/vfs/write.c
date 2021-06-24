#include <fs/fs.h>
#include <task.h>
#include <string.h>
#include <err.h>

int64_t vfs_write_file(struct file_descriptor *fdes, void *buf, int64_t amount) {
	if (fdes == NULL)       { return -ERR_INVALID_PARAM; }
	if (buf == NULL)        { return -ERR_INVALID_PARAM; }
	if (fdes->node == NULL) { return -ERR_INVALID_PARAM; }

	if (fdes->mode != FD_WRITE) {
		return -ERR_INVALID_PARAM;
	}

	struct file_vnode *node = fdes->node;
	acquire_semaphore(node->mutex);

	/* Determine the exact amount we can write. TODO: enlarge file. */
	int64_t to_write = amount;
	if ((to_write + fdes->pos) > node->size) {
		to_write = node->size - fdes->pos;
	}

	/* Request the filesystem driver to write to disk. */
	int64_t stat = node->fs->driver->write(node->fs, node->inode_num, buf, fdes->pos, to_write);

	release_semaphore(node->mutex);
	if (stat > 0) {
		fdes->pos += stat;
	}
	return stat;
}


int64_t vfs_write_pipe(struct file_descriptor *fdes, void *buf, int64_t amount) {
	if (fdes == NULL)       { return -ERR_INVALID_PARAM; }
	if (buf == NULL)        { return -ERR_INVALID_PARAM; }
	if (fdes->node == NULL) { return -ERR_INVALID_PARAM; }

	if (fdes->mode != FD_WRITE) {
		return -ERR_INVALID_PARAM;
	}

	struct file_vnode *node = fdes->node;
	acquire_semaphore(node->mutex);

	int64_t ret = 0;
	while (amount > 0) {
		/* For pipes, the size attribute indicates the amount of data stored. */
		int64_t to_write = ((int64_t)(DEFAULT_PIPE_SIZE - node->size) > amount) ? amount : (int64_t)(DEFAULT_PIPE_SIZE - node->size);

		if (to_write == 0) {
			/* There isn't enough space on the pipe. Wait on a queue. */
			lock_task_switches();

			wait_queue(node->write_queue);
			release_semaphore(node->mutex);
			signal_queue(node->read_queue);

			unlock_task_switches();
			acquire_semaphore(node->mutex);
			continue;
		}

		/* Copy the data from the buffer to the pipe. */
		memcpy((uint8_t *)node->pipe_mem + node->size, buf, to_write);

		amount -= to_write;
		node->size += to_write;
		buf = (uint8_t *)buf + to_write;
		ret += to_write;
	}

	signal_queue(node->read_queue);
	release_semaphore(node->mutex);
	return ret;
}

int64_t kwrite(int32_t fd, void *buf, int64_t amount) {
	if (buf == NULL) { return -ERR_INVALID_PARAM; }
	if (fd < 0)      { return -ERR_INVALID_PARAM; }

	struct file_descriptor *fdes = vfs_find_fd(get_current_task(), fd);
	if (fdes == NULL) { return -ERR_INVALID_PARAM; }
	if (!fdes->file)   { return -ERR_INVALID_PARAM; }

	struct file_vnode *fnode = fdes->node;
	if (fnode == NULL)        { return -ERR_INVALID_PARAM; }
	if (fnode->write == NULL) { return -ERR_INVALID_PARAM; }

	return fnode->write(fdes, buf, amount);
}
