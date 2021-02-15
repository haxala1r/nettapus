
#include <fs/fs.h>
#include <mem.h>
#include <task.h>

/* Now we need to include the headers for the FS drivers. */
#include <fs/ustar.h>
#include <fs/fat16.h>


int64_t read_file(FILE *f, void *buf, int64_t bytes) {
	/* This is the generic read function for files. It loads the data from disk,
	 * and copies it to buf.
	 */
	
	if (f == NULL) 				{ return -1; }
	if (f->node == NULL) 		{ return -1; }
	if (f->node->fs == NULL) 	{ return -1; }
	
	if (f->mode != 0) 			{ return -1; }
	
	
	
	FILE_VNODE* node = f->node;
	
	/* Status is going to be stored here. */
	uint8_t status = 0;	
	
	/* Determine the amount of bytes that can be read, and read it. */
	int64_t to_read = bytes;
	
	if ((f->position + to_read) > node->last) {
		to_read = node->last - f->position;
	}
	
	
	
	/* Now we need to consult the relevant FS driver. */
	
	switch (node->fs->fs_type) {
		case FS_USTAR: 	status = ustar_read_file(node->fs, node->special, buf, f->position, (uint32_t)to_read); break;
		case FS_FAT16:	status = fat16_read_file(node->fs, node->special, buf, f->position, (uint32_t)to_read); break;
		case 0xFF: 		status = 0xFF; break;
	}
	
	
	/* Now we can check for status, then return. */
	
	if (status) { return -1; }
	
	/* Return the amount of bytes that were actually read.*/
	f->position += to_read;
	return to_read;
};


int64_t read_pipe(FILE *f, void *buf, int64_t bytes) {
	/* This is the generic read function used for pipes. If the request
	 * size exceeds the pipe's size, or the requested amount of data has
	 * not been written to the pipe, the process will be blocked.
	 */
	
	/* Some checks to see if the request is valid. */
	if (f == NULL) 					{ return -1; }
	if (f->node == NULL) 			{ return -1; }
	if (f->node->special == NULL) 	{ return -1; }
	if (buf == NULL) 				{ return -1; }
	
	if (bytes <= 0) 				{ return 0; }
	if (f->node->writer_count == 0) { return -1; };
	
	/* Block if there isn't enough data to read. */
	while (f->node->last < bytes) {
		//TODO:
		// BLOCK();
		// COPY
		// REPEAT UNTIL NO MORE DATA IS REQUESTED
		return -1;	/* For now, don't block. */
	}
	
	/* Copy the data to caller's buffer. */
	memcpy(buf, f->node->special, bytes);
	
	/* Remove the data that was read from the pipe by relocating everything
	 * after the requested data to the start.*/
	memcpy(f->node->special, f->node->special + bytes, 0x1000 - bytes);
	
	f->node->last -= bytes;
	
	/* Clear out the data after the last byte of the pipe. This is
	 * probably unnecessary, but I'm not really sure if it's a good idea
	 * to remove it yet.
	 */
	memset(f->node->special + f->node->last, 0, 0x1000 - f->node->last);
	
	return bytes;
};


int64_t read(Task *task, int32_t file_des, void* buf, int64_t bytes) {
	/* This function simply finds the node associated with the given
	 * file descriptor and calls its read function.
	 */
	
	if (task == NULL) 			{ return -1; }
	
	/* Find the file. */
	FILE *f = vfs_fd_lookup(task, file_des);
	
	/* If the file descriptor isn't open, return an error. */
	if (f == NULL) 				{ return -1; }
	
	return f->node->read(f, buf, bytes);
};

int64_t kread(int32_t file_des, void *buf, int64_t bytes) {
	return read(get_current_task(), file_des, buf, bytes);
};
