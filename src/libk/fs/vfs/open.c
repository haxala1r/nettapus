#include <fs/fs.h>
#include <mem.h>
#include <task.h>

#include <fs/ustar.h>
#include <fs/fat16.h>


int32_t vfs_assign_file_des(struct file_vnode *node, struct task *t, uint8_t mode, uint8_t flags) {
	/* Creates a new file descriptor for a given node. */

	struct file *f = kmalloc(sizeof(struct file));
	if (f == NULL) {
		return -1;
	}
	f->node = node;
	f->mode = mode;
	f->flags = flags;
	f->position = 0;
	f->next = NULL;
	f->prev = NULL;

	/* Now assign the freshly-created file a valid descriptor. */
	int32_t i = 0;
	struct file *fi = t->files;

	if (fi == NULL) {
		/*
		 * The below loop relies on fi->next being NULL. this obviously breaks a lot of
		 * things if fi is NULL before the loop.
		 */

		f->file_des = i;
		t->files = f;
		switch (mode) {
			case FD_READ:
				node->reader_count++;
				break;

			case FD_WRITE:
				node->writer_count++;
				break;

			default:
				break;
		}

		return f->file_des;
	}

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
	f->file_des = i;
	fi->next = f;
	f->prev = fi;

	switch (mode) {
		case FD_READ:
			node->reader_count++;
			break;

		case FD_WRITE:
			node->writer_count++;
			break;

		default:
			break;
	}

	return f->file_des;
}


struct file *vfs_fd_lookup(struct task *t, int32_t file_des) {
	struct file *i = t->files;

	while (i) {
		if (i->file_des == file_des) {
			return i;
		}
		i = i->next;
	}
	return NULL;
};


int32_t open_file(struct file_vnode *node, struct task *t, uint8_t mode) {
	/* This is the generic open function for on-disk files. */


	/* NULL checks are important. */
	if (node == NULL)               { return -1; };
	if (node->fs == NULL)           { return -1; };
	if (node->file_name == NULL)     { return -1; };
	if (node->mutex == NULL)    { return -1; };
	if (node->type != FILE_NORMAL)  { return -1; };

	if (t == NULL)                  { return -1; };

	acquire_semaphore(node->mutex);

	/* Load FS-specific info about the file if not loaded already. */
	if (node->special == NULL) {
		switch (node->fs->fs_type) {
		case FS_USTAR:
			node->special = ustar_file_lookup(node->fs, node->file_name);

			if (node->special != NULL) {
				node->last = ((USTAR_FILE_t*)(node->special))->size;
			}
			break;

		case FS_FAT16:
			node->special = fat16_file_lookup(node->fs, node->file_name);

			if (node->special != NULL) {
				node->last = ((FAT16_FILE *)(node->special))->entry->file_size;
			}
			break;

		default:
			node->special = NULL;
			break;
		}

		if (node->special == NULL) {
			release_semaphore(node->mutex);
			return -1;
		}
	}

	/* Keep the fd in a variable, because we need to release the semaphore. */
	int32_t fd = vfs_assign_file_des(node, t, mode, 0);

	release_semaphore(node->mutex);
	return fd;
};


int32_t open_pipe(struct file_vnode *node, struct task *t, uint8_t mode) {
	/* This is the generic open function for pipes. It *can* be used
	 * for unnamed pipes, but a pointer to the node is necessary, making it
	 * impossible for userspace apps to actually call this on unnamed pipes.
	 */

	if (node == NULL)               { return -1; };
	if (node->mutex == NULL)        { return -1; };
	if ((node->type != FILE_PIPE_NAMED) && (node->type != FILE_PIPE_UNNAMED)) {
		return -1;
	}
	if (t == NULL)                  { return -1; };

	acquire_semaphore(node->mutex);

	/* Allocate memory for the pipe if necessary. */
	if (node->special == NULL) {
		node->special = kmalloc(DEFAULT_PIPE_SIZE);
		if (node->special == NULL) {
			release_semaphore(node->mutex);
			return -1;
		}

		node->last = 0;
	}

	/* Keep the fd in a variable, because we need to release the semaphore. */
	int32_t fd = vfs_assign_file_des(node, t, mode, 0);

	release_semaphore(node->mutex);
	return fd;
};



int32_t open(struct file_system *fs, struct task *t, char *file_name, uint8_t mode) {
	/* Opens a file for the given process on the given file system. */

	struct file_vnode *node;
	node = vfs_vnode_lookup(file_name);
	if (node == NULL) {
		/* We need to create a new node.
		 *
		 * TODO: Remove fs from the parameters. Instead, the root file
		 * system should be determined at boot and then the specific file system
		 * can be determined through a list of mount points.
		 */
		node = vfs_create_node((uintptr_t)open_file, (uintptr_t)NULL, (uintptr_t)read_file, (uintptr_t)write_file, FILE_NORMAL);

		node->fs = fs;

		/* Create a copy of the file name. */
		node->file_name = kmalloc(strlen(file_name) + 1);
		memcpy(node->file_name, file_name, strlen(file_name) + 1);
	}

	/* Now open the file. */
	return node->open(node, t, mode);
};


int32_t pipeu(struct task *t, int32_t *ret) {
	/* This function creates a generic unnamed pipe, with two file descriptors
	 * as "ends" (one end is read, the other is write)
	 *
	 * This function, unlike pipen() (which is yet to be implemented),
	 * creates an *unnamed* pipe. (I.e. it does *not* appear in the filesystem.)
	 */

	struct file_vnode *node = vfs_create_node((uintptr_t)open_pipe, (uintptr_t)NULL, \
		(uintptr_t)read_pipe, (uintptr_t)write_pipe, FILE_PIPE_UNNAMED);

	ret[0] = node->open(node, t, 0);
	ret[1] = node->open(node, t, 1);

	if ((ret[0] == -1) || (ret[1] == -1)) {
		vfs_destroy_node(node);
		return -1;
	}
	return 0;
};


int32_t kopen(char* file_name, uint8_t mode) {
	return open(fs_get_root(), get_current_task(), file_name, mode);
};
