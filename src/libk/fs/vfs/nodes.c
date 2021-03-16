#include <fs/fs.h>
#include <string.h>
#include <task.h>
#include <mem.h>
#include <fs/fat16.h>

/* TODO: Move the node creation/deletion utilities here, and refactor
 * some of the existing code so that they use the functions
 * defined in this file.
 */

/* This is a list of all VNODEs in the entire system.
 * This list is checked whenever a new file is opened/closed, and the relevant info gets
 * updated.
 */
struct file_vnode *vnodes;

SEMAPHORE vnodes_semaphore = {
	.current_count = 0,
	.max_count = 1,
	.first_waiting_task = NULL,
	.last_waiting_task = NULL
};


struct file_vnode *vfs_create_node(uintptr_t open, uintptr_t close, uintptr_t read,
						uintptr_t write, uint16_t type) {
	/* This function simply allocates space for a new node and links it.
	 * It does not initialise things like the file system, name, buffer etc.
	 * Those must be done by the caller. This function's main job is to
	 * manage linking/uninking the vnodes.
	 *
	 * It does, however, set the open, close, read and write functions
	 * and the type for the node, whiich must be present for all valid nodes.
	 */

	if ((open == 0) || (close != 0) || (read == 0) || (write == 0)){
		return NULL;
	}

	acquire_semaphore(&vnodes_semaphore);

	struct file_vnode *node = kmalloc(sizeof(struct file_vnode));

	/* Clear out the structure. */
	memset(node, 0, sizeof(*node));

	/* Set the given attributes. */
	node->open = (int32_t (*)(struct file_vnode *, struct task *, uint8_t))open;
	node->close = (int32_t (*)(struct file *))close;
	node->read = (int64_t (*)(struct file *, void *, size_t))read;
	node->write = (int64_t (*)(struct file *, void *, size_t))write;
	node->type = type;


	/* Every node has a semaphore with a count of one. */
	node->mutex = create_semaphore(1);

	if ((type == FILE_PIPE_UNNAMED) || (type == FILE_PIPE_NAMED)) {
		/* Every pipe needs a read/write queue, so that processes can wait
		 * until some data has been written to/read from it.
		 */
		node->read_queue = kmalloc(sizeof(QUEUE));
		memset(node->read_queue, 0, sizeof(QUEUE));

		node->write_queue = kmalloc(sizeof(QUEUE));
		memset(node->write_queue, 0, sizeof(QUEUE));
	}

	/* Now we need to link the node. */
	if (vnodes == NULL) {
		/* There aren't any other nodes. */
		node->next = NULL;
		node->prev = NULL;
		vnodes = node;
	} else {
		node->next = vnodes;
		node->prev = NULL;
		vnodes->prev = node;
		vnodes = node;
	};

	release_semaphore(&vnodes_semaphore);
	return node;
};


uint8_t vfs_destroy_node(struct file_vnode *node) {
	if (node == NULL) { return 1; }

	/* There mustn't be any streams open to this node. */
	if (node->reader_count != 0) { return 1; }
	if (node->writer_count != 0) { return 1; }

	/* First we need to unlink the node. */
	acquire_semaphore(node->mutex);
	acquire_semaphore(&vnodes_semaphore);
	if (node->next != NULL) {
		node->next->prev = node->prev;
	}
	if (node->prev != NULL) {
		node->prev->next = node->next;
	}
	if (node == vnodes) {
		vnodes = vnodes->next;
	}
	release_semaphore(&vnodes_semaphore);
	release_semaphore(node->mutex);

	/* Now we can safely free the node. */
	if (node->special != NULL) {
		if ((node->fs != NULL) && (node->fs->fs_type == FS_FAT16)) {
			kfree(((FAT16_FILE*)node->special)->entry);
		}
		kfree(node->special);
	}
	if (node->mutex != NULL) {
		/* BUG: If there are processes waiting for this semaphore, then
		 * they will stay blocked forever. This should be fixed in the future.
		 */
		kfree(node->mutex);
	}
	if (node->file_name != NULL) {
		kfree(node->file_name);
	}
	/* Free the QUEUEs.*/
	if ((node->type == FILE_PIPE_UNNAMED) || (node->type == FILE_PIPE_NAMED)) {
		kfree(node->read_queue);
		kfree(node->write_queue);
	}

	kfree(node);

	return 0;
};

struct file_vnode *vfs_vnode_lookup(char *file_name) {
	/* Finds a vnode given a file name.
	 *
	 * TODO: Since the entire list has just one
	 * semaphore, this slows things A WHOLE FUCKING LOT. This should be fixed,
	 * and the vnodes should take a more hierarchical structure, so that one
	 * task creating a new node doesn't stop another task reading from an
	 * unrelated pipe.
	 */
	acquire_semaphore(&vnodes_semaphore);

	struct file_vnode *i = vnodes;

	while (i) {
		if (!strcmp(i->file_name, file_name)) {
			break;
		}
		i = i->next;
	}
	release_semaphore(&vnodes_semaphore);
	return i;
};


#ifdef DEBUG
#include <tty.h>

void vfs_print_nodes() {
	kputs("VNODES: \n {NAME}: {TYPE} {LAST} {ADDRESS} {READER C} {WRITER C}\n");

	struct file_vnode *i = vnodes;

	while (i) {
		kputs(i->file_name);
		kputs(": ");

		kputx(i->type);
		kputs(" ");
		kputx(i->last);
		kputs(" ");

		kputx((uintptr_t)i);
		kputs(" ");

		kputx(i->reader_count);
		kputs(" ");
		kputx(i->writer_count);
		kputs("\n");

		i = i->next;
	}
};

#endif
