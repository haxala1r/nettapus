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

	release_semaphore(node->mutex);
	return fdes->fd;
}

int32_t vfs_open_dir(struct folder_vnode *node, struct task *t, size_t mode) {
	if (node == NULL) { return -ERR_INVALID_PARAM; }
	if (t == NULL) { return -ERR_INVALID_PARAM; }
	if (mode != FD_READ) { return -ERR_INVALID_PARAM; }

	acquire_semaphore(node->mutex);
	struct file_descriptor *fdes = vfs_create_fd(t, node, 0, mode);
	if (fdes == NULL) {
		return -1;
	}
	release_semaphore(node->mutex);
	return fdes->fd;
}



int32_t kopen(char *path, size_t mode) {
	if (path == NULL) { return -1;}

	/* This function searches the existing nodes. */
	int64_t type = 0;
	void *node = vfs_search_vnode(path, &type);

	if (node == NULL) {
		if (mode & O_CREAT) {
			node = mknode_file(path);
			type = 1; /* file. */
			if (node == NULL) {
				return -ERR_NOT_FOUND;
			}
		} else {
			return -ERR_NOT_FOUND;
		}
	}
	if (type == 0) {
		/* It's a directory. */
		return vfs_open_dir(node, get_current_task(), mode);
	}

	struct file_vnode *vnode = node;

	/* It's a file. */
	if (vnode->open == NULL) {
		return -ERR_INVALID_PARAM;
	}

	return vnode->open(node, get_current_task(), mode);
}
