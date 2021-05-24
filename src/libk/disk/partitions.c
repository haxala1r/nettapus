#include <disk/disk.h>
#include <disk/ide.h>
#include <mem.h>
#include <err.h>

size_t drive_parse_mbr(struct drive *d) {
	if (d == NULL) { return 1; }

	uint8_t *mbr = kmalloc(512);

	if (drive_read_sectors(d, mbr, 0, 1)) {
		kfree(mbr);
		return ERR_DISK;
	}

	size_t parts_found = 0;

	/* Go through all entries.
	 * TODO: it might be a good idea to go through the entries twice, checking
	 * their validity the first time, and then actually adding them in the second
	 * time.
	 */
	for (uint8_t i = 0; i < 4; i++) {
		uint8_t *mbre = (uint8_t*)(mbr + 0x1BE + i * 0x10);

		/* Check the Partition Type field. If non-zero, the partition is used. */
		if (mbr[0x1BE + i * 0x10 + 4] != 0) {
			/* Create a new drive object. */
			struct drive *new_drive = kmalloc(sizeof(struct drive));
			new_drive->dev = d->dev;
			new_drive->starting_sector = mbre[8] | (mbre[9] << 8) | (mbre[10] << 16) | (mbre[11] << 24);
			new_drive->sector_count = mbre[12] | (mbre[13] << 8) | (mbre[14] << 16) | (mbre[15] << 24);
			new_drive->type = check_drive(new_drive);

			if (register_drive(new_drive)) {
				kfree(mbr);
				return 0;	/* It's invalid. */
			};
			parts_found++;
		}
	}

	kfree(mbr);
	return parts_found;

}
