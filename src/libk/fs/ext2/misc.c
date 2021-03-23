#include <fs/fs.h>
#include <fs/ext2.h>

#include <err.h>
#include <mem.h>

size_t ext2_get_size(void *f_void) {
	struct ext2_inode *f = f_void;
	if (f == NULL) {
		return 0;
	}
	size_t ret = f->size_low;
	ret = ret << 32;
	ret = ret | f->size_high;
	return f->size_low;
};
