#include <fs/fs.h>
#include <fs/ext2.h>

#include <err.h>
#include <mem.h>


uint8_t ext2_init_fs(struct file_system *fs) {
	if (fs == NULL) { return ERR_INVALID_PARAM; }

	struct ext2_superblock *sb = kmalloc(512);
	if (sb == NULL) { return ERR_OUT_OF_MEM; }

	if (fs_read_sectors(fs, 2, 1, sb)) {
		kfree(sb);
		return ERR_DISK;
	}

	if (sb->ext2_signature != 0xef53) {
		/* The FS isn't ext2. */
		kfree(sb);
		return ERR_INCOMPAT_PARAM;
	}


	struct ext2_fs *fs_ext2 = kmalloc(sizeof(struct ext2_fs));
	fs_ext2->sb = kmalloc(sizeof(struct ext2_superblock));

	memcpy(fs_ext2->sb, sb, sizeof(struct ext2_superblock));
	kfree(sb);

	fs_ext2->group_des_table_block = fs_ext2->sb->log2_block_size ? 1 : 2;
	fs_ext2->block_size = (1024 << fs_ext2->sb->log2_block_size);
	fs->special = fs_ext2;

	return GENERIC_SUCCESS;
};

