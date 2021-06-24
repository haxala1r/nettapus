#include <fs/fs.h>
#include <task.h>
#include <string.h>
#include <mem.h>
#include <err.h>

struct file_descriptor *vfs_find_fd(struct task *t, int32_t fd) {
	if (t == NULL)       { return NULL; }

	struct file_descriptor *i = t->fds;
	while (i) {
		if (i->fd == fd) {
			break;
		}

		i = i->next;
	};

	return i;
}


struct file_descriptor *vfs_create_fd(struct task *t, void *node, size_t file, size_t mode) {
	if (t == NULL)    { return NULL; }
	if (node == NULL) { return NULL; }

	struct file_descriptor *new_fd = kmalloc(sizeof(*new_fd));
	memset(new_fd, 0, sizeof(*new_fd));
	new_fd->file = file;
	new_fd->node = node;
	new_fd->mode = mode;
	new_fd->next = NULL;

	if (file) {
		struct file_vnode *n = node;
		n->streams_open++;
	} else {
		struct folder_vnode *n = node;
		n->streams_open++;
	}

	/* We're modifying task structures, therefore it is appropriate to do this. */
	lock_scheduler();

	/* Assign fd, then add new_fd to t's fds list. */
	struct file_descriptor *fdi = t->fds;
	int32_t i = 0;
	if (fdi == NULL) {
		t->fds = new_fd;
		t->fds->fd = i;

		unlock_scheduler();
		return new_fd;
	}

	while (fdi != NULL) {
		/* This loop finds a suitable file descriptor number to use. */
		if (fdi->fd == i) {
			i++;
			fdi = t->fds;
			continue;
		}
		if (fdi->next == NULL) {
			break;
		}
		fdi = fdi->next;
	}
	fdi->next = new_fd;
	new_fd->fd = i;

	unlock_scheduler();
	return new_fd;
}

/* I don't even think this function is necessary, but just in case. */
int64_t ktell(int32_t fd) {
	if (fd < 0) { return -ERR_INVALID_PARAM; }

	struct file_descriptor *fdes = vfs_find_fd(get_current_task(), fd);

	return fdes->pos;
}

int64_t kseek(int32_t fd, int64_t pos) {
	if (fd < 0)  { return -ERR_INVALID_PARAM; }
	if (pos < 0) { return -ERR_INVALID_PARAM; }

	struct file_descriptor *fdes = vfs_find_fd(get_current_task(), fd);
	if (fdes->file == 0) {
		return 0;
	}

	struct file_vnode *node = fdes->node;
	if (node == NULL) {
		return -ERR_INVALID_PARAM;
	}

	if ((size_t)pos >= node->size) {
		fdes->pos = node->size - 1;
	} else {
		fdes->pos = pos;
	}

	return fdes->pos;
}
