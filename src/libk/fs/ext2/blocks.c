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
	if (e2fs->sb == NULL)                       { return ERR_INVALID_PARAM; }
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


uint8_t ext2_write_block(struct file_system *fs, size_t block_addr, void *buf,
                        size_t offset, size_t bytes) {
	if (fs == NULL) { return ERR_INVALID_PARAM; }
	if (buf == NULL) { return ERR_INVALID_PARAM; }

	struct ext2_fs *e2fs = fs->special;
	if ((offset + bytes) > e2fs->block_size) {
		return ERR_INVALID_PARAM;
	}

	void *block_buf = kmalloc(e2fs->block_size);

	ext2_load_block(fs, block_addr, block_buf);
	memcpy(block_buf + offset, buf, bytes);

	if (fs_write_sectors(fs, block_addr * e2fs->block_size / 512, e2fs->block_size / 512, block_buf)) {
		kfree(block_buf);
		return ERR_DISK;
	}
	kfree(block_buf);
	return GENERIC_SUCCESS;
};



uint8_t ext2_alloc_blocks(struct file_system *fs, struct ext2_file *f, size_t blocks) {
	if (fs == NULL)           { return ERR_INVALID_PARAM; }
	if (fs->special == NULL)  { return ERR_INVALID_PARAM; }
	if (f == NULL)            { return ERR_INVALID_PARAM; }

	struct ext2_fs *e2fs = fs->special;
	struct ext2_group_des *gd = kmalloc(e2fs->block_size);

	size_t gd_entry_count = e2fs->sb->inode_count / e2fs->sb->inodes_in_group;
	size_t gd_block_count = gd_entry_count * sizeof(struct ext2_group_des) / e2fs->block_size;
	gd_block_count += !!(gd_entry_count * sizeof(struct ext2_group_des) % e2fs->block_size);
	size_t gd_block = e2fs->group_des_table_block;


	for (size_t i = 0; i < gd_block_count; i++) {
		ext2_load_block(fs, gd_block + i, gd);

		for (size_t j = 0; j < (e2fs->block_size / sizeof(*gd)); j++) {
			/* The middle loop iterates over the group descriptors in the block. */
			if (gd[j].free_block_count == 0) {
				continue;
			}
			uint8_t *gb_bitmap = kmalloc(e2fs->block_size);
			ext2_load_block(fs, gd[j].block_bitmap_addr, gb_bitmap);

			for (size_t k = 0; k < e2fs->block_size; k++) {
				/* The innermost loop iterates through the bits in the
				 * block usage bitmap of the block group.
				 */
				for (size_t bit = 0; bit < 8; bit++) {
					if ((gb_bitmap[k] & (1 << bit)) == 0) {
						/* Found a free block, mark it used, and add it. */
						gb_bitmap[k] |= (1 << bit);
						ext2_inode_add_block(fs, f->node, k * 8 + bit);
						blocks--;
						if (blocks == 0) {
							kfree(gb_bitmap);
							goto done;
						}
					}
				}

			}

			ext2_write_block(fs, gd[j].block_bitmap_addr, gb_bitmap, 0, e2fs->block_size);
			kfree(gb_bitmap);
		}
	};
done:
	kfree(gd);
	return GENERIC_SUCCESS;
};



struct ext2_group_des ext2_get_group_des(struct file_system *fs, size_t group) {

	struct ext2_fs *e2fs = fs->special;
	size_t offset;
	size_t lba;

	offset = (group * sizeof(struct ext2_group_des));
	lba = (e2fs->group_des_table_block * e2fs->block_size + offset);
	lba /= 512;

	/* For efficiency reasons, we'll only read a single sector. */
	void *disk_buf = kmalloc(512);

	if (fs_read_sectors(fs, lba, 1, disk_buf)) {
		kfree(disk_buf);
		struct ext2_group_des ret = {};
		return ret;
	}

	struct ext2_group_des ret;

	memcpy(&ret, disk_buf + offset % 512, sizeof(ret));

	kfree(disk_buf);
	return ret;
};



