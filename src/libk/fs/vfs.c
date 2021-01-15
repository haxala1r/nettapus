
#include <fs/fs.h>
#include <mem.h>
#include <fs/ustar.h>


//This is a list of all VNODEs in the *ENTIRE SYSTEM*.
//This list is checked whenever a new file is opened/closed, and the relevant info gets
//updated.
FILE_VNODE* vnodes;


/*This is a list of open files inside the kernel process, each with a file descriptor.
 *** THIS WILL BE PUT INTO THE PROCESS STRUCTURE ONCE PROPER PROCESS MANAGEMENT IS IMPLEMENTED. 
 *** IT IS ONLY KEPT HERE FOR A TEMPORARY AMOUNT OF TIME. EACH PROCESS WILL HAVE ITS
 *** OWN LIST OF FILES, EACH WITH A FILE DESCRIPTOR, LIKE THIS ONE. THE VNODES, HOWEVER,
 *** WILL REMAIN IN THIS FILE, AS THE INFORMATION THEY CONTAIN IS FOR KERNEL USE ONLY.***/
FILE* files;


int32_t vfs_assign_file_des(FILE_VNODE* node, uint8_t mode, uint8_t flags) {
	//returns a file struct with a valid file descriptor. THIS WILL BE EXTENDED
	//TO BE USED FOR EVERY PROCESS. THIS LOGIC IS TEMPORARY, THE SAME WAY THE files VARIABLE
	//ABOVE IS.
	
	FILE* f = kmalloc(sizeof(FILE));
	if (f == NULL) {
		return -1;
	}
	f->node = node;
	f->mode = mode;
	f->flags = flags;
	f->position = 0;
	f->next = NULL;
	
	//now assign a valid file descriptor.
	int32_t i = 0;
	FILE* fi = files;
	if (fi == NULL) {
		//the below loop relies on fi->next being NULL. this obviously breaks a lot of
		//things if fi is NULL before the loop.
		f->file_des = i;
		files = f;
		node->stream_count++;
		return f->file_des;
	}
	while (1) {
		
		if (fi->file_des == i) {
			//try the next number and start from the beginning.
			i++;
			fi = files;
			continue;
		}
		if (fi->next == NULL) {
			break;
		}
		fi = fi->next;
	}
	f->file_des = i;
	fi->next = f;
	
	node->stream_count++;
	
	return f->file_des;
}

FILE_VNODE* vfs_vnode_lookup(char* file_name) {
	//finds a vnode by parsing the list, and returns it.
	
	FILE_VNODE* i = vnodes;
	
	while (i) {
		if (!memcmp(i->file_name, file_name, strlen(i->file_name))) {
			break;
		}
		i = i->next;
	}
	return i;
};

FILE* vfs_fd_lookup(int32_t file_des) {
	//After proper process management is implemented, this should also take a process
	//struct and use that to look for the necessary file structure.
	
	FILE* i = files;
	while (i) {
		if (i->file_des == file_des) {
			return i;
		}
		i = i->next;
	}
	return NULL;
};





int32_t kopen_fs(file_system_t* fs, char* file_name, uint8_t mode) {
	//takes the necessary parameters, and opens a file.
	
	
	FILE_VNODE* node;
	node = vfs_vnode_lookup(file_name);
	if (node == NULL) {
		//gotta create a new one.
		
		//add the node to the start of the list, to avoid having to loop through it again.
		node = kmalloc(sizeof(FILE_VNODE));
		if (node == NULL) {
			return -1;
		}
		node->next = vnodes;
		vnodes = node;
		
		
		node->fs = fs;
		node->stream_count = 0;	//this will get incremented by vfs_assign_file_des
		//now we ask the relevant file system to gather the relevant info about this file.
	
		//excuse the lazy programming here.
		switch (fs->fs_type) {
			case FS_USTAR: 
				node->special = (void*)ustar_file_lookup(fs, file_name); 
				if (node->special) {
					//no need to set these if file couldn't be found.
					node->size = ((USTAR_FILE_t*)(node->special))->size;
					node->file_name = kmalloc(strlen(file_name) + 1);
					memcpy(node->file_name, file_name, strlen(file_name) + 1);
				
				}
				break;
			default: node->special = NULL; break;	//the file system isn't known.
		}
	
		if (node->special == NULL) {
			if (node->stream_count == 0) {
			
				
				if (vnodes == node) {
					//This is here to make sure that vnodes doesn't point to freed memory.
					vnodes = vnodes->next;
				}
				kfree(node);
	
			}
			return -1;
		}
		
		
		
		
	} 
	
	
	//now we create the structure from the POV of the process, and return that.
	
	/* TODO: REPLACE THIS PART WITH SOME LOGIC TO FIND AN AVAILABLE FILE DESCRIPTOR DEPENDING
	 * ON THE AVAILABLE DESCRIPTORS OF A CERTAIN PROCESS*/
	//This part currently only works on the kernel process. 
	//The vfs_assign_file_des() function SHOULD AND WILL be changed so that each process
	//can have its own list of file descriptors. Currently it is still the same,
	//because there isn't proper multi-process support yet. When that comes, this function
	//(and everything else) should and will be changed.
	
	return vfs_assign_file_des(node, mode, 0);
};

int32_t kopen(char* file_name, uint8_t mode) {
	return kopen_fs(fs_get_root(), file_name, mode);
};

int32_t kclose(int32_t file_des) {
	//takes a file descriptor, and "closes" the file corresponding to that descriptor.
	//also frees the file vnode if there are no other "streams" open to it.
	
	/* 
	 * vfs_fd_lookup will loop through all descriptors anyway, and even if we use it,
	 * we still need a secondary loop to find the file descriptor before this one.
	 * (we need to unlink this descriptor.)
	 * so instead, we do a loop here, by ourselves. To avoid doing 2 loops.
	 * 
	 * In the future, I'd probably like to turn the open file list into a 
	 * doubly-linked list instead of a singly-linked one to fix this instead.
	 * That might also benefit us more in other ways as well.
	 */
	
	
	//copy of the loop in vfs_fd_lookup, but finds the file before this one instead.
	FILE* i = files;
	if (i == NULL) {
		return -1;
	}
	
	
	
	if (i->file_des == file_des) {
		//no need to unlink.
		
		files = files->next;
		
		FILE_VNODE* node = i->node;
		node->stream_count--;
		if (node->stream_count == 0) {
			
			//we need to free the node as well.
			//thus, another loop. Some refactoring is necessary at this point.
			
			FILE_VNODE* ni = vnodes;
			if (ni == node) {
				vnodes = vnodes->next;
				
				kfree(node);
				kfree(node->special);
				
				return 0;
			}
			while (ni) {
				if (ni->next == node) {
					break;
				}
				
				ni = ni->next;
			}
			
			ni->next = ni->next->next;
			kfree(node->special);
			kfree(node);
			
			
			
			//unlink should be successfull.
		}
		return 0;
	}
	
	
	
	
	while (i) {
		if (i->next->file_des == file_des) {
			break;
		}
		i = i->next;
	}
	
	FILE_VNODE* node = i->next->node;
	node->stream_count--;
	if (node->stream_count == 0) {
		
		//we need to free the node as well.
		//thus, another loop. Some refactoring is necessary at this point.
		
		FILE_VNODE* ni = vnodes;
		if (ni == node) {
			vnodes = vnodes->next;
			
			kfree(node);
			kfree(node->special);
			
			return 0;
		}
		while (ni) {
			if (ni->next == node) {
				break;
			}
			
			ni = ni->next;
		}
		
		ni->next = ni->next->next;
		
		kfree(node->special);	

		kfree(node);
		
		//unlink should be successfull.
	}
	
	kfree(i->next);
	i->next = i->next->next;	//not using f for clarity.s
	
	
	//should be done.
	
	return file_des;
};


int32_t kseek(int32_t file_des, uint32_t new_pos) {
	FILE* f = vfs_fd_lookup(file_des);
	if (!(new_pos < f->node->size)) {
		return -1;
	}
	f->position = new_pos;
	return 0;
};




int32_t kread(int32_t file_des, void* buf, uint32_t bytes) {
	//WOOO the classics!
	//TODO: change this and the file system drivers so that this function returns
	//the amount of bytes actually read, instead of returning the bytes parameter directly. 
	
	FILE* f = vfs_fd_lookup(file_des);
	
	
	
	//some NULL checks. we don't want the system to crash while reading a file do we?
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
		return -1;	//the file wasn't opened for reading.
	}
	
	
	
	FILE_VNODE* node = f->node;		//to avoid typing f->node everywhere.
	uint8_t status = 0;		//we're going to store the status here.
	
	//now we need to find the appropriate file system driver and pass the required parameters.
	
	switch (node->fs->fs_type) {
		case FS_USTAR: 	status = ustar_read_file(node->fs, (USTAR_FILE_t*)node->special, buf, f->position, bytes); break;
		case 0xFF: 	status = 0xFF; break;
	}
	
	
	//now we can return.
	
	if (status) {
		return -1;
	}
	
	//if the user tried to read more than there is, figure out how much was actually read.
	if (f->position + bytes > f->node->size) {
		//return the amount of bytes actually read.
		int32_t ret = f->node->size - f->position;
		f->position = f->node->size;
		return ret;
	} else {
		f->position += bytes;
	}
	
	return bytes;
};


int32_t kwrite(int32_t file_des, void* buf, uint32_t bytes) {
	//and now write!
	//This is a simple-enough implementation. Takes a buffer and amount of bytes, and writes
	//it to a file.
	
	FILE* f = vfs_fd_lookup(file_des);
	

	//some NULL checks. we don't want the system to crash while writing to a file do we?
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
		return -1;	//the file wasn't opened for reading.
	}
	
	
	FILE_VNODE* node = f->node;		//to avoid typing f->node everywhere.
	uint8_t status = 0;		//we're going to store the status here.
	
	
	if ((f->position + bytes) > node->size) {
		//we might need to enlarge the file if there isn't enough space.
		
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
	
	
	//now the actual disk access.
	switch (node->fs->fs_type) {
		case FS_USTAR: 	status = ustar_write_file(node->fs, (USTAR_FILE_t*)node->special, buf, f->position, bytes); break;
		case 0xFF: 		status = 0xFF; break;
		default: 		status++;
	}
	
	if (status) {
		return -1;
	}
	
	
	//because the file system driver is set to automatically enlarge a file before writing,
	//we don't need to figure out anything complicated.
	f->position += bytes;
	
	return bytes;
};








#ifdef DEBUG
#include <tty.h>

void vfs_print_state(void) {
	vfs_print_nodes();
	vfs_print_files();
};

void vfs_print_nodes(void) {
	char str[16] = {};
	FILE_VNODE* ni = vnodes;
	terminal_puts("\nVNODES:\n");
	terminal_puts("{file name}: {vnode ptr} {stream count} {file size}\n");
	while (ni)  {
		
		
		terminal_puts(ni->file_name);
		terminal_putc(':');
		terminal_putc(' ');
		
		xtoa((uint32_t)ni, str);
		terminal_puts(str);
		terminal_putc(' ');
		
		xtoa(ni->stream_count, str);
		terminal_puts(str);
		terminal_putc(' ');
		
		xtoa(ni->size, str);
		terminal_puts(str);
		terminal_putc('\n');
		
		ni = ni->next;
	}
	
};


void vfs_print_files(void) {
	char str[16] = {};
	FILE* fi = files;
	terminal_puts("\nFILES:\n");
	terminal_puts("{file des}: {file ptr} {file name} {position} {mode}\n");
	while (fi) {
		
		
		xtoa((uint32_t)fi->file_des, str);
		terminal_puts(str);
		terminal_putc(':');
		terminal_putc(' ');
		
		xtoa((uint32_t)fi, str);
		terminal_puts(str);
		terminal_putc(' ');
		
		
		terminal_puts(fi->node->file_name);
		terminal_putc(' ');
		
		xtoa(fi->position, str);
		terminal_puts(str);
		terminal_putc(' ');
		
		xtoa(fi->mode, str);
		terminal_puts(str);
		
		terminal_putc('\n');
		
		fi = fi->next;
	}
};



#endif

