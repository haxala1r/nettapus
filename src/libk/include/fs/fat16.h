#ifndef FS_FAT16_H

#ifdef __cplusplus
extern "C" {
#endif


#include <stddef.h>
#include <stdint.h>
#include <fs/fs.h>


struct fat16_bpb_s {
	char boot_jmp[3];
	char oem_identifier[8];

	uint16_t bytes_per_sector;

	uint8_t sectors_per_cluster;
	uint16_t reserved_sectors;
	uint8_t num_fats;
	uint16_t num_dir_entries;
	uint16_t total_sectors;
	uint8_t media_desc_type;
	uint16_t sectors_per_fat;
	uint16_t sectors_per_track;
	uint16_t num_heads;
	uint32_t num_hidden_sectors;
	uint32_t sector_count_long;
	uint8_t drive_num;
	uint8_t flags;
	uint8_t signature;
	uint32_t serial;
	char label_string[11];
	char sys_id_string[8];
	char boot_code[448];
	uint16_t bootable_signature;

} __attribute__((packed));


struct fat16_dir_entry_s {
	char file_name[8];
	char file_extension[3];
	uint8_t attributes;
	uint8_t _reserved;

	uint8_t creation_tenth_sec;
	uint16_t creation_time;
	uint16_t creation_date;

	uint16_t last_accessed;

	uint16_t first_cluster_high;

	uint16_t last_mod_time;
	uint16_t last_mod_date;

	uint16_t first_cluster_low;

	uint32_t file_size;

} __attribute__((packed));


struct fat16_file_s {
	struct fat16_dir_entry_s *entry;

	/* This number is the cluster number that keeps this file's directory entry. If zero,
	 * the entry is assumed to be in the root directory. */
	uint32_t entry_cluster;
};




typedef 	struct fat16_file_s 			FAT16_FILE		;
typedef 	struct fat16_bpb_s 			FAT16_BPB		;
typedef 	struct fat16_dir_entry_s 	FAT16_DIR_ENTRY	;


char **fat16_parse_path(char *, uint32_t *);

uint16_t fat16_get_FAT_entry(FILE_SYSTEM *, uint32_t);
uint8_t fat16_load_bpb(FILE_SYSTEM*);
FAT16_FILE *fat16_file_lookup(FILE_SYSTEM *, char *);
uint8_t fat16_name_compare(char *, char *, char *);
uint8_t fat16_read_file(FILE_SYSTEM *, FAT16_FILE *, void *, uint32_t, uint32_t);
uint8_t fat16_write_file(FILE_SYSTEM *, FAT16_FILE *, void *, uint32_t, uint32_t);

#ifdef __cplusplus
}
#endif

#endif


