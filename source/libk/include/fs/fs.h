#ifndef FS_H 
#define FS_H 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>


//filesystem types.
#define FS_USTAR 	0
//#define FS_FAT16 	1
//#define FS_FAT32 	2
#define FS_UNKNOWN 	0xFF




struct file_system {
	uint8_t fs_type;
	
	
	//info needed in order to access the system.
	uint16_t drive_id;
	
	uint32_t starting_sector;	//the LBA number the filesystem/partition starts at.
	uint32_t sector_count;		//The amount of sectors this filesystem contains.
	
	struct file_system* next;	//linked list.
};


typedef struct file_system file_system_t;


uint8_t fs_read_sectors(file_system_t*, uint32_t, uint8_t, void*);
uint8_t fs_write_sectors(file_system_t*, uint32_t, uint8_t, void*);

uint8_t fs_read_bytes(file_system_t*, void*, uint32_t, uint16_t, uint32_t);
uint8_t fs_write_bytes(file_system_t*, void*, uint32_t, uint16_t, uint32_t);




uint8_t fs_list_dirs(file_system_t, char*);

uint8_t fs_read_file(file_system_t, char*, void*, uint32_t, uint32_t);
uint8_t fs_write_file(file_system_t, char*, void*, uint32_t, uint32_t);




uint8_t fs_parse_mbr(uint16_t);
file_system_t* fs_get_root();
uint8_t fs_init();





#ifdef __cplusplus
}
#endif

#endif 

