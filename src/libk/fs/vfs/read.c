#include <fs/fs.h>
#include <err.h>
#include <task.h>
#include <string.h>

int64_t vfs_read_file(struct file_descriptor *fdes, void *buf, int64_t amount) {
	if (fdes == NULL)       { return -ERR_INVALID_PARAM; }
	if (buf == NULL)        { return -ERR_INVALID_PARAM; }
	if (fdes->node == NULL) { return -ERR_INVALID_PARAM; }
	if (amount == 0)        { return 0;}

	if (fdes->mode != FD_READ) {
		return -ERR_INVALID_PARAM;
	}

	struct file_vnode *node = fdes->node;
	acquire_semaphore(node->mutex);

	/* Determine the exact amount we can read. */
	int64_t to_read = amount;
	if ((to_read + fdes->pos) > node->size) {
		to_read = node->size - fdes->pos;
	}

	if (to_read == 0) {
		release_semaphore(node->mutex);
		return -ERR_EOF;
	}

	/* Request the filesystem driver to read from disk. */
	int64_t stat = node->fs->driver->read(node->fs, node->inode_num, buf, fdes->pos, to_read);

	release_semaphore(node->mutex);
	if (stat > 0) {
		fdes->pos += stat;
	}
	return stat;
}


int64_t vfs_read_pipe(struct file_descriptor *fdes, void *buf, int64_t amount) {
	if (fdes == NULL)       { return -ERR_INVALID_PARAM; }
	if (buf == NULL)        { return -ERR_INVALID_PARAM; }
	if (fdes->node == NULL) { return -ERR_INVALID_PARAM; }

	if (fdes->mode != FD_READ) {
		return -ERR_INVALID_PARAM;
	}

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

			/* This causes block, and then unblock when the read queue is signalled
			 * aka someone wrote to the pipe.
			 */
			unlock_task_switches();
			acquire_semaphore(node->mutex);
			continue;
		}

		/* Copy the data from the pipe to the buffer. */
		memcpy(buf, node->pipe_mem, to_read);

		/* Remove the data read from the pipe. */
		memcpy(node->pipe_mem, (uint8_t *)node->pipe_mem + to_read, DEFAULT_PIPE_SIZE - to_read);

		amount -= to_read;
		node->size -= to_read;
		buf = (uint8_t *)buf + to_read;
		ret += to_read;
	}

	signal_queue(node->write_queue);
	release_semaphore(node->mutex);
	return ret;
}

int64_t vfs_read_dir(struct file_descriptor *fdes, void *buf, int64_t amount) {
	if (fdes == NULL)       { return -ERR_INVALID_PARAM; }
	if (buf == NULL)        { return -ERR_INVALID_PARAM; }
	if (fdes->node == NULL) { return -ERR_INVALID_PARAM; }
	if ((size_t)amount < sizeof(struct dirent)) { return -ERR_EOF; }

	/* Returns <0 when EOF. Returns >0 when read is successful. 0 is undefined,
	 * but it should be treated like an error.
	 *
	 * amount is the amount of memory reserved for this call. This is necessary
	 * because the name field of the dirent struct has an undefined length, and
	 * if a limit isn't defined then this call could end up overwriting important
	 * data. Keep in mind that a partial read is treated the same way as a
	 * complete one, so the only way to attempt a re-read is to use lseek() then
	 * read again with more memory.
	 */

	/* Reads the next directory entry to buf.*/
	struct folder_vnode *node = fdes->node;
	if (node->mounted) {
		node = node->mount_point->vnode;
		if (node == NULL) {
			return -ERR_INVALID_PARAM;
		}
	}
	if (fdes->pos >= (node->subfolder_count + node->subfile_count)) { return -ERR_INVALID_PARAM; }
	acquire_semaphore(node->mutex);

	/* We'll be writing the information to the user's buffer. */
	struct dirent *ptr = buf;

	/* Find the node that corresponds to the current */
	if (fdes->pos < node->subfolder_count) {
		/* Return a folder's dirent. */
		struct folder_tnode *ret_node = node->subfolders;
		for (size_t i = 0; i < fdes->pos; i++) {
			if (ret_node == NULL) {
				release_semaphore(node->mutex);
				return -ERR_INVALID_PARAM;
			}
			ret_node = ret_node->next;
		}
		fdes->pos++;

		int64_t name_len = strlen(ret_node->folder_name) + 1;

		/* The Vnode might not have been loaded yet. */
		if (ret_node->vnode == NULL) {
			if (vfs_load_folder_at(node, ret_node) == NULL) {
				release_semaphore(node->mutex);
				return -ERR_NO_RESULT;
			}
		}

		ptr->inode = ret_node->vnode->inode_num;
		ptr->type = ret_node->vnode->type_perm;
		ptr->len = sizeof(*ptr) + name_len - 1;

		memcpy(ptr->name, ret_node->folder_name, (name_len > amount) ? amount : name_len);

		release_semaphore(node->mutex);
		return ptr->len;
	}

	/* We have to return a file's dirent. */
	struct file_tnode *ret_node = node->subfiles;

	for (size_t i = 0; i < (fdes->pos - node->subfolder_count); i++) {
		if (ret_node == NULL) {
			release_semaphore(node->mutex);
			return -ERR_INVALID_PARAM;
		}
		ret_node = ret_node->next;
	}
	fdes->pos++;

	int64_t name_len = strlen(ret_node->file_name) + 1;

	/* The Vnode might not have been loaded yet. */
	if (ret_node->vnode == NULL) {
		if (vfs_load_file_at(node, ret_node) == NULL) {
			release_semaphore(node->mutex);
			return -ERR_NO_RESULT;
		}
	}

	ptr->inode = ret_node->vnode->inode_num;
	ptr->type = ret_node->vnode->type_perm;
	ptr->len = sizeof(*ptr) + name_len - 1;
	memcpy(ptr->name, ret_node->file_name, (name_len > amount) ? amount : name_len);

	release_semaphore(node->mutex);
	/* This part definitely generates a PF but halting here doesn't tell anything.
	 * Perhaps it's best to single-step through it.
	 */
	return ptr->len;
}

int64_t kread(int32_t fd, void *buf, int64_t amount) {
	if (buf == NULL) { return -ERR_INVALID_PARAM; }
	if (fd < 0)      { return -ERR_INVALID_PARAM; }

	struct file_descriptor *fdes = vfs_find_fd(get_current_task(), fd);
	if (fdes == NULL) { return -ERR_INVALID_PARAM; }
	if (fdes->file == FILE_DIR)   {
		return vfs_read_dir(fdes, buf, amount);
	}

	struct file_vnode *fnode = fdes->node;
	if (fnode == NULL)       { return -ERR_INVALID_PARAM; }
	if (fnode->read == NULL) { return -ERR_INVALID_PARAM; }

	return fnode->read(fdes, buf, amount);
}
