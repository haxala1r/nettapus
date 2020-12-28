

#include <fs/fs.h>
#include <disk/ide.h>
#include <mem.h>

file_system_t* root_fs;	//the first filesystem in the list will be our root.

file_system_t* fs_get_root() {
	return root_fs;
};



void register_fs(uint16_t drive_id, uint32_t starting_sector, uint32_t sector_count) {
	file_system_t* nfs = kmalloc(sizeof(file_system_t));	//the partition is used.
	nfs->drive_id = drive_id;
	
	nfs->starting_sector = starting_sector;
	nfs->sector_count = sector_count;
	
	//we need to read the first sector of the file system to determine what it is.
	uint8_t* first_sector = kmalloc(512);
	if (ide_pio_read28(drive_id, nfs->starting_sector, 1, (uint16_t*)first_sector)) {
		//if we can't read the first sector of the filesystem/partition, then
		//there isn't much we can do.
		
		kfree(first_sector);
		kfree(nfs);
		return;
	}
	
	//we could successfully load the first sector of the partition.
	//now we determine the filesystem that is being used.
	if (memcmp(first_sector + 257, "ustar", 5)) {
		//it's a USTAR filesystem.
		nfs->fs_type = FS_USTAR;
	} else {
		nfs->fs_type = FS_UNKNOWN;	//no other filesystems are implemented yet.
	}
	
	//now we know everything about this FS.
	
	
	//we no longer need this data.
	kfree(first_sector);
	
	
	
	//now we can add this to the list.
	if (root_fs == NULL) {
		root_fs = nfs;
		return;
	} 
	if (nfs->fs_type == FS_USTAR) {
		//USTAR gets priority. This is temporary, and only because USTAR is the only file system
		//I implemented. This will be replaced with a logic to find our root via the FAT's labels.
		nfs->next = root_fs;
		root_fs = nfs;
		return;
	}
	
	
	
	
	//find the last entry, in order to add our new fs to the end.
	file_system_t* f = root_fs;
	while (f->next != NULL) {
		f = f->next;
	}
	
	f->next = nfs;
	return;
};



uint8_t fs_init() {
	//scans all available disks and finds all available filesystems.
	//sysroot is always the first FS available.
	
	ide_drive_t* i = ide_get_first_drive();
	uint8_t* mbr = kmalloc(512);

	while (i != NULL) {
		
		//if we can't read from the disk, skip it.
		if (ide_pio_read28(i->drive_id, 0, 1, (uint16_t*)mbr)) {
			i = i->next;
			continue;
		}
		
		//if we could read the first sector, now we can start gathering info about the
		//partitions/filesystems.
		for (uint8_t j = 0; j < 4; j++) {
			uint32_t* lmbr = (uint32_t*)(mbr + 0x1BE + j * 0x10); 
			if (mbr[0x1BE + j * 0x10 + 4]) {
				//this checks the partition type field of a partition table entry.
				//if it's non-zero, the partition is used.
				register_fs(i->drive_id, lmbr[2], lmbr[3]);
				
				
				
			}
		}
		
		
		i = i->next;
	}
	kfree(mbr);
	if (root_fs) {
		return 0;
	}
	return 1;
}

