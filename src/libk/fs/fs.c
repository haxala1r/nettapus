

#include <fs/fs.h>
#include <disk/ide.h>
#include <mem.h>
#include <fs/ustar.h>
#include <fs/fat16.h>




file_system_t* root_fs;	//the first filesystem in the list will be our root.

file_system_t* fs_get_root() {
	return root_fs;
};


//These functions, in particular, are only made as a convenient "abstraction" to be able to
//read stuff off the disk easily, without swapping 15 variables and 33 functions around.

uint8_t fs_read_sectors(file_system_t* fs, uint64_t lba, uint32_t sector_count, void* buf) {
	if ((lba + sector_count) > (fs->sector_count)) {
		return 1;
	}

	/* Now we need to perform the disk access. 0x0FFFFFFF is the maximum LBA that can be
	 * accessed using 28 bits.
	 */
	if ((sector_count <= 256) && ((fs->starting_sector + lba + sector_count) < 0x0FFFFFFF)) {
		
		if (sector_count == 256) {
			sector_count = 0;
		}
		
		/* Attempt a regular 28-bit PIO read.*/
		if (ide_pio_read28(fs->drive_id, fs->starting_sector + lba, (uint8_t)sector_count, (uint16_t*)buf)) {
			return 1;		//an error occured.
		}
	
	} else if (sector_count <= 65536) {
		
		if (sector_count == 65536) {
			sector_count = 0;
		}
		
		/* Attempt a regular 48-bit PIO read.*/
		if (ide_pio_read48(fs->drive_id, fs->starting_sector + lba, (uint16_t)sector_count, (uint16_t*)buf)) {
			return 1;		//an error occured.
		}
		
	} else  {
		//we need to do it in multiple stages.
		
		size_t i = 255;
		uint8_t* i_buf = (uint8_t*)buf;	//iterator pointer to be able to read conveniently without losing our pointer.
		while (sector_count) {
			if (sector_count < 255) {
				i = sector_count;
			}
			
			if (ide_pio_read28(fs->drive_id, fs->starting_sector + lba, (uint8_t)i, (uint16_t*)i_buf)) {
				return 1;		//an error occured.
			}
			
			sector_count -= i;
			lba += i;
			i_buf += i * 512;
		}
	}
	
	return 0;
};


uint8_t fs_write_sectors(file_system_t* fs, uint64_t lba, uint32_t sector_count, void* buf) {
	if ((lba + sector_count) > (fs->sector_count)) {
		return 1;
	}

	/* Now we need to perform the disk access. 0x0FFFFFFF is the maximum LBA that can be
	 * accessed using 28 bits.
	 */
	if ((sector_count <= 256) && ((fs->starting_sector + lba + sector_count) < 0x0FFFFFFF)) {
		if (sector_count == 256) {
			sector_count = 0;
		}
		
		/* Attempt a regular 28-bit write. */
		if (ide_pio_write48(fs->drive_id, fs->starting_sector + lba, (uint8_t)sector_count, (uint16_t*)buf)) {
			return 1;		//an error occured.
		}
		
		
	} else if (sector_count <= 65536) {
		if (sector_count == 65536) {
			sector_count = 0;
		}
		
		/* Attempt a regular 48-bit PIO read.*/
		if (ide_pio_read48(fs->drive_id, fs->starting_sector + lba, (uint16_t)sector_count, (uint16_t*)buf)) {
			return 1;		//an error occured.
		}
		
	} else  {
		/* We need to do multiple accesses. */
		
		/* This is the maximum amount of sectors we can access at a time. */
		size_t i = 255;
		
		/* This is an iterator pointer to be able to read conveniently without losing our place. */
		uint8_t* i_buf = (uint8_t*)buf;	
		
		while (sector_count) {
			if (sector_count < 255) {
				i = sector_count;
			}
			
			if (ide_pio_write28(fs->drive_id, fs->starting_sector + lba, (uint8_t)i, (uint16_t*)i_buf)) {
				return 1;		
			}
			
			sector_count -= i;
			lba += i;			
			i_buf += i * 512;	/* The reason i_buf was declared. */
		}
	}
	

	return 0;
};


uint8_t fs_read_bytes(file_system_t* fs, void* buf, uint32_t sector, uint16_t offset, uint32_t bytes) {
	/* This function reads {bytes} to {buf} from {fs} at {sector} at offset. */
	
	/* Minimise unnecessary disk access. */
	sector += offset / 512;
	offset %= 512;
	
	/* How many sectors in total need to be read. The formula is a bit arcane, and I
	 * apologise for that. Unfortunately though, I can't think of a way to present this
	 * without boring everyone too much.
	 */
	uint32_t sector_count = (bytes / 512) + ((bytes % 512) + 511)/512 + 1;
	
	
	
	uint8_t* temp_buf = kmalloc(sector_count * 512);
	
	if (fs_read_sectors(fs, sector, sector_count, temp_buf)) {
		return 1;
	}
	
	
	//now we just need to copy the data to where it belongs, and it is done.
	memcpy((uint8_t*)buf, temp_buf + offset, bytes);
	
	
	kfree(temp_buf);
	return 0;
};


uint8_t fs_write_bytes(file_system_t* fs, void* buf, uint32_t sector, uint16_t offset, uint32_t bytes) {
	/* Reads bytes on a non-sector aligned address. */
	
	//making sure that the values are correctly written in their respective places. 
	sector += offset / 512;
	offset %= 512;
	
	//how many sectors in total we need to read from the disk.
	uint32_t sector_count = (bytes / 512) + ((bytes % 512) + 511)/512;
	sector_count += offset%512 ? 1 : 0;	//we need to read an extra sector if the offset isn't sector-aligned.
	
	
	//temporary buffer, because we need to read from the middle of the disk.
	uint8_t* temp_buf = kmalloc(sector_count * 512);
	if (temp_buf == NULL) {
		return 1;
	}
	
	//in order to write, we need to do the "copying" in advance, but in order to do that,
	//we need to actually read the first and the last sectors and write them manually,
	//doing them as part of the loop would be terrible.
	//We need to do this mainly because otherwise there could be significant data loss.
	//if offset != 0. because the first offset amount of bytes in temp_buf isn't defined,
	//the bytes on the disk would become corrupt. Thankfully, we only need to read the
	//first and the last sectors in advance for this to work with no data loss.
	//It took a lot of effort to make this part NOT shit itself when the first
	//and the last sectors are the same.
	if (offset != 0) {
		uint8_t* disk_data = kmalloc(512);
		if (fs_read_sectors(fs, sector, 1, disk_data)) {
			kfree(disk_data);
			kfree(temp_buf);
			return 1;
		}
		
		//copy the disk's data to where it is relevant.
		//this part copies the data that comes before the data to be written on the sector.
		memcpy(temp_buf, disk_data, offset);
		kfree(disk_data);
	}
	if (((offset + bytes) % 512) != 0) {
		uint8_t* disk_data = kmalloc(512);
		if (fs_read_sectors(fs, sector + sector_count - 1, 1, disk_data)) {
			kfree(disk_data);
			kfree(temp_buf);
			return 1;
		}
		
		//copy the disk's data to where it is relevant.
		//this part copies the data that comes after the data to be written on the sector.
		memcpy(temp_buf + (sector_count - 1) * 512 + (bytes % 512), disk_data + (bytes % 512), 512 - (bytes % 512));
		
		kfree(disk_data);
	}
	
	
	//fill the middle space with the data to be written.
	memcpy(temp_buf + offset, ((uint8_t*)buf), bytes);
	
	
	
	

	if (fs_write_sectors(fs, sector, sector_count, temp_buf)) {
		kfree(temp_buf);
		return 1;	//an error.
	}
	
	
	//we should be done if we reach here.
	kfree(temp_buf);
	return 0;
}









uint8_t register_fs(uint16_t drive_id, uint64_t starting_sector, uint64_t sector_count) {
	file_system_t* nfs = kmalloc(sizeof(file_system_t));	//the partition is used.
	nfs->drive_id = drive_id;
	
	nfs->starting_sector = starting_sector;
	nfs->sector_count = sector_count;
	nfs->next = NULL;
	
	
	/* Read the first sector of the partition/fs to figure out what it is. */
	uint8_t* first_sector = kmalloc(512);
	memset(first_sector, 0, 512);
	if (ide_pio_read28(drive_id, nfs->starting_sector, 1, (uint16_t*)first_sector)) {
		kfree(first_sector);
		kfree(nfs);
		return 1;
	}
	
	
	/* This part determines the file system. */
	
	if (memcmp(first_sector + 257, "ustar", 5) == 0) {
		nfs->fs_type = FS_USTAR;
		nfs->special = NULL;	/* TAR doesn't need any fs-specific data. */
	} else if ((first_sector[38] == 0x28) || (first_sector[38] == 0x29))  {
		nfs->fs_type = FS_FAT16;
		if (fat16_load_bpb(nfs)) {
			return 1;
		}
	} else {
		nfs->fs_type = FS_UNKNOWN;	//no other filesystems are implemented yet.
	}
	
	//now we know everything about this FS.
	
	
	//we no longer need this data.
	kfree(first_sector);
	
	
	
	//now we can add this to the list.
	if (root_fs == NULL) {
		root_fs = nfs;
		
		return 0;
	} 
	if (nfs->fs_type == FS_FAT16) {
		//USTAR gets priority. This is temporary, and only because USTAR is the only file system
		//I implemented. This will be replaced with a logic to find our root via the FAT's labels.
		nfs->next = root_fs;
		root_fs = nfs;
		
		return 0;
	}
	
	
	
	
	//find the last entry, in order to add our new fs to the end.
	file_system_t* f = root_fs;
	while (f->next != NULL) {
		f = f->next;
	}
	
	f->next = nfs;
	
	return 0;
};




uint8_t fs_parse_mbr(uint16_t drive_id) {
	uint8_t* mbr = kmalloc(512);
	
	//if we can't read from the disk, skip it.
	if (ide_pio_read28(drive_id, 0, 1, (uint16_t*)mbr)) {
		kfree(mbr);
		return 1;
	}
	
	
	
	//if we could read the first sector, now we can start gathering info about the
	//partitions/filesystems.
	for (uint8_t j = 0; j < 4; j++) {
		uint32_t* lmbr = (uint32_t*)(mbr + 0x1BE + j * 0x10); 
		
		/* Check the Partition Type field. If non-zero, the partition is used. */
		if (mbr[0x1BE + j * 0x10 + 4] != 0) {
			
			if (register_fs(drive_id, lmbr[2], lmbr[3])) {
				return 1;	
			};
			
		}
	}
	
	kfree(mbr);
	return 0;
};

uint8_t fs_parse_gpt(uint16_t drive_id) {
	return 0;
};




uint8_t fs_init() {
	/* Goes over all available drives, and finds every filesystem it can recognize. */
	
	ide_drive_t* i = ide_get_first_drive();
	uint8_t* buf = kmalloc(512);	
	
	//the return code. after all, we want to be able to see all errors without stopping execution.
	//this will also be the amount of errors encountered.
	uint8_t ret_code = 0;	
	
	
	while (i != NULL) {
		
		/* If the first 8 bytes of LBA 1 read "EFI PART", then it's gpt. Otherwise, it must be
		 * DOS MBR. 
		 */
		if (ide_pio_read28(i->drive_id, 1, 1, (uint16_t*)buf)) {
			//if an error occured, simply skip the disk entirely.
			ret_code++;
			i = i->next;
			continue;
		}
		
		if (memcmp((uint8_t*)buf, "EFI PART", 8)) {
			//if the first 8 bytes aren't the magic values we're looking for, then it must
			//be mbr.
			if (fs_parse_mbr(i->drive_id)) {
				ret_code++;
			}
		} else {
			//the magic values match what is on the disk. It must be a gpt partitioned disk.
			if (fs_parse_gpt(i->drive_id)) {
				ret_code++;
			}
		}
		
		
		
		
		i = i->next;
	}
	
	
	kfree(buf);
	
	/* Check for more errors, and return. */
	if (root_fs == NULL) {
		return ++ret_code;
	}
	if (root_fs->fs_type == 0xFF) {
		return 0xFF;
	}
	return ret_code;
}






#ifdef DEBUG
#include <tty.h>

void fs_print_state() {
	kputs("\nFS STATES \n{fs_type}: {starting_lba} {sector count}\n");
	file_system_t *fs = root_fs;
	
	while (fs) {
		kputx(fs->fs_type);
		kputs(": ");
		
		kputx(fs->starting_sector);
		kputs(" ");
		
		kputx(fs->sector_count);
		kputs("\n");
		
		fs = fs->next;
	}
};


#endif




