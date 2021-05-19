#include <err.h>
#include <mem.h>
#include <disk/disk.h>
#include <fs/fs.h>
#include <fs/ext2.h>

size_t ext2_load_blocks(struct file_system *fs, void *buf, size_t block_addr, size_t count) {
	if (fs == NULL)   { return ERR_INVALID_PARAM; }
	if (buf == NULL)  { return ERR_INVALID_PARAM; }

	struct ext2_fs *e2fs = fs->special;
	if (e2fs == NULL) { return ERR_INVALID_PARAM; }

	/* Where to read from and how much to read, in bytes. */
	size_t addr = block_addr * e2fs->block_size;
	size_t amount = count * e2fs->block_size;

	/* Same as addr and amount, but in sectors. */
	size_t lba = addr / 512;
	size_t sector_count = amount / 512;

	if (drive_read_sectors(fs->dev, buf, lba, sector_count)) {
		return ERR_DISK;
	}

	return GENERIC_SUCCESS;
};

size_t ext2_write_blocks(struct file_system *fs, void *buf, size_t block_addr, size_t count) {
	if (fs == NULL)   { return ERR_INVALID_PARAM; }
	if (buf == NULL)  { return ERR_INVALID_PARAM; }

	struct ext2_fs *e2fs = fs->special;
	if (e2fs == NULL) { return ERR_INVALID_PARAM; }

	/* Where to read from and how much to read, in bytes. */
	size_t addr = block_addr * e2fs->block_size;
	size_t amount = count * e2fs->block_size;

	/* Same as addr and amount, but in sectors. */
	size_t lba = addr / 512;
	size_t sector_count = amount / 512;

	if (drive_write_sectors(fs->dev, buf, lba, sector_count)) {
		return ERR_DISK;
	}

	return GENERIC_SUCCESS;
};


size_t ext2_load_group_des(struct file_system *fs, struct ext2_group_des *dest, size_t group) {
	if (fs == NULL)   { return ERR_INVALID_PARAM; }
	if (dest == NULL) { return ERR_INVALID_PARAM; }

	struct ext2_fs *e2fs = fs->special;
	if (e2fs == NULL) { return ERR_INVALID_PARAM; }

	if (group > e2fs->group_count) {
		return ERR_INVALID_PARAM;
	}

	/* Number of group descriptors stored in each block of the group desc table. */
	size_t groups_per_block = e2fs->block_size / sizeof(*dest);

	/* Block number our entry is stored in. */
	size_t block = group / groups_per_block + e2fs->group_des_table_block;
	size_t index = group % groups_per_block;

	struct ext2_group_des *buf = kmalloc(e2fs->block_size);
	if (ext2_load_blocks(fs, buf, block, 1)) {
		kfree(buf);
		return ERR_DISK;
	}

	memcpy(dest, buf + index, sizeof(*dest));
	kfree(buf);
	return GENERIC_SUCCESS;
};


size_t ext2_alloc_blocks(struct file_system *fs, size_t count, uint32_t *ret) {
	if (fs == NULL)          { return ERR_INVALID_PARAM; }
	if (fs->special == NULL) { return ERR_INVALID_PARAM; }

	struct ext2_fs *e2fs = fs->special;
	struct ext2_group_des gdes;

	/* How many blocks to read at a time. */
	size_t to_read = e2fs->sb->blocks_in_group / e2fs->block_size / 8;
	to_read += !!(e2fs->sb->blocks_in_group % e2fs->block_size);

	uint8_t *buf = kmalloc(to_read * e2fs->block_size);


	for (size_t i = 0; i < e2fs->group_count; i++) {
		/* Load the group descriptor. */
		if (ext2_load_group_des(fs, &gdes, i)) {
			goto fail;
		};
		if (count > gdes.free_block_count) {
			continue;
		}

		/* Read the block usage bitmap into memory. */
		if (ext2_load_blocks(fs, buf, gdes.block_bitmap_addr, to_read)) {
			goto fail;
		}

		/* Iterate over this group's block usage bitmap. */
		for (size_t j = 0; j < e2fs->sb->blocks_in_group; j++) {
			if ((buf[j/8] & (1 << (j % 8))) == 0) {
				buf[j/8] |= (1 << (j % 8));

				*ret = i * e2fs->sb->blocks_in_group + j;  /* Set the block address.*/
				ret++;
				count--;
				if (count == 0) {
					break;
				}
			}
		}
		ext2_write_blocks(fs, buf, gdes.block_bitmap_addr, to_read);
		if (count == 0) {
			goto success;
		}
	}
fail:
	kfree(buf);
	return ERR_NO_RESULT;
success:
	kfree(buf);
	return GENERIC_SUCCESS;
};




