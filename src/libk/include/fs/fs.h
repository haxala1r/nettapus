#ifndef FS_H 
#define FS_H 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>


//filesystem types.
#define FS_USTAR 	0
#define FS_FAT16 	1
//#define FS_FAT32 	2
#define FS_UNKNOWN 	0xFF



struct file_system {
	/* This holds info on what kind of file system it is. */
	size_t fs_type;
	
	/* The ID of the drive this file system is on. */
	size_t drive_id;
	
	/* The LBA of the file system's start. (i. e. first sector) */
	uint64_t starting_sector;	
	
	/* The amount of sectors it contains. */
	uint64_t sector_count;		
	
	/* All the file systems available to the kernel are kept in a singly linked
	 * list.
	 */
	struct file_system* next;
	
	/* When a file system is initialised, this pointer will be a pointer
	 * to FS-specific information contained in a struct by the relevant
	 * FS driver.
	 */
	void* special;		
};

/* This contains information on a file, from the point of the VFS. */
struct file_vnode {
	struct file_system *fs;
	char *file_name;
	uint16_t stream_count;		//amount of streams open for this file.
	uint64_t size;		//the file's size, in bytes.
	
	
	/* The vnodes are all kept in a doubly-linked list. */
	struct file_vnode *next;	
	struct file_vnode *prev;
	//This should point to FS-specific info for the file. Things like where the meta-data
	//of said file is, where the actual data starts from, how big the file is,
	//and everything else that might be necessary.
	void *special;	
};



/* This contains information on a file specific to a process (e. g. the current
 * position on the file), from the point of the process. Each process has
 * its own list of these.
 */
struct file_s {
	/* The vnode that keeps relevant info on the file. Multiple file descriptors
	 * can point to the same node. */
	struct file_vnode* node;		
	
	/* The file descriptor used to access this struct, by the process. */
	int32_t file_des;	

	/* Where exactly, e.g. a kread() call will start reading from. */
	uint64_t position;
	
	/* Read/Write. 0 = Read, 1 = Write. */		
	uint8_t mode;	
	
	/* This is currently unused. */
	uint8_t flags;	
	
	/* The file structs are always kept in a doubly linked list for each process. */
	struct file_s *next;
	struct file_s *prev;
} __attribute__((packed));


typedef struct file_system file_system_t;
typedef struct file_vnode FILE_VNODE;
typedef struct file_s FILE;








uint8_t fs_read_sectors(file_system_t*, uint64_t, uint32_t, void*);
uint8_t fs_write_sectors(file_system_t*, uint64_t, uint32_t, void*);

/* These may be removed in the foreseeable future. */
uint8_t fs_read_bytes(file_system_t*, void*, uint32_t, uint16_t, uint32_t);
uint8_t fs_write_bytes(file_system_t*, void*, uint32_t, uint16_t, uint32_t);




uint8_t fs_list_dirs(file_system_t, char*);


//WOO!! The classics! Now we're doing the Good Stuff!

int32_t kopen_fs(file_system_t*, char*, uint8_t);
int32_t kopen(char*, uint8_t);

int32_t kclose(int32_t);

int64_t kread(int32_t, void*, int64_t);
int64_t kwrite(int32_t, void*, int64_t);

int32_t kseek(int32_t, uint64_t);





uint8_t fs_parse_mbr(uint16_t);
file_system_t* fs_get_root();
uint8_t fs_init();


#ifdef DEBUG

void fs_print_state();

void vfs_print_state(void);
void vfs_print_nodes(void);
void vfs_print_files(void);

#endif


#ifdef __cplusplus
}
#endif

#endif 

