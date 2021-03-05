#include <fs/fs.h>
#include <task.h>

/* Now we need to include the headers for the FS drivers. */
#include <fs/ustar.h>
#include <fs/fat16.h>


int64_t write_file(FILE *f, void *buf, size_t bytes) {
	/* This is the generic write function for files. */
	
	/* Some NULL checks. we don't want the system to crash while writing to a file do we? */
	if (f == NULL) 				{ return -1; }
	if (f->node == NULL) 		{ return -1; }
	if (f->node->fs == NULL) 	{ return -1; }
	if (f->mode != 1) 			{ return -1; }
	
	
	/* This is for convenience. */
	FILE_VNODE *node = f->node;		
	
	/* So that we can check the return value of the FS drivers without if's everywhere. */
	uint8_t status = 0;		
	
	
	
	if ((f->position + bytes) > node->last) {
		/* We need to enlarge the file if there isn't enough space. */
		
		//TODO: Add FAT16 here. also clean up a bit more, the way read() does.
		switch (node->fs->fs_type) {
			case 0:		status = ustar_enlarge_file(node->fs, (USTAR_FILE_t*)node->special, (f->position + bytes) - node->last); break;
			case 0xFF:	return -1; break;
			default:	return -1; break;
		}
		if (status) {
			return -1;
		} else {
			node->last = (f->position + bytes);
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



int64_t write_pipe(FILE *f, void *buf, size_t bytes) {
	/* This is the generic write function for pipes. It writes to a pipe
	 * and blocks if there isn't space on the pipe. 
	 */ 
	
	if (f == NULL) 						{ return -1; }
	if (f->node == NULL) 				{ return -1; }
	if (f->node->special == NULL) 		{ return -1; }
	if (f->node->semaphore == NULL) 	{ return -1; }
	if (buf == NULL) 					{ return -1; }
	
	if (bytes <= 0) 					{ return  0; }
	if (f->node->reader_count == 0) 	{ return -1; }
	
	/* Acquire exclusive access to the pipe. */
	acquire_semaphore(f->node->semaphore);
	
	/* This is here because the upcoming loop modifies "bytes", and we need to 
	 * return the amount of bytes written.
	 */
	size_t to_read = bytes;
	
	
	/* If there isn't enough space on the pipe, we need to block until 
	 * there is. 
	 */
	while (bytes > (DEFAULT_PIPE_SIZE - f->node->last)) {
		if (f->flags & FILE_FLAG_NONBLOCK) {
			return 0;
		}
		
		/* This keeps yielding until there's room on the pipe. 
		 * TODO: this should be replaced by a waiting queue for each vnode
		 * or pipe. Currently, the resource in question (pipe) isn't distributed 
		 * fairly among processes. 
		 */
		while (f->node->last == DEFAULT_PIPE_SIZE) {
			release_semaphore(f->node->semaphore);
			
			lock_scheduler();
			yield();
			unlock_scheduler();
			
			acquire_semaphore(f->node->semaphore);
		}
		
		/* If we reached here, some other task must've read from the buffer.
		 * If this check doesn't execute, then the part after to loop
		 * will take care of the rest. 
		 */
		if (bytes > (DEFAULT_PIPE_SIZE - f->node->last)) {
			memcpy(f->node->special + f->node->last, buf, DEFAULT_PIPE_SIZE - f->node->last);
			buf += (DEFAULT_PIPE_SIZE - f->node->last);
			bytes -= (DEFAULT_PIPE_SIZE - f->node->last);
			f->node->last = DEFAULT_PIPE_SIZE;
			if (bytes <= 0) {
				release_semaphore(f->node->semaphore);
				return 0;
			}
		}
	}
	
	/* This handles the rest */
	memcpy(f->node->special + f->node->last, buf, bytes);
	f->node->last += bytes;
	
	release_semaphore(f->node->semaphore);
	return to_read;
};





int64_t write(TASK *task, int32_t file_des, void *buf, size_t bytes) {
	/* This function writes to a file/node, at the position file_des indicates. */
	
	if (task == NULL) 		{ return -1; };
	FILE *f = vfs_fd_lookup(task, file_des);
	if (f == NULL) 			{ return -1; };
	if (f->node == NULL) 	{ return -1; };
	
	return f->node->write(f, buf, bytes);
};



int64_t kwrite(int32_t file_des, void *buf, size_t bytes) {
	return write(get_current_task(), file_des, buf, bytes);
};
