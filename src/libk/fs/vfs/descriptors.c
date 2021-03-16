#include <fs/fs.h>
#include <task.h>
#include <mem.h>

int32_t vfs_create_descriptor(struct task *t, struct file_vnode *n,
                              uint8_t mode, uint8_t flags) {
	/* Creates a new file descriptor for a given node.
	 * NOTE: Caller is responsible for acquiring the semaphore for the node.
	 */

	struct file *f = kmalloc(sizeof(struct file));
	if (f == NULL) { return -1; }
	if (n == NULL) { return -1; }
	if (t == NULL) { return -1; }

	f->node = n;
	f->mode = mode;
	f->flags = flags;
	f->position = 0;
	f->next = NULL;
	f->prev = NULL;

	/* Now assign the freshly-created file a valid descriptor. */
	int32_t i = 0;
	struct file *fi = t->files;

	if (fi == NULL) {
		/* If the task doesn't have any files yet. */
		t->files = f;

		goto __create_desc_ret;
	}

	/* If the task already has some files. */
	do {
		if (fi->file_des == i) {
			/* This file descriptor is used. Try the next number. */
			i++;
			fi = t->files;
			continue;
		}

		fi = fi->next;
	} while (fi->next != NULL);

	/* We found an available file descriptor. Now we should add this to
	 * the process' list of files.
	 */

	fi->next = f;
	f->prev = fi;

__create_desc_ret:

	f->file_des = i;
	switch (mode) {
		case FD_READ:
			n->reader_count++;
			break;

		case FD_WRITE:
			n->writer_count++;
			break;

		default:
			break;
	}

	return f->file_des;
};


int32_t vfs_destroy_descriptor(struct task *t, struct file *f) {
	/* "destroys" a file descriptor. */

	if (f == NULL)                  { return -1; }
	if (f->node == NULL)            { return -1; }
	if (f->node->mutex == NULL)     { return -1; }

	acquire_semaphore(f->node->mutex);

	/* Unlink the file descriptor from the task's list of files. */
	if (f == t->files) {
		t->files = t->files->next;
		if (t->files != NULL) {
			t->files->prev = NULL;
		}

	} else {
		if (f->next != NULL) {
			f->next->prev = f->prev;
		}
		if (f->prev != NULL) {
			f->prev->next = f->next;
		}
	}

	/* Update reader/writer count of the node. */
	switch (f->mode) {
		case FD_READ:
			f->node->reader_count--;
			break;
		case FD_WRITE:
			f->node->writer_count--;
			break;
		default: break;
	}
	release_semaphore(f->node->mutex);
	kfree(f);

	return 0;	/* TODO: Add better error handling/detection*/
};
