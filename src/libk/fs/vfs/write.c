#include <fs/fs.h>
#include <task.h>

/* Now we need to include the headers for the FS drivers. */
#include <fs/ustar.h>
#include <fs/fat16.h>


int64_t write_file(FILE *f, void *buf, int64_t bytes) {
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



int64_t write_pipe(FILE *f, void *buf, int64_t bytes) {
	/* This is the generic write function for pipes. It writes to a pipe
	 * and blocks if there isn't space on the pipe. 
	 */ 
	
	if (f == NULL) 					{ return -1; }
	if (f->node == NULL) 			{ return -1; }
	if (f->node->special == NULL) 	{ return -1; }
	if (buf == NULL) 				{ return -1; }
	
	if (bytes <= 0) 				{ return 0; }
	if (f->node->reader_count == 0) { return -1; };
	
	/* If there isn't enough space on the pipe, we need to block until 
	 * there is. 
	 */
	while (bytes > (0x1000 - f->node->last)) {
		//TODO: 
		// WRITE DATA TO PIPE
		// BLOCK UNTIL THERE'S SPACE
		// REPEAT UNTIL NO DATA LEFT
		
		/* For now, there isn't a blocking mechanism in place, so instead
		 * we'll return an error.
		 */
		return -1;
	}
	
	memcpy(f->node->special + f->node->last, buf, bytes);
	f->node->last += bytes;
	
	return bytes;
};





int64_t write(Task *task, int32_t file_des, void *buf, int64_t bytes) {
	/* This function writes to a file, at the position file_des indicates. */
	
	if (task == NULL) 		{ return -1; };
	FILE *f = vfs_fd_lookup(task, file_des);
	if (f == NULL) 			{ return -1; };
	if (f->node == NULL) 	{ return -1; };
	
	return f->node->write(f, buf, bytes);
};



int64_t kwrite(int32_t file_des, void *buf, int64_t bytes) {
	return write(get_current_task(), file_des, buf, bytes);
};
