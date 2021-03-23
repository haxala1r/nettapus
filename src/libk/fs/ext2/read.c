#include <fs/fs.h>
#include <fs/ext2.h>

#include <err.h>
#include <mem.h>




uint8_t ext2_read_file(struct file_system *fs, void *file_void, void *buf,
                     size_t offset, size_t bytes) {
	if (fs == NULL)              { return ERR_INVALID_PARAM;  }
	if (fs->special == NULL)     { return ERR_INVALID_PARAM;  }
	if (fs->fs_type != FS_EXT2)  { return ERR_INCOMPAT_PARAM; }
	if (file_void == NULL)        { return ERR_INVALID_PARAM;  }
	if (buf == NULL)             { return ERR_INVALID_PARAM;  }

	struct ext2_fs *fs_ext2 = fs->special;
	struct ext2_superblock *sb = fs_ext2->sb;
	struct ext2_inode *f = file_void;
	uint64_t f_size = ((uint64_t)f->size_high << 32) | ((uint64_t)f->size_low);

	if (offset > f_size) {
		return ERR_OUT_OF_BOUNDS;
	}
	if ((offset + bytes) > f_size) {
		bytes = f_size - offset;
	}

	/* 1024 is 2^10, sb->log2_block_size holds the number to shift that left by.*/
	size_t block_size = (1024 << (sb->log2_block_size));
	size_t block = offset / block_size;
	size_t block_addr;
	size_t lba;
	size_t sector_count;
	size_t to_read;

	/* Get a buffer ready. We will read a whole block at most at a time. */
	void *block_buf = kmalloc(block_size);
	if (block_buf == NULL) { return ERR_OUT_OF_MEM; }

	/* Read the contents of the block, skip to the next block if necessary. */

	while (bytes) {
		/* The block to read from. */
		block_addr = ext2_inode_block(f, block);

		/* Determine which sector we need to start reading from. */
		lba = (offset % block_size) / 512;

		/* Determine the amount of sectors to read. If offset+bytes steps over a
		 * block boundary, only read the current block for this iteration.
		 * This specific line ensures that we only read until the end of the block
		 * at most, at a time.
		 */
		to_read = (bytes > (block_size - offset % block_size)) ? (block_size - offset % block_size) : bytes;
		sector_count = to_read / 512 + !!(to_read % 512) + !!(offset % 512);

		lba += block_addr * block_size / 512;

		if (fs_read_sectors(fs, lba, sector_count, block_buf)) {
			kfree(block_buf);
			return ERR_DISK;
		}

		memcpy(buf, block_buf + (offset % 512), to_read);

		/* offset has been taken care of. */
		offset = 0;

		buf += to_read;
		bytes -= to_read;
		block++;
	}

	/* We should be done. */
	kfree(block_buf);

	return GENERIC_SUCCESS;
};

