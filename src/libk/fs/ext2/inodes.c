#include <fs/fs.h>
#include <fs/ext2.h>

#include <err.h>
#include <mem.h>


size_t ext2_inode_block(struct ext2_inode *f, size_t block_num) {
	/* Gives the block address of the block_num'th data block of an inode. */
	if (block_num < 12) {
		return f->direct_block[block_num];
	} else {
		/* TODO */
	}

	return -1;
};



uint8_t ext2_load_inode(struct file_system *fs, size_t inode_num, void *buf) {
	if (inode_num == 0) { return ERR_INVALID_PARAM; }
	if (buf == NULL)    { return ERR_INVALID_PARAM; }
	if (fs == NULL)     { return ERR_INVALID_PARAM; }

	struct ext2_fs *e2fs = fs->special;
	size_t block_size = (1024 << e2fs->sb->log2_block_size);

	size_t block_group = (inode_num - 1) / e2fs->sb->inodes_in_group;
	size_t index = (inode_num - 1) % e2fs->sb->inodes_in_group;

	/* Offset and lba of the block group descriptor table. */
	size_t offset = (e2fs->group_des_table_block * block_size + block_group * sizeof(struct ext2_group_des));
	size_t lba = offset / 512;

	/* We need to read from the block group descriptor table. */
	void *disk_buf = kmalloc(512);

	if (fs_read_sectors(fs, lba, 1, disk_buf)) {
		kfree(disk_buf);
		return ERR_DISK;
	}

	/* Now figure out the sector in which the inode resides.*/
	struct ext2_group_des *gd = disk_buf + (offset % 512);
	lba = gd->inode_table_addr * block_size / 512 + (index * e2fs->sb->inode_struct_size) / 512;

	/* Perform the read. */
	if (fs_read_sectors(fs, lba, 1, disk_buf)) {
		kfree(disk_buf);
		return ERR_DISK;
	}

	/* Copy the inode, then return. */
	struct ext2_inode *node = disk_buf + ((index * e2fs->sb->inode_struct_size) % 512);
	memcpy(buf, node, sizeof(struct ext2_inode));
	kfree(disk_buf);

	return GENERIC_SUCCESS;
};



