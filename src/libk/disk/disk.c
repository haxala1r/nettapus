#include <disk/disk.h>
#include <disk/ide.h>
#include <mem.h>
#include <task.h>

/* Disks are physical storage mediums. Drives are a range of sectors within a disk.
 * They can be partitions, filesystems etc. etc. or they can be the whole disk as
 * well.
 */
struct disk *disks = NULL;
struct drive *drives = NULL;


struct drive *get_drive_list() {
	return drives;
}


struct disk *register_disk(uint8_t type, void *dptr) {
	if (dptr == NULL) { return NULL; };

	struct disk *new_disk = kmalloc(sizeof(struct disk));
	new_disk->type = type;
	new_disk->sp = dptr;
	new_disk->mutex = create_semaphore(1);
	new_disk->next = NULL;

	/* Get the sector count. No other type than IDE is defined/implemented.*/
	if (type == DISK_TYPE_IDE) {
		struct ide_drive *i = dptr;
		new_disk->sector_count = (i->lba48_sectors) ? i->lba48_sectors : i->lba28_sectors;
	} else {
		kfree(new_disk);
		return NULL;
	}

	/* Add it to the list. */
	if (disks == NULL) {
		disks = new_disk;
		new_disk->next = NULL;
	} else {
		new_disk->next = disks;
		disks = new_disk;
	}
	return new_disk;
};

size_t register_drive(struct drive *d) {
	if (d == NULL) { return 1;}

	/* Check if the drive is valid. */
	if (d->starting_sector > d->dev->sector_count) {
		return 1;
	}
	if (d->sector_count > d->dev->sector_count) {
		return 1;
	}
	if (d->sector_count == 0) {
		return 1;
	}
	if ((d->starting_sector + d->sector_count) > d->dev->sector_count) {
		return 1;
	}

	/* Add the drive to the linked list. */
	if (drives == NULL) {
		drives = d;
		d->next = NULL;
	} else {
		d->next = drives;
		drives = d;
	}

	return 0;
};


size_t register_drives(struct disk *d) {
	if (d == NULL) { return 1; }

	struct drive *new_drive = kmalloc(sizeof(*new_drive));
	memset(new_drive, 0, sizeof(*new_drive));
	new_drive->dev = d;
	new_drive->starting_sector = 0;
	new_drive->sector_count = d->sector_count;

	/* This function simply tells you what type of FS/Partitioning scheme etc.
	 * is present on the drive.
	 */
	new_drive->type = check_drive(new_drive);

	switch (new_drive->type) {
	case DRIVE_TYPE_MBR:
		if (drive_parse_mbr(new_drive) == 0) {
			new_drive->type = DRIVE_TYPE_RAW;
		}
	default:
		break;
	}


	return register_drive(new_drive);
};

uint8_t refresh_disks() {

	/* If the lists aren't empty already, clear the lists so we can start fresh. */
	if (disks != NULL) {
		struct disk *disk1 = disks;
		struct disk *disk2 = disk1->next;

		while (disk1 != NULL) {
			kfree(disk1);

			disk1 = disk2;
			if (disk1 == NULL) {
				break;
			}
			disk2 = disk2->next;
		}
		disks = NULL;
	}
	if (drives != NULL) {
		struct drive *drive1 = drives;
		struct drive *drive2 = drive1->next;

		while (drive1 != NULL) {
			kfree(drive1);

			drive1 = drive2;
			if (drive1 == NULL) {
				break;
			}
			drive2 = drive2->next;
		}
		drives = NULL;
	}

	/* Loop over every available disk, and add them to the list. */
	struct ide_drive *i = ide_get_first_drive();

	while (i != NULL) {
		struct disk *nd = register_disk(DISK_TYPE_IDE, i);

		/* Now register the drives present on disk. */
		if (register_drives(nd)) {
			/* Something bad happened. */
			return 1;
		}

		i = i->next;
	};

	return 0;
};

uint8_t init_disk() {
	/* Currently our only way of accessing a disk is through IDE. */
	if (init_ide()) {
		return 1;
	}

	return refresh_disks();
};
