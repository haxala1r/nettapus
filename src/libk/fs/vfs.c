
#include <fs/fs.h>
#include <mem.h>
#include <task.h>

#include <fs/ustar.h>
#include <fs/fat16.h>


/* This is a list of all VNODEs in the *ENTIRE SYSTEM*.
 * This list is checked whenever a new file is opened/closed, and the relevant info gets
 * updated.
 */
FILE_VNODE* vnodes;



int32_t vfs_assign_file_des(FILE_VNODE* node, Task *task, uint8_t mode, uint8_t flags) {
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
		node->stream_count++;
		
		return f->file_des;
	}
	
	while (1) {
		if (fi->file_des == i) {
			//try the next number and start from the beginning.
			i++;
			fi = task->files;
			continue;
		}
		
		if (fi->next == NULL) {
			break;
		}
		
		fi = fi->next;
	}
	
	f->file_des = i;
	fi->next = f;
	f->prev = fi;
	
	node->stream_count++;
	
	return f->file_des;
}

FILE_VNODE *vfs_vnode_lookup(char* file_name) {
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




int32_t open(file_system_t *fs, Task *task, char *file_name, uint8_t mode) {
	/* Opens a file for the given process on the given file system. */
	
	
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
		
		node->fs = fs;
		node->stream_count = 0;	//this will get incremented by vfs_assign_file_des
		
		/* Now we ask the relevant file system to gather the relevant info about this file. */
		switch (fs->fs_type) {
			case FS_USTAR: 
				node->special = ustar_file_lookup(fs, file_name); 
				
				if (node->special != NULL) {	
					node->size = ((USTAR_FILE_t*)(node->special))->size;
				}
				
				break;
				
			case FS_FAT16:
				node->special = fat16_file_lookup(fs, file_name);
				
				if (node->special != NULL) {
					node->size = ((FAT16_FILE *)(node->special))->entry->file_size;
				}
				
				break;
			default: node->special = NULL; break;	//the file system isn't known.
		}
		
		/* A return of NULL indicates the file could not be found. */
		if (node->special == NULL) {
			vnodes = vnodes->next;
			
			if (vnodes != NULL) {
				vnodes->prev = NULL;
			}
			
			kfree(node);
			return -1;
		}
		
		node->file_name = kmalloc(strlen(file_name) + 1);
		memcpy(node->file_name, file_name, strlen(file_name) + 1);
	} 
	
	
	return vfs_assign_file_des(node, task, mode, 0);
};

int32_t kopen(char* file_name, uint8_t mode) {
	return open(fs_get_root(), get_current_task(), file_name, mode);
};

int32_t close(Task *task, int32_t file_des) {
	/* Takes a file descriptor, and "closes" the file corresponding to that descriptor.
	 * Also frees the file's associated vnode if there are no other "streams" open to it.
	 */
	
	FILE *f = vfs_fd_lookup(task, file_des);
	if (f == NULL) {
		return 1;
	}
	
	if (f == task->files) {
		task->files = task->files->next;
		if (task->files != NULL) {
			task->files->prev = NULL;
		}
	
	} else {
		if (f->next != NULL) {
			f->next->prev = f->prev;
		}
		if (f->prev != NULL) {
			f->prev->next = f->next;
		}
	}
	
	
	FILE_VNODE* node = f->node;
	kfree(f);
	
	if (node == NULL) {
		return 1;
	}
	
	node->stream_count--;
	if (node->stream_count == 0) {
		//we need to free the node as well.
		
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
		switch (node->fs->fs_type) {
			case FS_FAT16:
				kfree(((FAT16_FILE*)node->special)->entry);
				break;
			default:
				break;
		}
		kfree(node->special);
		kfree(node);
	}
	
	
	return 0;
};

int32_t kclose(int32_t fd) {
	return close(get_current_task(), fd);
};



int32_t seek(Task *task, int32_t file_des, uint64_t new_pos) {
	
	FILE* f = vfs_fd_lookup(task, file_des);
	
	if (!(new_pos < f->node->size)) {
		return -1;
	}
	
	f->position = new_pos;
	return 0;
};

int32_t kseek(int32_t file_des, uint64_t new_pos) {
	return seek(get_current_task(), file_des, new_pos);
};


int64_t read(Task *task, int32_t file_des, void* buf, int64_t bytes) {
	/* Here comes The Classic! */
	
	FILE* f = vfs_fd_lookup(task, file_des);
	
	if (f == NULL) {
		return -1;
	}
	if (f->node == NULL) {
		return -1;
	}
	if (f->node->fs == NULL) {
		return -1;
	}
	
	if (f->mode != 0) {
		return -1;
	}
	
	
	FILE_VNODE* node = f->node;		//to avoid typing f->node everywhere.
	uint8_t status = 0;		//we're going to store the status here.
	int64_t to_read = bytes;
	
	
	if ((f->position + to_read) > node->size) {
		to_read = node->size - f->position;
	}
	
	//now we need to find the appropriate file system driver and pass the required parameters.
	
	switch (node->fs->fs_type) {
		case FS_USTAR: 	status = ustar_read_file(node->fs, (USTAR_FILE_t*)node->special, buf, f->position, (uint32_t)to_read); break;
		case FS_FAT16:	status = fat16_read_file(node->fs, (FAT16_FILE*)node->special, buf, f->position, (uint32_t)to_read); break;
		case 0xFF: 	status = 0xFF; break;
	}
	
	
	//now we can return.
	
	if (status) {
		return -1;
	}
	
	f->position += to_read;
	return to_read;
};

int64_t kread(int32_t file_des, void *buf, int64_t bytes) {
	return read(get_current_task(), file_des, buf, bytes);
};



int32_t write(Task *task, int32_t file_des, void* buf, int64_t bytes) {
	/* This function writes to a file, at the position file_des indicates. */
	
	FILE* f = vfs_fd_lookup(task, file_des);
	

	/* Some NULL checks. we don't want the system to crash while writing to a file do we? */
	if (f == NULL) {
		return -1;
	}
	if (f->node == NULL) {
		return -1;
	}
	if (f->node->fs == NULL) {
		return -1;
	}
	
	if (f->mode != 1) {
		return -1;	
	}
		
	/* This is for convenience. */
	FILE_VNODE* node = f->node;		
	
	/* So that we can check the return value of the FS drivers without if's everywhere. */
	uint8_t status = 0;		
	
	
	
	if ((f->position + bytes) > node->size) {
		/* We need to enlarge the file if there isn't enough space. */
		
		//TODO: Add FAT16 here. also clean up a bit more, the way read() does.
		switch (node->fs->fs_type) {
			case 0:		status = ustar_enlarge_file(node->fs, (USTAR_FILE_t*)node->special, (f->position + bytes) - node->size); break;
			case 0xFF:	return -1; break;
			default:	return -1; break;
		}
		if (status) {
			return -1;
		} else {
			node->size = (f->position + bytes);
		}
		
	}
	
	/* Now the actual disk access. */
	switch (node->fs->fs_type) {
		case FS_USTAR: 	status = ustar_write_file(node->fs, (USTAR_FILE_t*)node->special, buf, f->position, (uint32_t)bytes); break;
		case FS_FAT16:	status = fat16_write_file(node->fs, (FAT16_FILE*)node->special, buf, f->position, (uint32_t)bytes); break;
		case 0xFF: 		status = 0xFF; break;
		default: 		status++;
	}
	
	
	
	if (status) {
		return -1;
	}
	f->position += bytes;
	
	return bytes;
};



int64_t kwrite(int32_t file_des, void* buf, int64_t bytes) {
	return write(get_current_task(), file_des, buf, bytes);
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
	kputs("{file name}: {vnode ptr} {stream count} {file size}\n");
	while (ni)  {
		
		
		kputs(ni->file_name);
		kputs(": ");
		
		kputx((uint64_t)ni);
		kputs(" ");
		
		kputx(ni->stream_count);
		kputs(" ");
		
		kputx(ni->size);
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

