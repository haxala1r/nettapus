#ifndef EXT2_H
#define EXT2_H 1

#include <stddef.h>
#include <stdint.h>

struct ext2_superblock {

	/* BASE SUPERBLOCK */
	uint32_t inode_count;
	uint32_t block_count;

	uint32_t reserved_block_count;

	uint32_t free_block_count;
	uint32_t free_inode_count;

	uint32_t super_block_num;

	/* These two are slightly misnamed, as they are actually log2(thing) - 10 */
	uint32_t log2_block_size;
	uint32_t log2_frag_size;

	uint32_t blocks_in_group;
	uint32_t frags_in_group;
	uint32_t inodes_in_group;

	uint32_t last_mount_time;
	uint32_t last_write_time;

	uint16_t mounts_since_fsck;
	uint16_t mounts_before_fsck;

	uint16_t ext2_signature;

	uint16_t fs_state;
	uint16_t on_error;

	uint16_t minor_version;

	uint32_t last_fsck_time;
	uint32_t time_between_fsck;

	uint32_t os_id;

	uint32_t major_version;

	uint16_t reserved_uid;
	uint16_t reserved_gid;

	/* EXTENDED SUPERBLOCK */

	uint32_t first_inode; /* first non-reserved inode*/
	uint32_t inode_struct_size;
	uint16_t bg_of_sb;	/* Block group of this superblock. */

	uint32_t optional_ft;
	uint32_t req_ft;
	uint32_t write_ft;

	uint8_t fs_id[16];
	char vol_name[16];

	char path_last_mount[64];

	uint32_t compress_algo;
	uint8_t prealloc_file;
	uint8_t prealloc_dir;

	uint16_t _unused;

	uint8_t journal_id[16];
	uint32_t journal_inode;
	uint32_t journal_dev;

	uint32_t head_orphan_inode;

} __attribute__((packed));


struct ext2_inode {
	uint16_t type_perm;
	uint16_t uid;
	uint32_t size_low;

	uint32_t last_access;
	uint32_t creation_time;
	uint32_t last_mod;
	uint32_t deletion_time;

	uint16_t gid;

	uint16_t hard_links;
	uint32_t sector_count;

	uint32_t flags;
	uint32_t os_specific1;

	uint32_t direct_block[12];

	uint32_t singly_indirect;
	uint32_t doubly_indirect;
	uint32_t triply_indirect;

	uint32_t gen_num;

	uint32_t extended_attr;
	uint32_t size_high;

	uint32_t frag_block_addr;

	char os_specific2[12];
} __attribute__((packed));


struct ext2_group_des {
	/* These are block addresses. */
	uint32_t block_bitmap_addr;
	uint32_t inode_bitmap_addr;
	uint32_t inode_table_addr;

	uint16_t free_block_count;
	uint16_t free_inode_count;
	uint16_t dir_count;

	char _pad[14];
} __attribute__((packed));

struct ext2_dir_entry {
	uint32_t inode;
	uint16_t size;
	uint8_t name_len;
	uint8_t type;
	char nm;
} __attribute__((packed));




struct ext2_fs {
	struct ext2_superblock *sb;

	/* These are here so that we don't compute them everytime a file is accessed. */
	size_t group_des_table_block;
	size_t block_size;
	size_t group_count;
};

struct ext2_file {
	size_t inode_addr;		/* This is here so that the node can be updated. */
	struct ext2_inode *node;
};

struct file_system;
struct drive;

size_t ext2_load_blocks(struct file_system *fs, void *buf, size_t block_addr, size_t count);
size_t ext2_write_blocks(struct file_system *fs, void *buf, size_t block_addr, size_t count);
size_t ext2_load_group_des(struct file_system *fs, struct ext2_group_des *dest, size_t group);
size_t ext2_block_in_inode(struct file_system *fs, struct ext2_inode *node, size_t n);
size_t ext2_alloc_blocks(struct file_system *fs, size_t count, uint32_t *ret);

size_t ext2_load_inode(struct file_system *fs, struct ext2_inode *dest, size_t inode_num);
size_t ext2_write_inode(struct file_system *fs, struct ext2_inode *dest, size_t inode_num);
size_t ext2_alloc_inode(struct file_system *fs);

/* Get attributes of inodes. */
int64_t ext2_get_size(struct file_system *fs, size_t inode_num);
int64_t ext2_get_links(struct file_system *fs, size_t inode_num);
uint16_t ext2_get_type_perm(struct file_system *fs, size_t inode_num);

uint8_t ext2_init_fs(struct file_system *fs);
uint8_t ext2_check_drive(struct drive *d);

size_t ext2_open_at(struct file_system *fs, size_t folder_inode, char *full_path);
size_t ext2_open(struct file_system *fs, char *full_path);
int64_t ext2_read_file(struct file_system *fs, size_t inode, void *dest_buf, size_t off, size_t bytes);
int64_t ext2_write_file(struct file_system *fs, size_t inode, void *dest_buf, size_t off, size_t bytes);

size_t ext2_mknod(struct file_system *fs, size_t parent_inode, char *file_name, uint16_t type_perm, size_t uid, size_t gid);

struct file_tnode *ext2_list_files(struct file_system *fs, size_t inode, size_t *count);
struct folder_tnode *ext2_list_folders(struct file_system *fs, size_t inode, size_t *count);

#endif
