#include <fs/fs.h>
#include <fs/ext2.h>

#include <err.h>
#include <mem.h>


/* Searches a directory given its inode number, and returns the directory entry. */
struct ext2_dir_entry *ext2_search_dir(struct file_system *fs, char *file_name, size_t inode_num) {
	if (fs == NULL)          { return NULL; }
	if (file_name == NULL)    { return NULL; }
	if (fs->special == NULL) { return NULL; }

	struct ext2_inode *dir = kmalloc(sizeof(*dir));
	struct ext2_fs *e2fs = fs->special;
	ext2_load_inode(fs, inode_num, dir);

	size_t block = 0;
	size_t block_addr = 0;
	size_t block_size = 1024 << e2fs->sb->log2_block_size;
	void *block_buf = kmalloc(block_size);

	struct ext2_dir_entry *entry;
	while (1) {
		block_addr = ext2_inode_block(dir, block);
		if (ext2_load_block(fs, block_addr, block_buf)) {
			goto file_not_found;
		}

		size_t i = 0;
		while(i < block_size) {
			entry = block_buf + i;
			if (entry->size == 0) {
				goto file_not_found;
			}

			if (!strcmp(&entry->nm, file_name)) {
				goto file_found;
			}
			i += entry->size;
		}
	}

file_not_found:
	kfree(dir);
	kfree(block_buf);
	return NULL;

file_found:
	kfree(dir);

	struct ext2_dir_entry *ret = kmalloc(sizeof(*ret));
	memcpy(ret, entry, sizeof(*ret));

	kfree(block_buf);
	return ret;
};


/* Finds a file in the file system and loads its i-node. */
void *ext2_open_file(struct file_system *fs, char *path) {
	if (fs == NULL)               { return NULL; }
	if (fs->fs_type != FS_EXT2)   { return NULL; }
	if (path == NULL)             { return NULL; }

	/* Iterate through the directories and look for the file. */
	struct ext2_dir_entry *n = NULL;
	char **path_arr = fs_parse_path(path);

	char **cur_target = path_arr;
	size_t inode_num = 2;

	while (*cur_target != NULL) {
		n = ext2_search_dir(fs, *cur_target, inode_num);
		if (n == NULL) {
			/* File not found. */
			fs_free_path(path_arr);
			return NULL;
		}

		inode_num = n->inode;
		kfree(n);

		cur_target++;
	}

	/* n should now hold the directory entry of the file. */
	struct ext2_file *ret = kmalloc(sizeof(*ret));
	ret->inode_addr = inode_num;
	ret->node = kmalloc(sizeof(struct ext2_inode));
	ext2_load_inode(fs, inode_num, ret->node);

	fs_free_path(path_arr);
	return ret;
};
