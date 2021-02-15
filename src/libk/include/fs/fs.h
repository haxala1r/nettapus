#ifndef FS_H 
#define FS_H 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
//#include <task.h>

/* File systems */
#define FS_USTAR 	0
#define FS_FAT16 	1
//#define FS_FAT32 	2
#define FS_UNKNOWN 	0xFF


/* File types */
#define FILE_NORMAL 	0
#define FILE_PIPE 	1

/* Descriptor modes */
#define FD_READ		0
#define FD_WRITE		1

struct Task;	/* Defined in <task.h>*/
typedef struct Task Task;

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

struct file_s;

/* This contains information on a file/pipe/etc. , from the point of the VFS. */
struct file_vnode {
	/* The file system it resides on. */
	struct file_system *fs;
	
	/* The path of the file/pipe, all pipes are kept in a special FS. */
	char *file_name;
	
	/* This indicates the node's type. */
	uint16_t type;
	
	/* Amount of reading/writing streams open to this file/pipe. */
	size_t reader_count;		
	size_t writer_count;		
	
	/* The size of the file in bytes, for files (i. e. last available byte's
	 * position). 
	 * 
	 * For pipes, this indicates
	 * the place in the buffer that was written to last. 
	 */
	size_t last;
	
	/* This contains FS-specific info on a file, if it is a file, and it is
	 * a pointer to the (circular) buffer if it is a pipe.
	 */
	void *special;	
	
	/* The function pointers for specific operations. */
	int32_t (*open)(struct file_vnode *, void *, uint8_t);
	int32_t (*close)(struct file_s *);
	int64_t (*read)(struct file_s *, void *, int64_t);
	int64_t (*write)(struct file_s *, void *, int64_t);
	
	/* The vnodes are all kept in a doubly-linked list. */
	struct file_vnode *next;	
	struct file_vnode *prev;
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



/* Isn't implemented yet. */
uint8_t fs_list_dirs(file_system_t, char*);


/* These are some of the VFS stuff. Might be a good idea to put these in
 * a different header.
 */

int64_t read_pipe(FILE *f, void *buf, int64_t bytes);
int64_t write_pipe(FILE *f, void *buf, int64_t bytes);
int64_t close_pipe(FILE *f);

int64_t read_file(FILE *f, void *buf, int64_t bytes);
int64_t write_file(FILE *f, void *buf, int64_t bytes);
int64_t close_file(FILE *f);


int32_t kopen_fs(file_system_t *fs, char *fname, uint8_t mode);
int32_t kopen(char *fname, uint8_t mode);
int32_t pipeu(Task *task, int32_t *ret);

int32_t kclose(int32_t fd);

int64_t kread(int32_t fd, void *buf, int64_t bytes);
int64_t kwrite(int32_t fd, void *buf, int64_t bytes);

int32_t kseek(int32_t fd, uint64_t where);


uint8_t vfs_unlink_node(FILE_VNODE *node);
FILE *vfs_fd_lookup(Task *task, int32_t fd);




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

