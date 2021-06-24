#include <err.h>
#include <disk/disk.h>
#include <fs/fs.h>
#include <fs/ext2.h>
#include <string.h>
#include <mem.h>


int64_t ext2_read_file(struct file_system *fs, size_t inode, void *dest_buf, size_t off, size_t bytes) {
	if (fs == NULL)       { return -ERR_INVALID_PARAM; }
	if (inode <= 2)       { return -ERR_INVALID_PARAM; }
	if (dest_buf == NULL) { return -ERR_INVALID_PARAM; }

	struct ext2_fs *e2fs = fs->special;

	struct ext2_inode node;
	if (ext2_load_inode(fs, &node, inode)) {
		return -ERR_DISK;
	}

	uint64_t file_size = ((uint64_t)node.size_high << 32) | ((uint64_t)node.size_low);
	if ((off + bytes) > file_size) {
		return -ERR_INVALID_PARAM;
	}

	/* Get the buffer ready.*/
	void *data_buf = kmalloc(e2fs->block_size);
	if (data_buf == NULL) { return -ERR_OUT_OF_MEM; }

	size_t block = off / e2fs->block_size;
	off = off % e2fs->block_size;
	size_t block_addr;
	size_t bytes_read = 0;

	while (bytes) {
		/* Get the block address of the next block;*/
		block_addr = ext2_block_in_inode(fs, &node, block);
		if (block_addr == 0) {
			kfree(data_buf);
			return -ERR_DISK;
		}

		/* Load the block into memory. */
		if (ext2_load_blocks(fs, data_buf, block_addr, 1)) {
			kfree(data_buf);
			return -ERR_DISK;
		}

		/* Copy the data from the block. */
		size_t to_copy = e2fs->block_size - off;
		to_copy = (to_copy > bytes) ? bytes : to_copy;
		memcpy(dest_buf, (uint8_t *)data_buf + off, to_copy);

		/* Continue. */
		bytes -= to_copy;
		dest_buf = (uint8_t *)dest_buf + to_copy;
		bytes_read += to_copy;
		off = 0;
		block++;
	}

	kfree(data_buf);
	return bytes_read;
}
