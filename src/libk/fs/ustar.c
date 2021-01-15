
#include <fs/ustar.h>
#include <mem.h>

USTAR_FILE_t* ustar_file_lookup(file_system_t* fs, char* file_name) {
	//fs is the file system to look in.
	//file_name is the file name.
	//the return value is a USTAR_FILE structure with the necessary data.
	//NULL is returned when the file cannot be found.
	
	//this buffer is used to hold the data read from the disk. it doesn't serve a purpose
	//other than that.
	uint8_t* buf = kmalloc(512);
	
	//this variable holds which sector of the fs we need to read. it will be incremented
	//as necessary in the loop.
	uint32_t sector = 0;	
	
	
	
	//load the first sector of the file system into memory.
	if (fs_read_sectors(fs, 0, 1, buf)) {
		//an error occured.
		kfree(buf);
		return NULL;
	}
	
	//this loop iterates through the entire archive.
	while (!memcmp(buf + 257, "ustar", 5)) {
		uint32_t file_size = oct2bin((char*)buf + 124, 11);
		
		//check if this is the file we're looking for
		if (!memcmp(buf, file_name, strlen(file_name) + 1)) {
			//we found it, now we can return.
			
			//this is the data we're going to return.
			USTAR_FILE_t* file = kmalloc(sizeof(USTAR_FILE_t));
			
			memcpy(file->file_name, buf, 100);
			memcpy(file->owner_user_name, buf + 265, 32);
			memcpy(file->owner_group_name, buf + 297, 32);
			
			file->type = buf[156];	//the type flag.
			file->starting_lba = sector;
			file->size = file_size;
			
			return file;
		}
		
		
		//this is not it either, skip.
		
		sector += (((file_size + 511) / 512) + 1);
		if (fs_read_sectors(fs, sector, 1, buf)) {
			//an error occured.
			
			break;
		}
		
		
		
	}
	
	kfree(buf);
	return NULL;
};



uint8_t ustar_load_file(file_system_t* fs, USTAR_FILE_t* file, void* buf) {
	//fs is the file system to be read from.
	//file_name is the file name.
	//buf is the buffer to be written to. The file will be loaded there.
	//return value indicates whether the function encountered an error or not.
	//return value of 0 = success, while any other value means an error.
	
	if (!file) {
		return 1;
	}
	
	
	
	uint32_t starting_lba = file->starting_lba;
	starting_lba++;	//increment it, because we don't need the "meta-data sector".
	
	
	
	uint8_t* dest 	= (uint8_t*)buf;
	
	if ((file->size / 512) == 0) {
		//then the file doesn't occupy a full sector.
		//so we do the "remainder" part early.
		
		//we need a temporary buffer. we will first read the sector here, and then copy the
		//necessary parts to dest. This is necessary as reading the entire sector
		//directly to dest can cause data corruption.
		uint8_t* temp = kmalloc(512);
		
		if (fs_read_sectors(fs, starting_lba, 1, (uint16_t*)temp)) {
			//error.
			kfree(temp);
			return 1;
		} 
		
		//now a memcpy call should do it.
		memcpy(dest, temp, file->size % 512);
		kfree(temp);
		return 0;
	}
	
	
	
	
	//this loop transfers the whole sectors within the file. the remainder will be done
	//after this.
	
	if (fs_read_sectors(fs, starting_lba, file->size/512, (uint16_t*)(dest))) {
		return 1;	//error while reading.
	}
	
	
	
	if (file->size % 512 == 0) {
		//if there is no "remainder", don't even bother continuing after here.
		//I'm doing this because reading from a disk is pretty damn slow.
		return 0;
	}
	
	
	//increment it, because we need to read the last sector now.
	starting_lba += file->size / 512;
	
	
	//we need a temporary buffer. we will first read the sector here, and then copy the
	//necessary parts to dest. This is necessary as reading the entire sector
	//directly to dest can cause data corruption.
	uint8_t* temp = kmalloc(512);
		
	if (fs_read_sectors(fs, starting_lba, 1, (uint16_t*)temp)) {
		//error.
		kfree(temp);
		return 1;
	} 
	
	//now a memcpy call should do it.
	memcpy(dest + (file->size / 512), temp, file->size % 512);
	kfree(temp);
	return 0;
	
};

uint8_t ustar_read_file(file_system_t* fs, USTAR_FILE_t* file, void* buf, uint32_t offset, uint32_t bytes) {
	//fs is the file system to be read from.
	//file_name is self-explanatory.
	//buf is the buffer to be read into. It is assumed to have enough space.
	//offset is the offset to be read from, in bytes.
	//bytes is the amount of bytes to be read.
	
	
	
	if (!file) {
		return 1;
	}
	
	
	uint32_t starting_sector = file->starting_lba;
	
	
	//skip the metadata.
	starting_sector++;
	
	//now we need to figure out which sector we're gonna start reading from.
	starting_sector += offset / 512;
	
	
	
	//now to read and store the proper data.
	if (fs_read_bytes(fs, buf, starting_sector, offset % 512, bytes)) {
		return 1;	//an error.
	}
	
	return 0;
};


uint8_t ustar_write_file(file_system_t* fs, USTAR_FILE_t* file, void* buf, uint32_t offset, uint32_t bytes) {
	//fs is the file system to be read from.
	//file_name is self-explanatory.
	//buf is the buffer to be read into. It is assumed to have enough space.
	//offset is the offset to be read from, in bytes.
	//bytes is the amount of bytes to be written.
	
	
	if (file == NULL) {
		return 1;
	}
	if (offset + bytes > file->size) {
		return 1;
	}
	
	
	uint32_t starting_sector = file->starting_lba;
	
	
	//skip the metadata.
	starting_sector++;
	
	//now we need to figure out which sector we're gonna start reading from.
	starting_sector += offset / 512;
	
	
	
	//now to read and store the proper data.
	if (fs_write_bytes(fs, buf, starting_sector, offset % 512, bytes)) {
		return 1;	//an error.
	}
	
	return 0;
};


uint8_t ustar_enlarge_file(file_system_t* fs, USTAR_FILE_t* file, uint32_t bytes) {
	//fs is the file system file is in.
	//file is the file that will be enlarged.
	//bytes is the amount of bytes the file will be enlarged by.
	
	/* This function's logic goes like this:
	 * Since the TAR format doesn't support fragmentation, that means (in order to enlarge the
	 * data section of a file) either the file is re-located to the end of the archive 
	 * (which doesn't work if the file isn't in the root directory), or, everything in the
	 * archive that comes after the file has to be moved forward by the amount of bytes we want the
	 * file to be enlarged by. This function does the latter, as the former isn't even a proper
	 * option. That means this function needs to move everything that comes (physically)
	 * after the file forward by some amount. This is done like this : First we find the last
	 * sector of the archive, then move everything by some amount of sectors, looping backwards
	 * through the archive. This ensures no data loss occurs, and continues until it reaches
	 * the file to be enlarged. Then, the files metadata is edited accordingly.
	 */ 
	
	
	
	
	if ((bytes/512) == 0) {
		if (((file->size % 512) + bytes) < 512) {
			//in this case, since enlarging the file won't cross a sector-boundary,
			//we don't actually need to move anything at all.
			//so we simply update the metadata, and return a success.
			char zeroes[bytes];
			
			memset(zeroes, 0, bytes);
			//zero out the newly allocated place.
			/*if (fs_write_bytes(fs, zeroes, file->starting_lba + (file->size/512) + 1, (file->size % 512), bytes)) {
				return 1;
			}*/
			
			file->size += bytes;
			//now change the info on the actual disk.
			char fsize[12] = {};
			memset(fsize, 0, 12);
			//TODO, add some code to actually convert from binary to octal here.
			bin2oct(file->size, fsize, 11);
			
			
			
			//for some reason fs_write_bytes didn't work, so instead let's read the
			//meta-data sector, change it then write it back.
			uint8_t* buf = kmalloc(512);
			if (fs_read_sectors(fs, file->starting_lba, 1, buf)) {
				kfree(buf);
				return 1;
			}
			
			memcpy(buf + 124, fsize, 11);
			
			
			if (fs_write_sectors(fs, file->starting_lba, 1, buf)) {
				kfree(buf);
				return 1;
			}
			
			kfree(buf);
			
			//now we should be done.
			return 0;
		}
	}
	
	//amount of sectors to enlarge the file by.
	uint32_t amount_sectors = (bytes + 511)/512;
	
	//buffer to hold the sectors we read from disk.
	uint8_t* buf = kmalloc(512);
	
	
	//the current sector to be moved. Used inside the loop.
	uint32_t sector = 0;
	if (fs_read_sectors(fs, sector, 1, buf)) {
		kfree(buf);
		return 1;
	}
	//this loop determines the last sector of the archive.
	while (!memcmp(buf + 257, "ustar", 5)) {
		uint32_t fsize = oct2bin((char*)buf + 124, 11);
		
		sector += (((fsize + 511) / 512) + 1);
		if (fs_read_sectors(fs, sector, 1, buf)) {
			kfree(buf);
			return 1;
		}
	}
	
	
	
	//because of the way the loop works, sector currently points to what is *after* the
	//data section of the last file of the archive, not the last sector. By exactly one (1) sector.
	sector--;	
	
	//now we now the LBA of the last sector of the entire archive. now we determine the 
	//file's last sector, then loop and move everything forwards until we reach that sector.
	uint32_t file_last = file->starting_lba + ((file->size + 511)/512);
	
	
	//This loops until it reaches the file's last sector.
	//at every iteration, it moves the sector it's currently on
	while (sector > file_last) {
		if (fs_read_sectors(fs, sector, 1, buf)) {
			kfree(buf);
			return 1;
		}
		
		if (fs_write_sectors(fs, sector + amount_sectors, 1, buf)) {
			kfree(buf);
			return 1;
		}
		
		sector--;
	}
	
	
	//if everything worked correctly, we should also update some stuff on the disk as well as the
	//meta data here.
	/* 
	 * TODO: Tuck some of this stuff into some other "helper" functions. Also refactor
	 * A big chunk of the FS drivers in general. God, everything here is so damn shitty.
	 * sigh...
	 */
	file->size += bytes;
	
	char fsize[11] = {};
	memset(fsize, 0, 11);
	bin2oct(file->size, fsize, 11);
	
	
	if (fs_read_sectors(fs, file->starting_lba, 1, buf)) {
		kfree(buf);
		return 1;
	}
	
	memcpy(buf + 124, fsize, 11);
	
	
	if (fs_write_sectors(fs, file->starting_lba, 1, buf)) {
		kfree(buf);
		return 1;
	}
	
	kfree(buf);
			
			
			
	kfree(buf);
	return 0;
};



