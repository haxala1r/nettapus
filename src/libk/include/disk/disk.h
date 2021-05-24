#ifndef DISK_H
#define DISK_H 1


#define DISK_TYPE_IDE         0

#define DRIVE_TYPE_RAW      0
#define DRIVE_TYPE_MBR      1
#define DRIVE_TYPE_GPT      2
#define DRIVE_TYPE_FS       3


#include <stddef.h>
#include <stdint.h>


struct semaphore; /* Full definition in <task.h>*/

/* Disks are physical storage mediums. Drives are a range of sectors within a disk.
 * They can be partitions, filesystems etc. etc. or they can be the whole disk as
 * well.
 */
struct disk {
	uint8_t type;
	size_t sector_count;

	/* Special info specific to the type of disk. */
	void *sp;

	/* A semaphore for the whole disk. Two processes cannot access the same disk
	 * at the same time, even if they're different drives.
	 */
	struct semaphore *mutex;

	struct disk *next;
};


struct drive {
	struct disk *dev;	/* The device it resides on. */

	size_t starting_sector;
	size_t sector_count;

	/* Indicates what resides on this drive. Maybe a MBR, a GPT, an ext2 filesystem
	 * etc. etc.
	 */
	size_t type;

	struct drive *next;
};



size_t drive_write_sectors(struct drive *d, void *buf, size_t lba, size_t amount);
size_t drive_read_sectors(struct drive *d, void *buf, size_t lba, size_t amount);

size_t drive_parse_mbr(struct drive *d);
size_t check_drive(struct drive *d);

size_t register_drive(struct drive *d);

struct drive *get_drive_list();

uint8_t refresh_disks();
uint8_t init_disk();



#endif /* DISK_H */
