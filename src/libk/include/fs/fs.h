#ifndef FS_H
#define FS_H 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>


/* File systems */
#define FS_UNKNOWN   0
#define FS_EXT2      1


/* Vnode types 						*/
#define FILE_NORMAL          0
#define FILE_PIPE_UNNAMED    1
#define FILE_PIPE_NAMED      2


/* Descriptor modes 				*/
#define FD_READ      0
#define FD_WRITE     1

/* Flags for file descriptors.		*/
#define FILE_FLAG_NONBLOCK   0b00000001

/* Some default values. */
#define DEFAULT_PIPE_SIZE    0x1000


/* Defined in <task.h>, which can't be included here because of magical resouns.*/
struct task;
struct semaphore;
struct queue;

typedef struct semaphore SEMAPHORE;
typedef struct queue QUEUE;

struct drive;
struct file_system;

struct file_tnode;
struct folder_tnode;
struct file_descriptor;

struct fs_driver {
	/* Loads any necessary information on a file_system, and performs
	 * initialisation.
	 * It MUST return an ERR_INCOMPAT_PARAM if the file system supplied to it
	 * isn't a file system it can drive. Return 0 if FS can be driven.
	 */
	uint8_t (*init_fs)(struct file_system *fs);

	/* Returns GENERIC_SUCCESS if the driver can drive this drive, but doesn't
	 * load any information.
	 */
	uint8_t (*check_drive)(struct drive *d);


	/* Finds a file on the given file system, given a path. Returns an FS-specific
	 * structure that will be the "special" field of the vnode, and be used by
	 * further calls to read, write and close.
	 */
	size_t (*open)(struct file_system *fs, char *path);
	size_t (*openat)(struct file_system *fs, size_t folder_inode, char *name);

	/* Flushes the changes made to the file, and makes sure everything matches
	 * what is on the disk.
	 */
	uint8_t (*close)(struct file_system *fs, size_t inode);

	/* Reads/writes a certain amount of bytes at offest to/from buf from/to f. */
	int64_t (*read)(struct file_system *fs, size_t inode, void *buf, size_t offset,
	                size_t bytes);
	int64_t (*write)(struct file_system *fs, size_t inode, void *buf, size_t offset,
	                 size_t bytes);
	int64_t (*get_size)(struct file_system *fs, size_t inode);
	int64_t (*get_links)(struct file_system *fs, size_t inode);
	uint16_t (*get_type_perm)(struct file_system *fs, size_t inode);

	struct file_tnode *(*list_files)(struct file_system *fs, size_t inode, size_t *count);
	struct folder_tnode *(*list_folders)(struct file_system *fs, size_t inode, size_t *count);

	size_t (*mknod)(struct file_system *fs, size_t inode, char *name, uint16_t type_perm, size_t uid, size_t gid);



	size_t type;	/* What file system it drives. */
};



struct file_system {
	struct drive *dev;

	/* All the file systems available to the kernel are kept in a singly linked
	 * list.
	 */
	struct file_system *next;

	/* This points to FS-specific information loaded by the driver. superblock, BPB, etc. */
	void *special;

	/* Holds function pointers of the driver, see above. */
	struct fs_driver *driver;

	/* The root node of the file system. */
	struct folder_tnode *root_node;
};



struct file_vnode {
	struct file_system *fs; /* The file system this node lies on. */

	/* Whether the file is a device, pipe, regular file etc. as well as permissions. */
	size_t type_perm;
	size_t owner_uid;
	size_t owner_gid;

	size_t size;

	size_t link_count;
	size_t cached_links; /* the amount of links already in cache. */

	/* The inode number on the concrete file system. */
	size_t inode_num;

	int32_t (*open)(struct file_vnode *vnode, struct task *t, size_t mode);
	int64_t (*read)(struct file_descriptor *, void *, int64_t count);
	int64_t (*write)(struct file_descriptor *, void *, int64_t count);
	int32_t (*close)(struct task *t, struct file_descriptor *);

	/* This solves the mutual exclusion problem. */
	SEMAPHORE *mutex;

	/* These two help us notify other processes when the file is written to/read
	 * from.
	 */
	QUEUE *read_queue;
	QUEUE *write_queue;

	size_t streams_open;

	/* The area of memory where the data is kept for a pipe. Unused for files. */
	void *pipe_mem;
};

struct file_tnode {
	char *file_name;
	struct file_vnode *vnode;
	struct file_tnode *next;   /* tnodes are kept in a linked list. */
};

struct folder_tnode;

struct folder_vnode {
	struct file_system *fs; /* The file system this node lies on. */

	uint16_t type_perm;
	uint32_t uid;
	uint32_t gid;

	size_t link_count;
	size_t cached_links; /* the amount of links already in cache. */

	struct file_tnode *subfiles;
	struct folder_tnode *subfolders;

	size_t subfile_count;
	size_t subfolder_count;

	size_t inode_num;

	/* Whether the folder is currently mounted, and if so, where. */
	size_t mounted;
	struct folder_tnode *mount_point;

	/* This solves the mutual exclusion problem. */
	SEMAPHORE *mutex;

	/* A folder doesn't need any queues.*/

	size_t streams_open;
};

struct folder_tnode {
	char *folder_name;
	struct folder_vnode *vnode;
	struct folder_tnode *next;   /* tnodes are kept in a linked list. */
};



struct file_descriptor {
	void *node;
	size_t file; /* Whether it points to a file or folder. 1 if file. */

	int32_t fd;  /* The actual number used to access this descriptor. */

	size_t pos;
	size_t mode;

	struct file_descriptor *next;
};

struct file_system *fs_get_root();
size_t fs_set_root(struct file_system *fs);



size_t fs_check_drive(struct drive *d);
char **fs_parse_path(char *path);
void fs_free_path(char **arr);


size_t vfs_dir_load_list(struct folder_vnode *vnode);
struct file_vnode *vfs_search_file_vnode(char *path);

size_t vfs_unload_fnode(struct file_vnode *f);
size_t vfs_unload_dnode(struct folder_vnode *f);

/* These are the functions used for regular files. */
int32_t vfs_open_file(struct file_vnode *, struct task *, size_t mode);
int64_t vfs_read_file(struct file_descriptor *, void *, int64_t count);
int64_t vfs_write_file(struct file_descriptor *, void *, int64_t count);
int32_t vfs_close_file(struct task *, struct file_descriptor *);

int64_t vfs_read_pipe(struct file_descriptor *fdes, void *buf, int64_t amount);
int64_t vfs_write_pipe(struct file_descriptor *fdes, void *buf, int64_t amount);

/* Functions used for operations on nodes. */
struct file_descriptor *vfs_find_fd(struct task *t, int32_t fd);
struct file_vnode *mknode_file(char *path);

struct file_descriptor *vfs_create_fd(struct task *t, void *node, size_t file, size_t mode);

int32_t pipeu(struct task *t, int32_t ret[]);
int32_t kopen(char *path, size_t mode);
int64_t kread(int32_t fd, void *buf, int64_t amount);
int64_t kwrite(int32_t fd, void *buf, int64_t amount);
int32_t kclose(int32_t fd);

size_t init_vfs(struct file_system *root);
size_t init_fs(void);




#ifdef DEBUG
void fs_print_state(void);
#endif


#ifdef __cplusplus
}
#endif

#endif

