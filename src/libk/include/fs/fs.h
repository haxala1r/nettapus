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
	
	//This should point to FS-specific info, used by that specific FS driver (internally.)
	//it can be the location of a FAT, the LBA of sertain special sector, a BPB
	//etc. etc. 
	//none of the functions defined in this file will interfere with this. 
	void* special;		
};

//this holds a file's relevant info, from the point of view of the VFS.
struct file_vnode {
	struct file_system* fs;
	char* file_name;
	uint16_t stream_count;		//amount of streams open for this file.
	uint64_t size;		//the file's size, in bytes.
	
	
	struct file_vnode* next;	//these are kept in a linked list.
	
	//This should point to FS-specific info for the file. Things like where the meta-data
	//of said file is, where the actual data starts from, how big the file is,
	//and everything else that might be necessary.
	void* special;	
};



//an easy abstraction over files in a file system. This structure is from the point of view of
//the process requesting the file. IT WILL BE STORED SPECIFICALLY FOR EACH PROCESS, which
//results in something similar to file descriptors.
//this structure can easily be ported over to the userspace, when the time comes.

struct file_s {
	struct file_vnode* node;		//the vnode it refers to.
	
	int32_t file_des;	//file descriptor.

	uint64_t position;		//where exactly we are on the file.
	uint8_t mode;	//read/write. 0 = read, any other value = write.
	uint8_t flags;	//currently unused.
	
	struct file_s* next;
};


typedef struct file_system file_system_t;
typedef struct file_vnode FILE_VNODE;
typedef struct file_s FILE;








uint8_t fs_read_sectors(file_system_t*, uint32_t, uint32_t, void*);
uint8_t fs_write_sectors(file_system_t*, uint32_t, uint32_t, void*);

uint8_t fs_read_bytes(file_system_t*, void*, uint32_t, uint16_t, uint32_t);
uint8_t fs_write_bytes(file_system_t*, void*, uint32_t, uint16_t, uint32_t);




uint8_t fs_list_dirs(file_system_t, char*);


//WOO!! The classics! Now we're doing the Good Stuff!

int32_t kopen_fs(file_system_t*, char*, uint8_t);
int32_t kopen(char*, uint8_t);

int32_t kclose(int32_t);

int32_t kread(int32_t, void*, uint64_t);
int32_t kwrite(int32_t, void*, uint64_t);

int32_t kseek(int32_t, uint64_t);





uint8_t fs_parse_mbr(uint16_t);
file_system_t* fs_get_root();
uint8_t fs_init();


#ifdef DEBUG

void vfs_print_state(void);
void vfs_print_nodes(void);
void vfs_print_files(void);

#endif


#ifdef __cplusplus
}
#endif

#endif 

