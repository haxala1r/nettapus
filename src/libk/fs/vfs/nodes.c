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
FILE_VNODE *vnodes;

SEMAPHORE vnodes_semaphore = {
	.current_count = 0,
	.max_count = 1,
	.first_waiting_task = NULL,
	.last_waiting_task = NULL
};


FILE_VNODE *vfs_create_node(uintptr_t open, uintptr_t close, uintptr_t read,
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

	FILE_VNODE *node = kmalloc(sizeof(FILE_VNODE));

	/* Clear out the structure. */
	memset(node, 0, sizeof(*node));

	/* Set the given attributes. */
	node->open = (int32_t (*)(FILE_VNODE *, TASK *, uint8_t))open;
	node->close = (int32_t (*)(FILE *))close;
	node->read = (int64_t (*)(FILE *, void *, size_t))read;
	node->write = (int64_t (*)(FILE *, void *, size_t))write;
	node->type = type;


	/* Every node has a semaphore with a count of one. */
	node->semaphore = create_semaphore(1);

	if (type == FILE_PIPE) {
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


uint8_t vfs_destroy_node(FILE_VNODE *node) {
	if (node == NULL) { return 1; }

	/* There mustn't be any streams open to this node. */
	if (node->reader_count != 0) { return 1; }
	if (node->writer_count != 0) { return 1; }


	/* First we need to unlink the node. */
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

	/* Now we can safely free the node. */
	if (node->special != NULL) {
		if ((node->fs != NULL) && (node->fs->fs_type == FS_FAT16)) {
			kfree(((FAT16_FILE*)node->special)->entry);
		}
		kfree(node->special);
	}
	if (node->semaphore != NULL) {
		/* BUG: If there are processes waiting for this semaphore, then
		 * they will stay blocked forever. This should be fixed in the future.
		 */
		kfree(node->semaphore);
	}
	if (node->file_name != NULL) {
		kfree(node->file_name);
	}
	if (node->type == FILE_PIPE) {
		kfree(node->read_queue);
		kfree(node->write_queue);
	}

	kfree(node);

	return 0;
};

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





