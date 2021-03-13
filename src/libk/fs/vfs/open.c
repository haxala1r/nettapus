#include <fs/fs.h>
#include <mem.h>
#include <task.h>

#include <fs/ustar.h>
#include <fs/fat16.h>


int32_t vfs_assign_file_des(FILE_VNODE *node, TASK *task, uint8_t mode, uint8_t flags) {
	/* Creates a new file descriptor for a given node. */

	FILE *f = kmalloc(sizeof(FILE));
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
	FILE *fi = task->files;

	if (fi == NULL) {
		/*
		 * The below loop relies on fi->next being NULL. this obviously breaks a lot of
		 * things if fi is NULL before the loop.
		 */

		f->file_des = i;
		task->files = f;
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

	while (1) {
		if (fi->file_des == i) {
			/* This file descriptor is used. Try the next number. */
			i++;
			fi = task->files;
			continue;
		}

		if (fi->next == NULL) {
			break;
		}

		fi = fi->next;
	}

	/* We found an available file descriptor. Now we should add this to
	 * the process' list of files.
	 */
	f->file_des = i;
	fi->next = f;
	f->prev = fi;

	/* Now increment the appropriate counts accordingly. */
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


FILE *vfs_fd_lookup(TASK *task, int32_t file_des) {

	FILE *i = task->files;
	while (i) {
		if (i->file_des == file_des) {
			return i;
		}
		i = i->next;
	}
	return NULL;
};


int32_t open_file(FILE_VNODE *node, TASK *task, uint8_t mode) {
	/* This is the generic open function for on-disk files. */


	/* NULL checks are important. */
	if (node == NULL) 				{ return -1; };
	if (node->fs == NULL) 			{ return -1; };
	if (node->file_name == NULL) 	{ return -1; };
	if (node->semaphore == NULL) 	{ return -1; };
	if (node->type != FILE_NORMAL)	{ return -1; };

	if (task == NULL) 				{ return -1; };

	acquire_semaphore(node->semaphore);

	/* Check if the relevant (FS-specific) data about the file has
	 * already been loaded. If not, we need to load it now.
	 */

	if (node->special == NULL) {

		/* Now ask the relevant driver to gather info on the file. */
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

			default: node->special = NULL; break;
		}

		/* The caller must deal with the consequences if the file isn't found
		 * or an error occurs.
		 */
		if (node->special == NULL) {
			release_semaphore(node->semaphore);
			return -1;
		}
	}

	/* Now we need to assign a file descriptor, then we can return.
	 * A variable is used, because we need to release the semaphore after
	 * the file descriptor has been assigned.
	 */

	int32_t fd = vfs_assign_file_des(node, task, mode, 0);

	release_semaphore(node->semaphore);
	return fd;
};


int32_t open_pipe(FILE_VNODE *node, TASK *task, uint8_t mode) {
	/* This is the generic open function for pipes. It *can* be used
	 * for unnamed pipes, but a pointer to the node is necessary, making it
	 * impossible for userspace apps to actually call this on unnamed pipes.
	 */

	/* NULL checks are important. */
	if (node == NULL) 				{ return -1; };
	if (node->semaphore == NULL) 	{ return -1; };
	if (node->type != FILE_PIPE)	{ return -1; };

	if (task == NULL) 				{ return -1; };

	acquire_semaphore(node->semaphore);

	/* Check whether memory for this pipe has already been allocated.
	 * If not, we need to allocate it now.
	 */

	if (node->special == NULL) {
		node->special = kmalloc(DEFAULT_PIPE_SIZE);
		if (node->special == NULL) {
			release_semaphore(node->semaphore);
			return -1;
		}
		node->last = 0;		/* In case it was corrupt */
	}

	/* Now we need to assign a file descriptor, then we can return.
	 * A variable is used, because we need to release the semaphore after
	 * the file descriptor has been assigned.
	 */

	int32_t fd = vfs_assign_file_des(node, task, mode, 0);

	release_semaphore(node->semaphore);
	return fd;
};



int32_t open(file_system_t *fs, TASK *task, char *file_name, uint8_t mode) {
	/* Opens a file for the given process on the given file system. */

	FILE_VNODE *node;
	node = vfs_vnode_lookup(file_name);
	if (node == NULL) {
		/* We need to create a new node.
		 *
		 * TODO: Create a general VFS semaphore, so that if two tasks
		 * try to create nodes at the same time, they don't screw everything
		 * up. This should be fixed everywhere new nodes are created.
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
	return node->open(node, task, mode);
};


int32_t pipeu(TASK *task, int32_t *ret) {
	/* This function creates a generic unnamed pipe, with two file descriptors
	 * as "ends" (one end is read, the other is write)
	 *
	 * This function, unlike pipen() (which is yet to be implemented),
	 * creates an *unnamed* pipe. (I.e. it does *not* appear in the filesystem.)
	 */

	FILE_VNODE *node = vfs_create_node((uintptr_t)open_pipe, (uintptr_t)NULL, (uintptr_t)read_pipe, (uintptr_t)write_pipe, FILE_PIPE);

	ret[0] = node->open(node, task, FD_READ);
	ret[1] = node->open(node, task, FD_WRITE);

	if ((ret[0] == -1) || (ret[1] == -1)) {
		vfs_destroy_node(node);
		return -1;
	}
	return 0;
};


int32_t kopen(char* file_name, uint8_t mode) {
	return open(fs_get_root(), get_current_task(), file_name, mode);
};
