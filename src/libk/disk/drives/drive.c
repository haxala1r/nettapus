#include <disk/disk.h>
#include <mem.h>
#include <fs/fs.h>


size_t check_drive(struct drive *d) {
	if (d == NULL) { return -1; }

	void *buf = kmalloc(512);

	if (drive_read_sectors(d, buf, 1, 1)) {
		kfree(buf);
		return DRIVE_TYPE_RAW;
	}

	if (!memcmp(buf, "EFI PART", 8)) {
		kfree(buf);
		return DRIVE_TYPE_GPT;
	};
	kfree(buf);

	/* Check for any valid file systems. */
	int fs = fs_check_drive(d);


	if (fs) {
		/* Currently, we don't care for the exact file system. That will be
		 * handled in the file_system struct.*/
		return DRIVE_TYPE_FS;
	}

	return DRIVE_TYPE_MBR;
}



