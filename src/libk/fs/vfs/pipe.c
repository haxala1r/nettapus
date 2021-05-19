#include <fs/fs.h>
#include <task.h>
#include <err.h>
#include <mem.h>



int32_t pipeu(struct task *t, int32_t ret[]) {
	if (t == NULL)   { return -ERR_INVALID_PARAM; }
	if (ret == NULL) { return -ERR_INVALID_PARAM; }

	/* Create the node first. */
	struct file_vnode *pipe_vnode = kmalloc(sizeof(*pipe_vnode));
	memset(pipe_vnode, 0, sizeof(*pipe_vnode));
	pipe_vnode->streams_open = 0;
	pipe_vnode->cached_links = 0;
	pipe_vnode->link_count = 0;
	pipe_vnode->type_perm = 0x1000;
	// TODO: Set owner uid and gid to that of the task.

	pipe_vnode->mutex = create_semaphore(1);

	pipe_vnode->read_queue = kmalloc(sizeof(QUEUE));
	memset(pipe_vnode->read_queue, 0, sizeof(QUEUE));
	pipe_vnode->write_queue = kmalloc(sizeof(QUEUE));
	memset(pipe_vnode->write_queue, 0, sizeof(QUEUE));

	/* Pipes use the same open function as files. */
	pipe_vnode->open = vfs_open_file;
	pipe_vnode->read = vfs_read_pipe;
	pipe_vnode->write = vfs_write_pipe;
	pipe_vnode->close = NULL;

	/* Now create two file descriptors, and return them. */
	ret[0] = pipe_vnode->open(pipe_vnode, t, FD_READ);
	ret[1] = pipe_vnode->open(pipe_vnode, t, FD_WRITE);

	if ((ret[0] < 0) || (ret[1] < 0)) {
		kfree(pipe_vnode->mutex);
		kfree(pipe_vnode->read_queue);
		kfree(pipe_vnode->write_queue);
		kfree(pipe_vnode);
		return -ERR_NO_RESULT;
	}

	pipe_vnode->pipe_mem = kmalloc(DEFAULT_PIPE_SIZE);

	return GENERIC_SUCCESS;
};

