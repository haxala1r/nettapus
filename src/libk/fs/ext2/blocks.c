#include <fs/fs.h>
#include <fs/ext2.h>

#include <err.h>
#include <mem.h>

uint8_t ext2_load_block(struct file_system *fs, size_t block_addr, void *buf) {
	if (fs == NULL)                         { return ERR_INVALID_PARAM; }
	if (buf == NULL)                        { return ERR_INVALID_PARAM; }
	if (block_addr == 0)                    { return ERR_INVALID_PARAM; }

	struct ext2_fs *e2fs = fs->special;
	if (e2fs == NULL)                       { return ERR_INVALID_PARAM; }
	if (e2fs->sb->block_count < block_addr) { return ERR_INVALID_PARAM; }

	/* log2_block_size is slightly misnamed, see ext2.h line 20 */
	size_t block_size = 1024 << e2fs->sb->log2_block_size;

	size_t lba = block_addr * block_size / 512;
	size_t sector_count = block_size / 512;

	if (fs_read_sectors(fs, lba, sector_count, buf)) {
		return ERR_DISK;
	}

	return GENERIC_SUCCESS;
};
