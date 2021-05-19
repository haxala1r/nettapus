#include <err.h>
#include <disk/disk.h>
#include <fs/fs.h>
#include <fs/ext2.h>
#include <string.h>
#include <mem.h>


struct ext2_dir_entry *ext2_search_dir(struct file_system *fs, struct ext2_inode *dir_node, char *file_name) {
	if (fs == NULL)       { return NULL; }
	if (dir_node == NULL) { return NULL; }
	if (file_name == NULL) { return NULL; }
	if (!(dir_node->type_perm & 0x4000)) {
		/* If the node isn't a directory. */
		return NULL;
	}

	struct ext2_fs *e2fs = fs->special;

	/* We'll iterate through every block of the directory. */
	void *dir = kmalloc(e2fs->block_size);
	struct ext2_dir_entry *entry;
	struct ext2_dir_entry *ret;
	size_t block = 0; /* Current block within the directory. */
	size_t block_addr;

	while (1) {
		block_addr = ext2_block_in_inode(dir_node, block);
		if (block_addr == 0) {
			goto fail;
		}

		if (ext2_load_blocks(fs, dir, block_addr, 1)) {
			goto fail;
		}

		entry = dir;
		size_t i = 0;
		while (1) {
			entry = dir + i;
			if (entry->size > e2fs->block_size) {
				goto fail;
			}

			if (!strcmp(&entry->nm, file_name)) {
				goto success;
			}

			i += entry->size;
			if (i >= e2fs->block_size) {
				break;
			}
		}


		block++;
	}

fail:
	kfree(dir);
	return NULL;
success:
	ret = kmalloc(sizeof(*ret));
	memcpy(ret, entry, sizeof(*ret));
	kfree(dir);
	return ret;
};



size_t ext2_open_at(struct file_system *fs, size_t folder_inode, char *full_path) {
	if (fs == NULL)        { return 0; }
	if (full_path == NULL) { return folder_inode; }

	/* If the root node is requested, there's no need to search for anything. */
	if (!strcmp(full_path, "/")) {
		return folder_inode;
	}

	char **arr = fs_parse_path(full_path);
	if (arr == NULL) { return 0; }

	struct ext2_inode *node = kmalloc(sizeof(*node));
	ext2_load_inode(fs, node, folder_inode);  /* Load the root inode. */

	struct ext2_dir_entry *entry;
	size_t i = 0;

	/* We'll need to return this after the loop - we can't do that without an
	 * extra variable because we'll have kfree()'d entry already.
	 */
	size_t inode_num = folder_inode;

	while (arr[i] != NULL) {
		entry = ext2_search_dir(fs, node, arr[i]);
		if (entry == NULL) {
			kfree(node);
			fs_free_path(arr);
			return 0;
		}

		inode_num = entry->inode;
		kfree(entry);

		/* Load the next directory. */
		if (ext2_load_inode(fs, node, inode_num)) {
			kfree(node);
			fs_free_path(arr);
			return 0;
		}

		i++;
	}

	kfree(node);
	return inode_num;
};

size_t ext2_open(struct file_system *fs, char *full_path) {
	return ext2_open_at(fs, 2, full_path);
};



