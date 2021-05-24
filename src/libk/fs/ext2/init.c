#include <err.h>
#include <disk/disk.h>
#include <fs/fs.h>
#include <fs/ext2.h>
#include <mem.h>



uint8_t ext2_init_fs(struct file_system *fs) {
	if (fs == NULL)       { return ERR_INVALID_PARAM; }
	if (fs->dev == NULL)  { return ERR_INVALID_PARAM; }

	struct ext2_superblock *sb = kmalloc(512);

	if (drive_read_sectors(fs->dev, sb, 2, 1)) {
		kfree(sb);
		return ERR_DISK;
	}

	if (sb->ext2_signature != 0xef53) {
		kfree(sb);
		return ERR_INCOMPAT_PARAM;
	}

	struct ext2_fs *e2fs = kmalloc(sizeof(*e2fs));
	e2fs->sb = sb;
	e2fs->block_size = (1024 << sb->log2_block_size);
	if (e2fs->block_size == 1024) {
		e2fs->group_des_table_block = 2;
	} else {
		e2fs->group_des_table_block = 1;
	}
	e2fs->group_count = sb->inode_count / sb->inodes_in_group + !!(sb->inode_count % sb->inodes_in_group);

	if (e2fs->group_count != (sb->block_count / sb->blocks_in_group + !!(sb->block_count % sb->blocks_in_group))) {
		kfree(sb);
		kfree(e2fs);
		return ERR_INCOMPAT_PARAM;
	}

	fs->special = e2fs;
	return GENERIC_SUCCESS;
}

uint8_t ext2_check_drive(struct drive *d) {
	if (d == NULL) { return ERR_INVALID_PARAM; }

	struct ext2_superblock *sb = kmalloc(512);

	if (drive_read_sectors(d, sb, 2, 1)) {
		kfree(sb);
		return ERR_DISK;
	}

	if (sb->ext2_signature != 0xef53) {
		kfree(sb);
		return ERR_INCOMPAT_PARAM;
	}

	kfree(sb);
	return GENERIC_SUCCESS;
}
