#include <mem.h>

#include <fs/ext2.h>
#include <fs/fs.h>


struct file_tnode *ext2_list_files(struct file_system *fs, size_t inode, size_t *count) {
	if (fs == NULL) { return NULL; }
	if (inode < 2)  { return NULL; }

	struct ext2_fs *e2fs = fs->special;
	if (e2fs == NULL) { return NULL;}

	struct ext2_inode *parent = kmalloc(sizeof(*parent));
	if (parent == NULL) { return NULL; }
	if (ext2_load_inode(fs, parent, inode)) {
		kfree(parent);
		return NULL;
	};

	/* These may look like they only work for file tnodes, but since the structures
	 * are practically the same with just namechanges, the same code will work
	 * for folders too.
	 */
	struct file_tnode *first_tnode = NULL;
	struct file_tnode *last_tnode = NULL;

	void *block_buf = kmalloc(e2fs->block_size);

	size_t block = 0;
	size_t block_addr;
	*count = 0;

	while (1) {
		/* Load the current block. */
		block_addr = ext2_block_in_inode(parent, block);
		if (block_addr == 0) {
			break;
		}
		if (ext2_load_blocks(fs, block_buf, block_addr, 1)) {
			kfree(block_buf);
			kfree(parent);
			return NULL;
		};

		/* Iterate over every entry in this block. */
		struct ext2_dir_entry *entry = block_buf;
		size_t i = 0;
		while (1) {
			entry = block_buf + i;
			if (entry->size < sizeof(*entry)) {
				goto done;  /* This must be the last entry. */
			}

			/* If the entry isn't for a directory, add the file to the list. */
			if (entry->type != 2) {
				if (first_tnode == NULL){
					first_tnode = kmalloc(sizeof(struct file_tnode));
					last_tnode = first_tnode;
				} else {
					last_tnode->next = kmalloc(sizeof(struct file_tnode));
					last_tnode = last_tnode->next;
				}

				last_tnode->file_name = kmalloc(strlen(&entry->nm) + 1);
				memcpy(last_tnode->file_name, &entry->nm, strlen(&entry->nm) + 1);
				last_tnode->vnode = NULL;
				last_tnode->next = NULL;
				*count += 1;
			}

			/* Next entry. */
			i += entry->size;
			if (i >= e2fs->block_size) {
				break;
			}
		}

		/* Next block. */
		block++;
	}
done:
	kfree(block_buf);
	kfree(parent);
	return first_tnode;
};


struct folder_tnode *ext2_list_folders(struct file_system *fs, size_t inode, size_t *count) {
	if (fs == NULL) { return NULL; }
	if (inode < 2)  { return NULL; }

	struct ext2_fs *e2fs = fs->special;
	if (e2fs == NULL) { return NULL;}

	struct ext2_inode *parent = kmalloc(sizeof(*parent));
	if (parent == NULL) { return NULL; }
	if (ext2_load_inode(fs, parent, inode)) {
		kfree(parent);
		return NULL;
	};

	struct folder_tnode *first_tnode = NULL;
	struct folder_tnode *last_tnode = NULL;

	void *block_buf = kmalloc(e2fs->block_size);

	size_t block = 0;
	size_t block_addr;
	*count = 0;

	while (1) {
		/* Load the current block. */
		block_addr = ext2_block_in_inode(parent, block);
		if (block_addr == 0) {
			break;
		}
		if (ext2_load_blocks(fs, block_buf, block_addr, 1)) {
			kfree(block_buf);
			kfree(parent);
			return NULL;
		};

		/* Iterate over every entry in this block. */
		struct ext2_dir_entry *entry = block_buf;
		size_t i = 0;
		while (1) {
			entry = block_buf + i;
			if (entry->size < sizeof(*entry)) {
				goto done;  /* This must be the last entry. */
			}

			/* If the entry is for a directory, add the folder to the list. */
			if (entry->type == 2) {
				if (first_tnode == NULL){
					first_tnode = kmalloc(sizeof(struct file_tnode));
					last_tnode = first_tnode;
				} else {
					last_tnode->next = kmalloc(sizeof(struct file_tnode));
					last_tnode = last_tnode->next;
				}

				last_tnode->folder_name = kmalloc(strlen(&entry->nm) + 1);
				memcpy(last_tnode->folder_name, &entry->nm, strlen(&entry->nm) + 1);
				last_tnode->vnode = NULL;
				last_tnode->next = NULL;
				*count += 1;
			}

			/* Next entry. */
			i += entry->size;
			if (i >= e2fs->block_size) {
				break;
			}
		}

		/* Next block. */
		block++;
	}
done:
	kfree(block_buf);
	kfree(parent);
	return first_tnode;
};


