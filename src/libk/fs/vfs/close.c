#include <fs/fs.h>
#include <err.h>
#include <task.h>
#include <mem.h>

int32_t vfs_close_file(struct task *t, struct file_descriptor *fd) {
	if (fd == NULL)     { return -ERR_INVALID_PARAM; }
	if (t == NULL)      { return -ERR_INVALID_PARAM; }
	if (t->fds == NULL) { return -ERR_INVALID_PARAM; }

	struct file_descriptor *i = t->fds;

	while ((i != NULL) && (i->next != fd)) {
		i = i->next;
	}

	if (i == NULL) {
		/* The file descriptor was not found. */
		return -ERR_INVALID_PARAM;
	}

	/* i points to the element before fd. */
	i->next = i->next->next; /* Unlink fd. */

	/* Decrease stream count, and unload the node if that leaves it unused. */
	if (fd->file) {
		struct file_vnode *node = fd->node;
		node->streams_open--;
		if ((node->streams_open == 0) && (node->cached_links == 0)) {
			// Unload node.
			vfs_unload_fnode(node);
		}
	} else {
		struct folder_vnode *node = fd->node;
		node->streams_open--;
		if ((node->streams_open == 0) && (node->cached_links == 0)) {
			vfs_unload_dnode(node);
		}
	}

	kfree(fd);

	return GENERIC_SUCCESS;
};


int32_t kclose(int32_t fd) {
	if (fd < 0) { return -ERR_INVALID_PARAM; }

	struct file_descriptor *fdes = vfs_find_fd(get_current_task(), fd);
	if (fdes == NULL) { return -ERR_INVALID_PARAM; }

	if (fdes->file) {
		struct file_vnode *fnode = fdes->node;

		return fnode->close(get_current_task(), fdes);
	} else {
		/* TODO */
	}
	return GENERIC_SUCCESS;
};

