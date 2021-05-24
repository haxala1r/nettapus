#include <mem.h>
#include <err.h>
#include <fs/ext2.h>
#include <fs/fs.h>


size_t ext2_mknod(struct file_system *fs, size_t parent_inode, char *file_name, uint16_t type_perm, size_t uid, size_t gid) {
	if (fs == NULL)          { return 0; }
	if (fs->special == NULL) { return 0; }
	if (parent_inode < 2)    { return 0; }

	struct ext2_fs *e2fs = fs->special;

	/* First, allocate a new directory entry within the parent directory. */
	struct ext2_inode *parent = kmalloc(sizeof(*parent));
	ext2_load_inode(fs, parent, parent_inode);

	uint8_t *buf = kmalloc(e2fs->block_size);
	size_t block = 0;
	size_t block_addr;
	struct ext2_dir_entry *entry;
	while (1) {
		block_addr = ext2_block_in_inode(parent, block);
		if (ext2_load_blocks(fs, buf, block_addr, 1)) {
			kfree(buf);
			kfree(parent);
			return 0;

		};

		size_t i = 0;
		while (1) {
			entry = (void *)(buf + i);
			if (entry->size == 0) {
				goto success;
			}

			i += entry->size;
		}
		block++;
	}

success:
	/* Now all we need to do is to create an inode, link the entry to it, then
	 * write both of them back.
	 * Create the inode.
	 */
	kfree(parent);
	size_t inode_num = ext2_alloc_inode(fs);
	struct ext2_inode *inode = kmalloc(sizeof(*inode));
	memset(inode, 0, sizeof(*inode));

	inode->type_perm = type_perm;

	/* GCC gives a warning if we pass the pointer directly. */
	uint32_t *bptr = (uint32_t *)((uint8_t *)inode) + offsetof(struct ext2_inode,direct_block);
	ext2_alloc_blocks(fs, 1, bptr);

	inode->uid = uid;
	inode->gid = gid;
	inode->size_low = 0;
	inode->size_high = 0;
	inode->hard_links = 1;
	inode->sector_count = e2fs->block_size / 512;
	/* TODO: set creation date etc. etc. */

	/* Write the inode to disk. */
	if (ext2_write_inode(fs, inode, inode_num)) {
		kfree(buf);
		return 0;
	}
	kfree(inode);

	/* Write the directory entry to disk. */
	entry->inode = inode_num;
	entry->name_len = strlen(file_name);
	entry->size = entry->name_len + 8;
	entry->type = type_perm >> 24;
	memcpy(&entry->nm, file_name, entry->name_len + 1); /* TODO: check if there's enough memory. */

	if (ext2_write_blocks(fs, buf, block_addr, 1)) {
		kfree(buf);
		return 0;
	}
	kfree(buf);
	return GENERIC_SUCCESS;
}
