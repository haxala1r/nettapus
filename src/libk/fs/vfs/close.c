#include <fs/fs.h>
#include <mem.h>
#include <task.h>

#include <fs/ustar.h>
#include <fs/fat16.h>




int32_t close(Task *task, int32_t file_des) {
	/* Takes a file descriptor, and "closes" the file corresponding to that descriptor.
	 * Also frees the file's associated vnode if there are no other "streams" open to it.
	 */
	
	FILE *f = vfs_fd_lookup(task, file_des);
	if (f == NULL) {
		return 1;
	}
	
	/* Unlink the file descriptor from the task's list of files. */
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
	
	
	
	FILE_VNODE *node = f->node;
	/* Now check if there are any other streams open to the node, and if
	 * not, unlink the node as well.
	 */
	switch (f->mode) {
		case FD_READ:
			node->reader_count--;
			break;
		case FD_WRITE:
			node->writer_count--;
			break;
		default: break;
	}
	
	kfree(f);
	
	if (node == NULL) {
		return 1;
	}
	
	if ((node->reader_count == 0) && (node->writer_count == 0)) {
		/* We also need to free the node. */
		vfs_unlink_node(node);
		
	}
	
	return 0;
};

int32_t kclose(int32_t fd) {
	return close(get_current_task(), fd);
};
