

#include <fs/fs.h>
#include <disk/disk.h>
#include <task.h>

#include <mem.h>
#include <err.h>

#include <fs/ext2.h>



const struct fs_driver ext2_driver = {
	.init_fs     = ext2_init_fs,
	.check_drive = ext2_check_drive,

	.open        = ext2_open,
	.openat      = ext2_open_at,
	.close       = NULL,
	.read        = ext2_read_file,
	.write       = ext2_write_file,

	.get_size    = ext2_get_size,
	.get_links   = ext2_get_links,
	.get_type_perm= ext2_get_type_perm,

	.list_files   = ext2_list_files,
	.list_folders= ext2_list_folders,

	.mknod       = ext2_mknod,

	.type        = FS_EXT2
};

/* This is a list of all FS drivers in the system. These will (hopefully) be
 * loaded at run-time in the future, in form of kernel modules.
 */
struct fs_driver fs_drivers[16] = {
	ext2_driver
};

/* Will be incremented every time a driver is added. */
size_t fs_drivers_count = 1;


struct file_system *root_fs = NULL;

struct file_system *fs_get_root() {
	return root_fs;
};

size_t fs_set_root(struct file_system *fs) {
	/* This is a convenience function that sets the root_fs variable and mounts
	 * the new filesystem to /
	 */
	if (fs == NULL) { return ERR_INVALID_PARAM; }
	root_fs = fs;
	return init_vfs(fs);
};


size_t fs_check_drive(struct drive *d) {
	/* Checks whether the given drive contains any of the drivable filesystems.*/
	if (d == NULL) { return 0; }

	for (size_t i = 0 ; i < fs_drivers_count; i++) {
		if (fs_drivers[i].init_fs == NULL) {
			continue;
		}

		size_t stat = fs_drivers[i].check_drive(d);

		if (stat == GENERIC_SUCCESS) {
			return fs_drivers[i].type;
		}
		/* Incompatible drive. */
		continue;
	}

	return 0;
};


uint8_t register_fs(struct drive *d) {
	if (d == NULL) { return 2; }

	struct file_system *new_fs = kmalloc(sizeof(*new_fs));
	memset(new_fs, 0, sizeof(*new_fs));
	new_fs->dev = d;

	/* Initialise the file system - e.g. load things like the superblock or BPB.*/
	for (size_t i = 0 ; i < fs_drivers_count; i++) {

		if (fs_drivers[i].init_fs == NULL) {
			/* We must have reached the end of the list. */
			kfree(new_fs);
			return ERR_INCOMPAT_PARAM;
		}

		size_t stat = fs_drivers[i].init_fs(new_fs);

		if (stat == GENERIC_SUCCESS) {
			/* We found the right driver. */
			new_fs->driver = fs_drivers + i;
			break;
		}
		/* Incompatible drive. */
		continue;
	}

	/* If no matching drivers were found, return an error. */
	if (new_fs->driver == NULL) {
		kfree(new_fs);
		return ERR_INCOMPAT_PARAM;
	}

	/* Otherwise, add the new file system to the list. */
	new_fs->next = root_fs;
	root_fs = new_fs;


	/* Load and create the root node for the file system. */
	new_fs->root_node = kmalloc(sizeof(struct folder_tnode));
	new_fs->root_node->folder_name = "/";

	new_fs->root_node->vnode = kmalloc(sizeof(struct folder_vnode));
	memset(new_fs->root_node->vnode, 0, sizeof(struct folder_vnode));

	new_fs->root_node->vnode->inode_num = new_fs->driver->open(new_fs, "/");

	new_fs->root_node->vnode->fs = new_fs;
	new_fs->root_node->vnode->mounted = 0;
	new_fs->root_node->vnode->mount_point = NULL;
	new_fs->root_node->vnode->mutex = create_semaphore(1);

	if (vfs_dir_load_list(new_fs->root_node->vnode)) {
		kfree(new_fs->root_node->vnode->mutex);
		kfree(new_fs->root_node->vnode);
		return ERR_DISK;
	}

	return GENERIC_SUCCESS;
};


size_t init_fs(void) {
	/* Goes over all available drives, and finds every filesystem it can recognize.
	 * Return value is the amount of file systems found. 0 means no file systems
	 * were found, and will generally be considered an error.
	 *
	 * TODO: it might be a good idea to add code to detect previous calls to this
	 * function and revert them, making it possible to attempt initialisation
	 * multiple times. That should also be done on all init_*() functions.
	 */

	if (refresh_disks()) {
		return 0;
	}

	struct drive *di = get_drive_list();

	size_t found_fs = 0;  /* Keep track of how many were found. */
	while (di != NULL) {
		if (di->type == DRIVE_TYPE_FS) {
			if (register_fs(di) == GENERIC_SUCCESS) {
				found_fs += 1;
			}
		}

		di = di->next;
	}

	if (found_fs) {
		init_vfs(root_fs);
	}

	return found_fs;
}


#ifdef DEBUG
#include <tty.h>

void fs_print_state(void) {
	kputs("\nFS STATES \n{fs_type}: {starting_lba} {sector count}\n");
	struct file_system *fs = root_fs;

	while (fs) {
		kputx(fs->fs_type);
		kputs(": ");

		kputx(fs->starting_sector);
		kputs(" ");

		kputx(fs->sector_count);
		kputs("\n");

		fs = fs->next;
	}
};


#endif
