

#include <fs/fat16.h>
#include <fs/fs.h>
#include <mem.h>


uint8_t fat16_name_compare(char *fname, char *fat_name, char *fat_extension) {
	/*
	 * This function exists because in FAT16, file names are kept in all capitals, padded
	 * with spaces etc. etc.
	 * And thus, a function to compare a regular file name with a FAT one is necessary.
	 */

	if (strlen(fname) > 12) {
		return 1;	/* They *can't* be the same. */
	}

	/* Create our own copy of the string. */
	char *file_name = kmalloc(strlen(fname) + 1);
	memcpy(file_name, fname, strlen(fname) + 1);


	/* We need to convert file_name to all capital letters. */
	unsigned char *i = (unsigned char*)file_name;
	while (*i) {
		/* If *i is a lowercase letter, turn it into an upper case one.*/
		if ((*i < 0x7B) && (*i > 0x60)) {
			*i -= 0x20;
		}

		i++;
	};

	/* Now determine where the file name ends and the extension begins (i. e. the dot.)*/

	char *name = file_name;
	uint16_t name_len = 0;
	char *extension = file_name;
	while (*extension) {
		if (*extension == '.') {
			extension++;
			break;
		}

		extension++;
		name_len++;
	}

	if (name_len > 8) {
		kfree(file_name);
		return 1;
	}
	if (strlen(extension) > 3) {
		kfree(file_name);
		return 1;
	}

	if (memcmp(name, fat_name, name_len)) {
		kfree(file_name);
		return 1;
	}
	if (memcmp(extension, fat_extension, strlen(extension))) {
		kfree(file_name);
		return 1;
	}

	kfree(file_name);
	return 0;
};


uint16_t fat16_get_FAT_entry(struct file_system *fs, uint32_t entry) {
	/* Gets an entry from the File Allocation Table. */

	FAT16_BPB *bpb = fs->special;

	uint16_t *FAT = kmalloc(512);
	uint32_t fat_sector = bpb->reserved_sectors + (entry / 256);
	entry %= 256;

	if (fs_read_sectors(fs, fat_sector, 1, FAT)) {
		return 0;
	}

	uint16_t val = FAT[entry];

	kfree(FAT);

	return val;
};



uint8_t fat16_load_bpb(struct file_system *fs) {
	/* Loads the Bios Parameter Block and the Extended Boot Record for the file system. */

	uint8_t *buf = kmalloc(512);

	if (fs_read_sectors(fs, 0, 1, buf)) {
		return 1;
	}

	FAT16_BPB *bpb = (FAT16_BPB*)buf;

	if ((bpb->signature != 0x28) && (bpb->signature != 0x29)) {
		kfree(bpb);
		return 2;
	}

	fs->special = bpb;
	return 0;
};


char **fat16_parse_path(char *path, uint32_t *depth) {
	/* After the path is parsed, the depth of the path will be stored inside depth. */

	char *i;	/* Our main iterator. */
	char *j;	/* Second iterator. Used only in the second loop.*/
	*depth = 0;

	/* This loop determines the depth. We need to determine depth before actually starting to
	 * allocate strings, because we need to know how much space to allocate.
	 */

	i = path;
	if (*i == '/') {
		i++;	/* Don't count the first slash. */
	}

	while (*i) {
		if (*i == '/') {
			*depth += 1;
		}
		i++;
	}

	/* If there are no slashes, there is nothing to parse. */
	if (*depth == 0) {
		return NULL;
	}

	/* Now we know the depth, and can allocate space for the array that will hold the strings. */
	char **arr = kmalloc((*depth + 1) * sizeof(char*));
	memset(arr, 0, (*depth + 1) * sizeof(char*));
	uint32_t arr_index = 0;		/* Which index will be done on the array next? */

	i = path;
	if (*i == '/') {
		i++;
	}
	j = i;


	/* Now the loop where we actually parse the path.
	 * This loop works like this:
	 * 	i is never directly incremented, while j is incremented on every iteration. The idea
	 * 	is that j is always ahead of i, and whenever j encounters a '/' that means whatever
	 * 	comes between i and j is another directory. So we allocate space for a string,
	 * 	and copy everything betweeen i and j there. Increment j, and set i to j. This way,
	 * 	we get a clear array of things between the slashes, a.k.a. directories.
	 * */

	while (1) {
		if (*j == '\0') {
			/* We're at the end of the string. */
			uint64_t len = ((uint64_t)j - (uint64_t)i);
			if (len == 0) {
				break;
			}

			arr[arr_index] = kmalloc(len + 1);

			memset(arr[arr_index], 0, len + 1);
			memcpy(arr[arr_index], i, len);
			arr_index++;
			break;
		}

		if (*j == '/') {
			uint64_t len = ((uint64_t)j - (uint64_t)i);
			if (len == 0) {
				j++;
				i = j;
				arr_index++;
				continue;
			}

			arr[arr_index] = kmalloc(len + 1);

			memset(arr[arr_index], 0, len + 1);

			memcpy(arr[arr_index], i, len);

			j++;
			i = j;
			arr_index++;
			continue;
		}
		j++;
	}

	/* We should be done, if we reached here. */
	return arr;
};



FAT16_DIR_ENTRY *fat16_search_directory(struct file_system *fs, char *file_name, uint32_t cluster) {
	/* Looks through a directory for a file. */

	FAT16_BPB *bpb = fs->special;

	if (bpb == NULL) {
		return NULL;
	}
	if (cluster < 2) {
		return NULL;
	}
	if (file_name == NULL) {
		return NULL;
	}

	uint64_t root_dir_sectors = ((bpb->num_dir_entries * 32) + (bpb->bytes_per_sector - 1)) / bpb->bytes_per_sector;
	uint64_t first_data = bpb->reserved_sectors + (bpb->num_fats * bpb->sectors_per_fat) + root_dir_sectors;

	/* We're going to load entire clusters. */
	FAT16_DIR_ENTRY *buf = kmalloc(bpb->sectors_per_cluster * bpb->bytes_per_sector);
	if (buf == NULL) {
		return NULL;
	}

	/* Loop over each cluster in the directory. */
	while (1) {
		uint32_t sector = ((cluster - 2) * bpb->sectors_per_cluster) + first_data;

		if (fs_read_sectors(fs, sector, bpb->sectors_per_cluster, buf)) {
			kfree(buf);
			return NULL;
		}

		/* Loop over each entry in the cluster. */
		FAT16_DIR_ENTRY *i = buf;
		while (i) {

			/* TODO: Add logic to break here after reaching the end of a cluster. */

			if (i->file_name[0] == '\0') {
				break;
			}

			if (!fat16_name_compare(file_name, i->file_name, i->file_extension)) {
				FAT16_DIR_ENTRY *ret = kmalloc(sizeof(FAT16_DIR_ENTRY));

				if (ret == NULL) {
					kfree(buf);
					return NULL;
				}

				memcpy(ret, i, sizeof(FAT16_DIR_ENTRY));
				kfree(buf);

				return ret;
			}



			i++;
		};


		if (i->file_name[0] == '\0') {
			break;	/* Break from the outer loop as well. */
		}

		cluster = fat16_get_FAT_entry(fs, cluster);
		if (cluster >= 0xFFF7) {
			break;
		}
	}

	/* If we're here, then we couldn't find the requested file. */
	kfree(buf);

	return NULL;
};






void *fat16_file_lookup(struct file_system *fs, char *full_path) {
	/* Finds a file on the FAT16 file system, and loads some information about it. */
	// TODO: Complete this proper.

	FAT16_BPB *bpb = fs->special;
	if (bpb == NULL) {
		return NULL;
	}
	if (full_path == NULL) {
		return NULL;
	}

	/* First, let's parse the file path, so that we know where the file will be. */
	uint32_t depth = 0;
	char **file_path = fat16_parse_path(full_path, &depth);

	char *first_dir_name;	/* This is what we're going to search for in the root directory.*/

	if (file_path == NULL) {
		first_dir_name = full_path;
	} else {
		first_dir_name = file_path[0];
	}



	/* Now we need to figure out some info on the root directory, so that we can search it.*/
	uint64_t root_dir_sectors = ((bpb->num_dir_entries * 32) + (bpb->bytes_per_sector - 1)) / bpb->bytes_per_sector;

	/* On FAT16, the root directory is always right after the FATs. */
	uint64_t root_dir = bpb->reserved_sectors + (bpb->num_fats * bpb->sectors_per_fat);


	/* Load the root directory into memory.*/
	uint8_t *buf = kmalloc(root_dir_sectors * bpb->bytes_per_sector);

	if (fs_read_sectors(fs, root_dir, root_dir_sectors, buf)) {
		return NULL;
	}



	/* Now search the root directory. */
	FAT16_DIR_ENTRY *entry = (FAT16_DIR_ENTRY*)buf;

	while (1) {

		if (entry->file_name[0] == '\0') {
			return NULL;	/* We reached the end, and could not find the file. */
		}

		if (!fat16_name_compare(first_dir_name, entry->file_name, entry->file_extension)) {
			break;
		}


		entry++;
	}

	FAT16_DIR_ENTRY *f_entry = kmalloc(sizeof(FAT16_DIR_ENTRY));

	if (f_entry == NULL) {
		return NULL;
	}
	memcpy(f_entry, entry, sizeof(FAT16_DIR_ENTRY));

	kfree(buf);



	if (file_path == NULL) {
		/* This means it is in the root directory, so *f now holds our desired file. */
		FAT16_FILE *f = kmalloc(sizeof(FAT16_FILE));
		f->entry = f_entry;
		f->entry_cluster = 0;

		return f;
	}

	kfree(file_path[0]);

	/* If the above case returned false, then *entry now holds the first directory in
	 * the hierarchy. Now we need a loop to search every directory and find the file.
	 */
	uint32_t cluster = 0;
	for (size_t i = 1; i <= depth ; i++) {
		cluster = f_entry->first_cluster_low;
		kfree(f_entry);

		f_entry = fat16_search_directory(fs, file_path[i], cluster);
		kfree(file_path[i]);


		if (f_entry == NULL) {
			return NULL;
		}
	}

	/* *f should now hold the entry for the desired file. */
	kfree(file_path);

	FAT16_FILE *f = kmalloc(sizeof(FAT16_FILE));
	f->entry = f_entry;
	f->entry_cluster = cluster;


	return f;
};




uint8_t fat16_read_file(struct file_system *fs, void *file_void, void *buf, size_t offset, size_t bytes) {
	/* Reads some amount of bytes at some offset from some file. */

	FAT16_FILE *f = file_void;
	offset = (size_t)((uint32_t)offset);
	bytes = (size_t)((uint32_t)bytes);
	if (fs == NULL) {
		return 1;
	}
	if (f == NULL) {
		return 1;
	}
	if (buf == NULL) {
		return 1;
	}
	if (bytes == 0) {
		return 0;	/* Nothing to do.*/
	}

	FAT16_DIR_ENTRY *entry = f->entry;	/* File's directory entry. */

	if (entry == NULL) {
		return 1;
	}

	if ((offset + bytes) > entry->file_size) {
		return 1;
	}


	FAT16_BPB *bpb = fs->special;


	if (bpb == NULL) {
		return 1;
	}

	uint64_t root_dir_sectors = ((bpb->num_dir_entries * 32) + (bpb->bytes_per_sector - 1)) / bpb->bytes_per_sector;
	uint64_t first_data = bpb->reserved_sectors + (bpb->num_fats * bpb->sectors_per_fat) + root_dir_sectors;


	/* Size of each cluster. */
	uint32_t cluster_size = bpb->sectors_per_cluster * bpb->bytes_per_sector;


	/* This is the actual cluster number.*/
	uint32_t cluster = entry->first_cluster_low;

	while (offset > cluster_size) {
		cluster = fat16_get_FAT_entry(fs, cluster);
		if (cluster >= 0xFFF7) {
			return 1;
		}

		offset -= cluster_size;
	}


	/* Get our buffer ready. */
	uint32_t sector_count = (bytes + bpb->bytes_per_sector - 1) / bpb->bytes_per_sector;
	uint8_t *temp_buf = kmalloc(sector_count * bpb->bytes_per_sector);
	uint16_t offset_b = offset % bpb->bytes_per_sector;

	/* This is our iterator, this will keep track of how many more bytes we need to read. */
	int64_t i = bytes;

	/* This is out iterator-buffer, this will keep track of where on the temporary bufffer to
	 * read to.
	 */
	uint8_t *i_buf = temp_buf;

	/* This loop goes over each cluster in the file that is requested. */
	while (1) {
		/* Now determine which sector in the cluster to read. */
		uint32_t sector = ((cluster - 2) * bpb->sectors_per_cluster) + first_data;
		sector += offset / bpb->bytes_per_sector;


		/* Determine the amount of sectors we need to read. */
		sector_count = (i + bpb->bytes_per_sector - 1) / bpb->bytes_per_sector;
		if (sector_count > bpb->sectors_per_cluster) {
			sector_count = bpb->sectors_per_cluster;
		}



		/* Perform disk access. */
		if (fs_read_sectors(fs, sector, sector_count, i_buf)) {
			kfree(temp_buf);
			return 1;
		}


		/* Increment our iterators. */
		i -= sector_count * bpb->bytes_per_sector;

		/* Because our calculation of sector_count above rounds up, if i isn't perfectly
		 * divisable by 512, i will go negative. If we use an unsigned integer for i, this
		 * causes an integer underflow, meaning the loop will go on forever, and potentially
		 * fail.
		 *
		 * That is why we use int64_t for i, instead of uint32_t. It is also the reason
		 * why we have to check this with a "<=" instead of "==".
		 */
		if (i <= 0) {
			break;
		}

		/* We already added offset once, no need to do it anymore.*/
		offset = 0;

		/* Each iteration goes over one cluster. */
		cluster = fat16_get_FAT_entry(fs, cluster);
		i_buf += sector_count * bpb->bytes_per_sector;
	}

	/* Copy data to *buf. */
	memcpy(buf, temp_buf + offset_b, bytes);

	kfree(temp_buf);

	return 0;
};


uint8_t fat16_write_file(struct file_system *fs, void *file_void, void* buf, size_t offset, size_t bytes) {
	/* Reads some bytes at some offset from some file. */

	FAT16_FILE *f = file_void;

	offset = (size_t)((uint32_t)offset);
	bytes = (size_t)((uint32_t)bytes);
	/* Some checks first. */
	if (fs == NULL)                             { return 1; }
	if (f == NULL)                              { return 1; }
	if (buf == NULL)                            { return 1; }
	if (bytes == 0)                             { return 0; }
	if (f->entry == NULL)                     { return 1; }
	if ((offset + bytes) > f->entry->file_size) return 1;
	if (fs->special == NULL)                    { return 1; }



	FAT16_BPB *bpb = fs->special;
	FAT16_DIR_ENTRY *entry = f->entry;


	uint64_t root_dir_sectors = ((bpb->num_dir_entries * 32) + (bpb->bytes_per_sector - 1)) / bpb->bytes_per_sector;
	uint64_t first_data = bpb->reserved_sectors + (bpb->num_fats * bpb->sectors_per_fat) + root_dir_sectors;


	/* Size of each cluster. */
	uint32_t cluster_size = bpb->sectors_per_cluster * bpb->bytes_per_sector;


	/* This is the actual cluster number.*/
	uint32_t cluster = entry->first_cluster_low;

	/* We should find out at which cluster offset starts. */
	while (offset > cluster_size) {
		cluster = fat16_get_FAT_entry(fs, cluster);
		if (cluster >= 0xFFF7) {
			return 1;
		}

		offset -= cluster_size;
	}


	/* We need to actually construct the buffer we will write to disk.
	 * This is because data on disk will be lost if offset and bytes aren't sector aligned.
	 */
	uint32_t sector_count = (bytes + bpb->bytes_per_sector - 1) / bpb->bytes_per_sector;
	uint8_t *temp_buf = kmalloc(sector_count * bpb->bytes_per_sector);
	uint16_t offset_b = offset % bpb->bytes_per_sector;

	if (offset_b != 0) {
		/* We need to copy the data on disk to temp_buf before we can progress.
		 * Otherwise, data on disk will be lost.
		 */
		uint8_t *data = kmalloc(bpb->bytes_per_sector);

		if (fs_read_sectors(fs, (cluster - 2) * bpb->sectors_per_cluster + first_data, 1, data)) {
			kfree(temp_buf);
			kfree(data);
			return 1;
		}

		memcpy(temp_buf, data, offset_b);

		kfree(data);
	}

	if (((offset + bytes) % bpb->bytes_per_sector) != 0) {
		/* We need to do the same thing above, but at the end instead. */

		uint8_t *data = kmalloc(bpb->bytes_per_sector);

		/* Because we need to copy the data at the very end, we need to figure out the position
		 * of that very end. So, here are a couple variables to help with that.
		 */

		uint32_t data_cluster = cluster;

		/* Where, inside the sector the data is. (in bytes)*/
		uint32_t data_offset = ((offset + bytes) % bpb->bytes_per_sector);

		/* Where, inside the cluster the data is. (in sectors)*/
		uint32_t cluster_offset = ((offset + bytes) % cluster_size) / bpb->bytes_per_sector;


		/* Find the right cluster. */
		for (size_t i = 0; i < ((offset + bytes)/cluster_size); i++) {
			data_cluster = fat16_get_FAT_entry(fs, data_cluster);
		}


		/* Perform disk access. */
		if (fs_read_sectors(fs, (data_cluster - 2) * bpb->sectors_per_cluster + first_data + cluster_offset, 1, data)) {
			kfree(temp_buf);
			kfree(data);
			return 1;
		}


		/* Now copy the data to the end of temp_buf. This guarantees there won't be data loss.*/
		memcpy(temp_buf + offset_b + bytes, data + data_offset, bpb->bytes_per_sector - data_offset);
		kfree(data);
	}


	memcpy(temp_buf + offset_b, buf, bytes);

	/* Now our buffer is complete, and we can write it to disk without worry. */



	/* This is our iterator, it will keep track of how much more we need to write. */
	int64_t i = bytes;

	/* This is our iterator-buffer, it helps us keep track of where we should write from on
	 * *buf.*/
	uint8_t *i_buf = temp_buf;

	while (1) {
		/* Determine the sector of our cluster. */
		uint32_t sector = ((cluster - 2) * bpb->sectors_per_cluster) + first_data;
		sector += offset / bpb->bytes_per_sector;

		/* Determine the amount of sectors we need to write. */
		sector_count = (i + bpb->bytes_per_sector - 1) / bpb->bytes_per_sector;
		if (sector_count > bpb->sectors_per_cluster) {
			sector_count = bpb->sectors_per_cluster;
		}

		/* Perform disk access. */
		if (fs_write_sectors(fs, sector, sector_count, i_buf)) {
			kfree(temp_buf);
			return 1;
		}



		/* Now increment/decrement our iterators.*/
		i -= sector_count * bpb->bytes_per_sector;

		if (i <= 0) {
			break;
		}

		offset = 0;


		/* Each iteration goes over one cluster. */
		cluster = fat16_get_FAT_entry(fs, cluster);
		i_buf += sector_count * bpb->bytes_per_sector;
	}

	/* If we reach here, everything should be done. */

	kfree(temp_buf);
	return 0;
};
