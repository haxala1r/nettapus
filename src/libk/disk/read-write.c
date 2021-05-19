#include <disk/disk.h>
#include <disk/ide.h>
#include <mem.h>
#include <task.h>
#include <err.h>

size_t drive_read_sectors(struct drive *d, void *buf, size_t lba, size_t amount) {
	if (d == NULL) { return ERR_INVALID_PARAM; }
	if (buf == NULL) { return ERR_INVALID_PARAM; }

	acquire_semaphore(d->dev->mutex);

	if ((lba + amount) > d->dev->sector_count) {
		return ERR_INVALID_PARAM;
	}

	if (amount == 0) {
		return GENERIC_SUCCESS;
	}

	size_t stat = GENERIC_SUCCESS;

	if (d->dev->type == DISK_TYPE_IDE) {
		struct ide_drive *ide_d = d->dev->sp;
		if (amount < 0xFFFF) {
			stat = ide_pio_read48(ide_d->drive_id, lba + d->starting_sector, amount, buf);
		} else {
			/* TODO: handle this. */
		}
	} else {
		stat = ERR_DISK;
	}

	release_semaphore(d->dev->mutex);
	return stat;
};

size_t drive_write_sectors(struct drive *d, void *buf, size_t lba, size_t amount) {
	if (d == NULL) { return ERR_INVALID_PARAM; }
	if (buf == NULL) { return ERR_INVALID_PARAM; }

	acquire_semaphore(d->dev->mutex);

	if ((lba + amount) > d->dev->sector_count) {
		return ERR_INVALID_PARAM;
	}

	if (amount == 0) {
		return GENERIC_SUCCESS;
	}

	size_t stat = GENERIC_SUCCESS;

	if (d->dev->type == DISK_TYPE_IDE) {
		struct ide_drive *ide_d = d->dev->sp;
		if (amount < 0xFFFF) {
			stat = ide_pio_write48(ide_d->drive_id, lba + d->starting_sector, amount, buf);
		} else {
			/* TODO: handle this. */
		}
	} else {
		stat = ERR_DISK;
	}

	release_semaphore(d->dev->mutex);
	return stat;
};


