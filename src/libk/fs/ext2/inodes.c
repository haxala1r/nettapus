#include <err.h>
#include <mem.h>
#include <string.h>
#include <disk/disk.h>
#include <fs/fs.h>
#include <fs/ext2.h>


size_t ext2_block_in_inode(struct ext2_inode *node, size_t n) {
	/* Returns the nth data block of the inode. */
	if (node == NULL) { return 0; }

	if (n < 12) {
		return node->direct_block[n];
	}
	/* TODO: handle indirect blocks. */
	return 0;
}


size_t ext2_load_inode(struct file_system *fs, struct ext2_inode *dest, size_t inode_num) {
	if (fs == NULL)          { return ERR_INVALID_PARAM; }
	if (fs->special == NULL) { return ERR_INVALID_PARAM; }

	struct ext2_fs *e2fs = fs->special;

	size_t group = (inode_num - 1) / e2fs->sb->inodes_in_group;
	size_t index = (inode_num - 1) % e2fs->sb->inodes_in_group;
	size_t containing_block = index * sizeof(*dest) / e2fs->block_size;

	/* Load the group descriptor table entry. */
	struct ext2_group_des *gd = kmalloc(sizeof(*gd));
	if (ext2_load_group_des(fs, gd, group)) {
		kfree(gd);
		return ERR_DISK;
	};

	/* Get the block address of the inode table, then load and search the inode table. */
	struct ext2_inode *buf = kmalloc(e2fs->block_size);
	size_t block = gd->inode_table_addr + containing_block;
	kfree(gd);

	if (ext2_load_blocks(fs, buf, block, 1)) {
		kfree(buf);
		return ERR_DISK;
	}

	memcpy(dest, buf + (index % (e2fs->block_size / sizeof(*dest))), sizeof(*dest));
	kfree(buf);

	return GENERIC_SUCCESS;
}

size_t ext2_write_inode(struct file_system *fs, struct ext2_inode *inode, size_t inode_num) {
	if (fs == NULL)          { return ERR_INVALID_PARAM; }
	if (inode == NULL)       { return ERR_INVALID_PARAM; }
	if (fs->special == NULL) { return ERR_INVALID_PARAM; }

	struct ext2_fs *e2fs = fs->special;

	size_t group = (inode_num - 1) / e2fs->sb->inodes_in_group;
	size_t index = (inode_num - 1) % e2fs->sb->inodes_in_group;
	size_t block = (index * sizeof(*inode)) / e2fs->block_size;
	size_t offset = (index * sizeof(*inode)) % e2fs->block_size;

	/* Load the group descriptor and find the inode table. */
	struct ext2_group_des gdes;

	if (ext2_load_group_des(fs, &gdes, group)) {
		return ERR_DISK;
	}

	block += gdes.inode_table_addr;
	uint8_t *buf = kmalloc(e2fs->block_size);

	/* Load the specified block of the inode table. */
	if (ext2_load_blocks(fs, buf, block, 1)) {
		kfree(buf);
		return ERR_DISK;
	}

	/* Modify & write back. */
	memcpy(buf + offset, inode, sizeof(*inode));

	if (ext2_write_blocks(fs, buf, block, 1)) {
		kfree(buf);
		return ERR_DISK;
	}

	kfree(buf);
	return GENERIC_SUCCESS;
}


size_t ext2_alloc_inode(struct file_system *fs) {
	if (fs == NULL)          { return 0; }
	if (fs->special == NULL) { return 0; }

	struct ext2_fs *e2fs = fs->special;
	struct ext2_group_des gdes;

	/* How many blocks to read at a time. */
	size_t to_read = e2fs->sb->inodes_in_group / e2fs->block_size / 8;
	to_read += !!(e2fs->sb->inodes_in_group % e2fs->block_size);

	uint8_t *buf = kmalloc(to_read * e2fs->block_size);

	for (size_t i = 0; i < e2fs->group_count; i++) {
		if (ext2_load_group_des(fs, &gdes, i)) {
			return 0;
		};

		if (gdes.free_inode_count == 0) {
			continue;
		}

		/* Load the inode usage bitmap. */

		if (ext2_load_blocks(fs, buf, gdes.inode_bitmap_addr, to_read)) {
			kfree(buf);
			return 0;
		}

		/* Iterate over the inode usage bitmap. */
		for (size_t j = 0; j < e2fs->sb->inodes_in_group; j++) {
			if ((buf[j/8] & (1 << (j%8))) == 0) {
				buf[j/8] |= (1 << (j%8));
				ext2_write_blocks(fs, buf, gdes.inode_bitmap_addr, to_read);
				return i * e2fs->sb->inodes_in_group + j;
			}
		}
	}
	kfree(buf);
	return 0;
}


int64_t ext2_get_size(struct file_system *fs, size_t inode_num) {
	if (fs == NULL)          { return -ERR_INVALID_PARAM; }
	if (fs->special == NULL) { return -ERR_INVALID_PARAM; }

	struct ext2_inode inode;

	if (ext2_load_inode(fs, &inode, inode_num)) {
		return -ERR_DISK;
	}

	size_t file_size = ((size_t)inode.size_high << 32) | (inode.size_low);
	return file_size;
}

int64_t ext2_get_links(struct file_system *fs, size_t inode_num) {
	if (fs == NULL)          { return -ERR_INVALID_PARAM; }
	if (fs->special == NULL) { return -ERR_INVALID_PARAM; }

	struct ext2_inode inode;

	if (ext2_load_inode(fs, &inode, inode_num)) {
		return -ERR_DISK;
	}

	return inode.hard_links;
}

uint16_t ext2_get_type_perm(struct file_system *fs, size_t inode_num) {
	if (fs == NULL) { return 0xFFFF; }
	if (inode_num < 2) { return 0xFFFF; }

	struct ext2_inode inode;

	if (ext2_load_inode(fs, &inode, inode_num)) {
		return -ERR_DISK;
	}

	return inode.type_perm;
}


