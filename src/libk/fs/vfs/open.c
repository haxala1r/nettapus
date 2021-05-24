#include <fs/fs.h>
#include <task.h>
#include <err.h>
#include <mem.h>

int32_t vfs_open_file(struct file_vnode *node, struct task *t, size_t mode) {
	if (node == NULL) { return -ERR_INVALID_PARAM; }
	if (t == NULL)    { return -ERR_INVALID_PARAM; }

	acquire_semaphore(node->mutex);
	struct file_descriptor *fdes = vfs_create_fd(t, node, 1, mode);
	if (fdes == NULL) {
		return -1;
	}

	node->streams_open++;

	release_semaphore(node->mutex);
	return fdes->fd;
}


int32_t kopen(char *path, size_t mode) {
	if (path == NULL) { return -1;}

	/* This function searches the existing nodes. */
	struct file_vnode *node = vfs_search_file_vnode(path);
	/* TODO: also search for a folder vnode here. */

	if (node == NULL) {
		/* The file or folder doesn't exist. */
		if (mode == 0) {
			/* If the create flag hasn't been set, return an error. */
			return -ERR_NO_RESULT;
		}

		/* We need to create a new node. */
		node = mknode_file(path);
	}
	if (node->open == NULL) {
		return -1;
	}
	return node->open(node, get_current_task(), mode);
}
