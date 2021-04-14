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

	uint32_t os_specific2;
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

	/* These are here so that we don't compute them everytime a file is accessed.
	 * They are easy to compute, but you don't really want to do that a thousand
	 * times a second.
	 */
	size_t group_des_table_block;
	size_t block_size;
};

struct ext2_file {
	size_t inode_addr;		/* This is here so that the node can be updated. */
	struct ext2_inode *node;
};


uint8_t ext2_init_fs(struct file_system *fs);
uint8_t ext2_read_file(struct file_system *fs, void *file_void, void *buf,
                     size_t offset, size_t bytes);
uint8_t ext2_write_file(struct file_system *fs, void *file_void, void *buf,
                     size_t offset, size_t bytes);
void *ext2_open_file(struct file_system *fs, char *path);

size_t ext2_get_size(void *);





uint8_t ext2_load_inode(struct file_system *fs, size_t inode_num, void *buf);
size_t ext2_inode_block(struct ext2_inode *f, size_t block_num);

uint8_t ext2_load_block(struct file_system *fs, size_t block_addr, void *buf);
uint8_t ext2_write_block(struct file_system *fs, size_t block_addr, void *buf,
                        size_t offset, size_t bytes);

struct ext2_group_des ext2_get_group_des(struct file_system *fs, size_t group);
uint8_t ext2_write_inode(struct file_system *fs, struct ext2_inode *n, size_t n_addr);

uint8_t ext2_alloc_blocks(struct file_system *fs, struct ext2_file *f, size_t blocks);
uint8_t ext2_inode_add_block(struct file_system *fs, struct ext2_inode *f, size_t block_addr);
#endif
