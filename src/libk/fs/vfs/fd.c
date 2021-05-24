#include <fs/fs.h>
#include <task.h>
#include <string.h>
#include <mem.h>

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
	while (fdi) {
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
