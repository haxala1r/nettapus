#include <fs/fs.h>
#include <fs/ext2.h>

#include <err.h>
#include <mem.h>


void *ext2_open_file(struct file_system *fs, char *path) {
	if (fs == NULL)               { return NULL; }
	if (fs->fs_type != FS_EXT2)   { return NULL; }
	if (path == NULL)             { return NULL; }

	/* Currently, this looks for the file in the root directory.
	 * TODO: add support for full path parsing.
	 */
	struct ext2_inode *root = kmalloc(sizeof(*root));
	struct ext2_fs *e2fs = fs->special;
	ext2_load_inode(fs, 2, root);


	size_t block = 0;
	size_t block_addr = 0;
	size_t block_size = 1024 << e2fs->sb->log2_block_size;
	void *block_buf = kmalloc(block_size);

	/* This will be set to the file's directory entry. */
	struct ext2_dir_entry *entry;
	while (1) {
		block_addr = ext2_inode_block(root, block++);
		if (ext2_load_block(fs, block_addr, block_buf)) {
			/* We reached the end of the directory, and couldn't find the file. */
			goto _file_not_found;
		}

		size_t i = 0;
		while (1) {
			entry = block_buf + i;
			if (entry->size == 0) {
				/* The end of the directory. File not found. */
				goto _file_not_found;
			}

			if (!strcmp(&(entry->nm), path)) {
				/* Found the file. */
				goto _file_found;
			}

			i += entry->size;
			if (i >= block_size) {
				break;	/* skip to the next block. */
			}
		}
	}
_file_not_found:
	kfree(block_buf);
	kfree(root);
	return NULL;

_file_found:
	kfree(root);
	size_t inode_num = entry->inode;
	kfree(block_buf);

	struct ext2_inode *ret = kmalloc(sizeof(*ret));
	ext2_load_inode(fs, inode_num, ret);

	return ret;
};
