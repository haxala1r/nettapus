#include <fs/fs.h>
#include <mem.h>
#include <task.h>

#include <fs/ustar.h>
#include <fs/fat16.h>




int32_t close(struct task *t, int32_t file_des) {
	/* Takes a file descriptor, and "closes" the file corresponding to that descriptor.
	 * Also frees the file's associated vnode if there are no other "streams" open to it.
	 */

	struct file *f = vfs_fd_lookup(t, file_des);
	if (f == NULL)       { return -1; }
	if (f->node == NULL) { return -1; }

	struct file_vnode *node = f->node;

	vfs_destroy_descriptor(t, f);

	/* Unlink the node if no streams are left open. */
	if ((node->reader_count == 0) && (node->writer_count == 0)) {
		/* We also need to free the node.
		 * BUG: This function can go wackity-wack if another process is creating
		 * a file descriptor to this node at the same time.
		 */
		vfs_destroy_node(node);
	}

	return 0;
};

int32_t kclose(int32_t fd) {
	return close(get_current_task(), fd);
};
