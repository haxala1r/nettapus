#include <fs/fs.h>
#include <fs/ext2.h>

#include <err.h>
#include <mem.h>

uint8_t ext2_write_file(struct file_system *fs, void *file_void, void *buf,
                     size_t offset, size_t bytes) {

	if (fs == NULL)              { return ERR_INVALID_PARAM;  }
	if (fs->special == NULL)     { return ERR_INVALID_PARAM;  }
	if (fs->fs_type != FS_EXT2)  { return ERR_INCOMPAT_PARAM; }
	if (file_void == NULL)        { return ERR_INVALID_PARAM;  }
	if (buf == NULL)             { return ERR_INVALID_PARAM;  }

	struct ext2_fs *e2fs = fs->special;
	struct ext2_file *f = file_void;


	size_t file_size = ((uint64_t)f->node->size_high << 32) | ((uint64_t)f->node->size_low);
	size_t block_size = (1024 << e2fs->sb->log2_block_size);

	size_t block = offset / bytes;
	size_t block_addr;
	size_t to_write = 0;

	if ((offset + bytes) > file_size) {
		/* Allocate more blocks. */
		size_t to_add = offset + bytes - file_size;
		size_t blocks_needed = to_add + file_size % block_size;
		blocks_needed = blocks_needed / block_size + !!(blocks_needed % block_size);

		/* Update size information on disk */
		file_size = offset + bytes;
		f->node->size_low = (uint32_t)(file_size);
		f->node->size_high = (uint32_t)(file_size >> 32);

		ext2_alloc_blocks(fs, f, blocks_needed);

		/* Sync the info on disk. */
		ext2_write_inode(fs, f->node, f->inode_addr);
	}

	while (bytes) {
		/* Get the amount to write and the block address to write. */
		block_addr = ext2_inode_block(f->node, block);
		to_write = (bytes > (block_size - offset % block_size)) ? (block_size - offset % block_size) : bytes;

		if (ext2_write_block(fs, block_addr, buf, offset % block_size, to_write)) {
			return ERR_DISK;
		}


		block++;
		buf += to_write;
		bytes -= to_write;
		offset = 0;
	}

	return GENERIC_SUCCESS;
};
