#include <fs/fs.h>
#include <mem.h>
#include <task.h>

#include <fs/ustar.h>
#include <fs/fat16.h>

/* This file contains things related to the management of vnodes, i. e.
 * files, pipes and all that stuff from the POV of the kernel.
 */



/* This is a list of all VNODEs in the entire system.
 * This list is checked whenever a new file is opened/closed, and the relevant info gets
 * updated.
 */
FILE_VNODE *vnodes;



int32_t vfs_assign_file_des(FILE_VNODE *node, Task *task, uint8_t mode, uint8_t flags) {
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

FILE_VNODE *vfs_vnode_lookup(char *file_name) {
	/* Finds a vnode given a file name. */
	
	FILE_VNODE *i = vnodes;
	
	while (i) {
		if (!strcmp(i->file_name, file_name)) {
			break;
		}
		i = i->next;
	}
	return i;
};

FILE *vfs_fd_lookup(Task *task, int32_t file_des) {
	
	FILE *i = task->files;
	while (i) {
		if (i->file_des == file_des) {
			return i;
		}
		i = i->next;
	}
	return NULL;
};


uint8_t vfs_unlink_node(FILE_VNODE *node) {
	/* Unlinks a node. It also frees the resources allocated for the node. */
	if (node == NULL) { return 1; }
	
	if (node == vnodes) {
		vnodes = vnodes->next;
		if (vnodes != NULL) {
			vnodes->prev = NULL;
		}
			
	} else {
		if (node->next != NULL) {
			node->next->prev = node->prev;
		}
			
		if (node->prev != NULL) {
			node->prev->next = node->next;
		}
	}
		
	/* Now free any related structures, and then free the node. */
	
	if (node->fs != NULL) {
		switch (node->fs->fs_type) {
			/* Free any FS-specific structures. */
			case FS_USTAR:
				break;
				
			case FS_FAT16:
				kfree(((FAT16_FILE*)node->special)->entry);
				break;
					
			default:
				break;
		}
	}
	
	if (node->special != NULL) {
		kfree(node->special);
	}
	if (node->file_name != NULL) {
		kfree(node->file_name);
	}
	kfree(node);
	
	return 0;
};




int32_t open_file(FILE_VNODE *node, void *t, uint8_t mode) {
	/* This is the generic open function for on-disk files. */
	
	Task *task = t;
	
	/* NULL checks are important. */
	if (node == NULL) 				{ return -1; };
	if (node->fs == NULL) 			{ return -1; };
	if (node->file_name == NULL) 	{ return -1; };
	if (node->type != FILE_NORMAL)	{ return -1; };
	
	if (task == NULL) 				{ return -1; };
	
	
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
			return -1;	
		}
	}
	
	/* Now we need to assign a file descriptor, then we can return. */
	return vfs_assign_file_des(node, task, mode, 0);
};


int32_t open_pipe(FILE_VNODE *node, void *t, uint8_t mode) {
	/* This is the generic open function for pipes. It *can* be used
	 * for unnamed pipes, but a pointer to the node is necessary, making it
	 * impossible for userspace apps to actually call this on unnamed pipes.
	 */
	
	Task *task = t;
	
	/* NULL checks are important. */
	if (node == NULL) 				{ return -1; };
	if (node->type != FILE_PIPE)	{ return -1; };
	
	if (task == NULL) 				{ return -1; };
	
	
	return vfs_assign_file_des(node, task, mode, 0);
};



int32_t open(file_system_t *fs, Task *task, char *file_name, uint8_t mode) {
	/* Opens a file for the given process on the given file system. */
	int32_t fd = -1;
	
	FILE_VNODE *node;
	node = vfs_vnode_lookup(file_name);
	if (node == NULL) {
		/* We need to create a new node. */
		
		/* Allocate space for a new node, and add it to the very beginning
		 * of the doubly-linked list.
		 */
		node = kmalloc(sizeof(FILE_VNODE));
		if (node == NULL) {
			return -1;
		}
		
		if (vnodes != NULL) {
			node->next = vnodes;
			node->prev = NULL;
			vnodes->prev = node;
		} else {
			node->next = NULL;
			node->prev = NULL;
		}
		
		vnodes = node;
		
		/* Set some attributes of the node.*/
		node->fs = fs;
		
		node->reader_count = 0;		/* open_file updates this accordingly. */
		node->writer_count = 0;		/* open_file updates this accordingly. */
		node->special = NULL;		/* open_file sets this accordingly.	  */
		
		node->type = FILE_NORMAL;
		
		
		/* If the file node didn't exist already, it must be a regular file. */
		node->open = open_file;
		node->read = read_file;
		node->write = write_file;
		
		
		/* Create a copy of the file name. */
		node->file_name = kmalloc(strlen(file_name) + 1);
		memcpy(node->file_name, file_name, strlen(file_name) + 1);
		
		/* Now open the file. */
		fd = node->open(node, task, mode);
		
		
		/* Now we ask the relevant file system to gather the relevant info about this file. */
		/* A return of NULL indicates the file could not be found, or
		 * an error occured. In either case, we can't do much else.
		 */
		if (node->special == NULL) {
			vnodes = vnodes->next;
			
			if (vnodes != NULL) {
				vnodes->prev = NULL;
			}
			
			kfree(node);
			return -1;
		}
	} else {
		fd = node->open(node, task, mode);
	}
	
	return fd;
};


int32_t pipeu(Task *task, int32_t *ret) {
	/* This function creates a generic unnamed pipe, with two file descriptors
	 * as "ends" (one end is read, the other is write)
	 * 
	 * This function, unlike pipen(), creates an *unnamed* pipe. I. e. it does
	 * *not* appear in the filesystem. 
	 */
	
	FILE_VNODE *node = kmalloc(sizeof(FILE_VNODE));
	if (node == NULL) {
		return -1;
	}
	
	if (vnodes != NULL) {
		node->next = vnodes;
		node->prev = NULL;
		vnodes->prev = node;
	} else {
		node->next = NULL;
		node->prev = NULL;
	}
	
	vnodes = node;
	
	/* An unnamed pipe does not reside in any file system. */
	node->fs = NULL;
	node->type = FILE_PIPE;
	
	node->reader_count = 0;	
	node->writer_count = 0;	
	node->special = kmalloc(0x1000);	/* Exactly one page. */
	node->file_name = NULL;
	
	/* Remember, for pipes this indicates the last byte in the buffer that 
	 * was written to! (of course, a 0 indicates the buffer is clean)
	 */
	node->last = 0;		
	
	/* Set the relevant functions for the pipe. open() isn't relevant much,
	 * but it is set for consistency.
	 */
	node->open = open_pipe;
	node->read = read_pipe;
	node->write = write_pipe;
	
	ret[0] = node->open(node, task, FD_READ);
	ret[1] = node->open(node, task, FD_WRITE);
	
	if ((ret[0] == -1) || (ret[1] == -1)) {
		vnodes = vnodes->next;
		return -1;
	}
	return 0;
};


int32_t kopen(char* file_name, uint8_t mode) {
	return open(fs_get_root(), get_current_task(), file_name, mode);
};



#ifdef DEBUG
#include <tty.h>

void vfs_print_state(void) {
	vfs_print_nodes();
	vfs_print_files();
};

void vfs_print_nodes(void) {
	FILE_VNODE* ni = vnodes;
	kputs("\nVNODES:\n");
	kputs("{file name}: {vnode ptr} {reader count} {writer count} {file size}\n");

	while (ni)  {
		
		
		kputs(ni->file_name);
		kputs(": ");
		
		kputx((uint64_t)ni);
		kputs(" ");
		
		kputx(ni->reader_count);
		kputs(" ");
		
		kputx(ni->writer_count);
		kputs(" ");
		
		kputx(ni->last);
		kputs("\n");
		
		ni = ni->next;
	}
	
};


void vfs_print_files(void) {
	
	FILE* fi = get_current_task()->files;
	kputs("\nFILES:\n");
	kputs("{file des}: {file ptr} {node ptr} {file name} {position} {mode}\n");

	while (fi) {
		
		
		kputx(fi->file_des);
		kputs(": ");
		
		kputx((uint64_t)fi);
		kputs(" ");
		kputx((uint64_t)fi->node);
		kputs(" ");
		
		
		kputs(fi->node->file_name);
		kputs(" ");
		
		kputx(fi->position);
		kputs(" ");
		
		kputx(fi->mode);
		kputs("\n");
		
		fi = fi->next;
	}
};



#endif
