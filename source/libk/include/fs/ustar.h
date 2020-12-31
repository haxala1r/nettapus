#ifndef FS_USTAR_H
#define FS_USTAR_H 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <fs/fs.h>

//this is a struct to keep info. This is only used by the USTAR driver, internally.
//this is also used when the fs driver requests specific info.
struct USTAR_FILE {
	char file_name[100];
	uint32_t starting_lba;		//is the "meta-data"s starting sector, not the data.
	uint32_t size;				//in bytes! not sectors. might change that later on.
	char type;
	char owner_user_name[32];
	char owner_group_name[32];
};

typedef struct USTAR_FILE USTAR_FILE_t;



USTAR_FILE_t* ustar_file_lookup(file_system_t*, char*);

uint8_t ustar_load_file(file_system_t*, char*, void*);

uint8_t ustar_read_file(file_system_t*, char*, void*, uint32_t, uint32_t);
uint8_t ustar_write_file(file_system_t*, char*, void*, uint32_t, uint32_t);




#ifdef __cplusplus
}
#endif

#endif

