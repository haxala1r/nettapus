#include <fs/fs.h>
#include <task.h>
#include <string.h>
#include <err.h>
#include <mem.h>

struct folder_tnode *root_tnode = NULL;

/* Searches a Directory for a Directory/File. Remember, when a folder vnode is
 * loaded into memory, t-nodes of all of its children are loaded along with
 * it (but not vnodes, just tnodes!). This means that if the vnode was
 * loaded, it already has a folder listing loaded as well.
 */
struct folder_tnode *vfs_search_dd(struct folder_vnode *node, char *name) {
	if (node == NULL) {
		return NULL;
	}
	if (node->mounted){
		node = node->mount_point->vnode;
	}
	acquire_semaphore(node->mutex);

	struct folder_tnode *i = node->subfolders;
	while (i != NULL) {
		if (!strcmp(i->folder_name, name)) {
			break;
		}

		i = i->next;
	}

	release_semaphore(node->mutex);
	return i;
}

struct file_tnode *vfs_search_df(struct folder_vnode *node, char *name) {
	if (node == NULL) { return NULL; }
	if (node->mounted){
		node = node->mount_point->vnode;
	}
	acquire_semaphore(node->mutex);

	struct file_tnode *i = node->subfiles;
	while (i != NULL) {
		if (!strcmp(i->file_name, name)) {
			break;
		}

		i = i->next;
	}

	release_semaphore(node->mutex);
	return i;
}

/* Load/free the list of a folder's children. */
void free_dir_list(struct folder_vnode *vnode) {
	if (vnode->subfiles != NULL) {
		struct file_tnode *i = vnode->subfiles;
		while (i) {
			if (i->vnode) {
				kfree(i->vnode->mutex);
				kfree(i->vnode->read_queue);
				kfree(i->vnode->write_queue);
				kfree(i->vnode);
			}
			kfree(i);

			i = i->next;
		}
		vnode->subfiles = NULL;
		vnode->subfile_count = 0;
	}
	if (vnode->subfolders != NULL) {
		/* TODO: free the folder tnodes here. */
		struct folder_tnode *i = vnode->subfolders;
		while (i) {
			if (i->vnode) {
				free_dir_list(i->vnode);
				kfree(i->vnode->mutex);
				kfree(i->vnode);
			}
			kfree(i);

			i = i->next;
		}
		vnode->subfiles = NULL;
		vnode->subfile_count = 0;
	}
}

size_t vfs_dir_load_list(struct folder_vnode *vnode) {
	if (vnode == NULL) { return ERR_INVALID_PARAM; }

	acquire_semaphore(vnode->mutex);
	free_dir_list(vnode);

	/* Get directory listings (only tnodes). */
	vnode->subfiles = vnode->fs->driver->list_files(vnode->fs, vnode->inode_num, &vnode->subfile_count);
	vnode->subfolders = vnode->fs->driver->list_folders(vnode->fs, vnode->inode_num, &vnode->subfolder_count);

	release_semaphore(vnode->mutex);
	return GENERIC_SUCCESS;
}


/* These functions are called when the child of a folder exists, but hasn't
 * been cached. This function loads the node into memory, and returns it.
 *
 * Keep in mind that whenever a folder is loaded, t-nodes of its children
 * are always loaded along with it. This helps us search for files/folders
 * faster.
 */
struct folder_vnode *vfs_load_folder_at(struct folder_vnode *parent, struct folder_tnode *tnode) {

	if (parent == NULL) { return NULL; }
	if (tnode == NULL)  { return NULL; }

	if (parent->mounted) {
		parent = parent->mount_point->vnode;
		if (parent == NULL) {
			return NULL;
		}
	}

	acquire_semaphore(parent->mutex);
	size_t inode = parent->fs->driver->openat(parent->fs, parent->inode_num, tnode->folder_name);
	release_semaphore(parent->mutex);
	if (inode <= 2) {
		/* Caller should try again. */
		vfs_dir_load_list(parent);
		return NULL;
	}

	/* TODO: check if a vnode with the same inode number already exists, and
	 * return that vnode if it already exists (increment cached_links field as well).
	 * A.k.a. check for already cached links.
	 */

	tnode->vnode = kmalloc(sizeof(struct folder_vnode));
	memset(tnode->vnode, 0, sizeof(struct folder_vnode));
	tnode->vnode->inode_num = inode;
	tnode->vnode->fs = parent->fs;
	tnode->vnode->mounted = 0;
	tnode->vnode->mount_point = NULL;
	tnode->vnode->mutex = create_semaphore(1);

	/* Load some attributes from the node for easy use.
	 * NOTE: Currently the functions used here all load the inode *every* time
	 * they're called. This should be changed to speed the file-loading stuff.
	 */
	tnode->vnode->type_perm = tnode->vnode->fs->driver->get_type_perm(tnode->vnode->fs, inode);
	//tnode->vnode->uid = tnode->vnode->fs->driver->get_uid(); TODO.
	tnode->vnode->link_count = tnode->vnode->fs->driver->get_links(tnode->vnode->fs, inode);

	if (vfs_dir_load_list(tnode->vnode)) {
		kfree(tnode->vnode->mutex);
		kfree(tnode->vnode);
		return NULL;
	}

	return tnode->vnode;
}

struct file_vnode *vfs_load_file_at(struct folder_vnode *parent, struct file_tnode *tnode) {
	if (parent == NULL) { return NULL; }
	if (tnode == NULL)  { return NULL; }

	/* Follow mounts. */
	if (parent->mounted) {
		parent = parent->mount_point->vnode;
		if (parent == NULL) {
			return NULL;
		}
	}

	acquire_semaphore(parent->mutex);
	size_t inode = parent->fs->driver->openat(parent->fs, parent->inode_num, tnode->file_name);
	release_semaphore(parent->mutex);
	if (inode <= 2) {
		/* Caller should try again. */
		vfs_dir_load_list(parent);
		return NULL;
	}

	/* TODO: check if a vnode with the same inode number already exists, and
	 * return that vnode if it already exists (increment cached_links field as well).
	 * A.k.a. check for already cached links.
	 */

	/* Load the node from disk. */
	tnode->vnode = kmalloc(sizeof(struct file_vnode));
	memset(tnode->vnode, 0, sizeof(struct file_vnode));
	tnode->vnode->inode_num = inode;
	tnode->vnode->fs = parent->fs;

	tnode->vnode->type_perm = 0; //parent->fs->driver->get_type_perm(parent->fs, inode_num);
	tnode->vnode->size = parent->fs->driver->get_size(parent->fs, inode);
	tnode->vnode->link_count = parent->fs->driver->get_links(parent->fs, inode);
	tnode->vnode->cached_links = 1;

	tnode->vnode->open = vfs_open_file;
	tnode->vnode->read = vfs_read_file;
	tnode->vnode->write = vfs_write_file;
	tnode->vnode->close = vfs_close_file;

	/* The mutex and the queues. */
	tnode->vnode->mutex = create_semaphore(1);
	tnode->vnode->read_queue = kmalloc(sizeof(QUEUE));
	memset(tnode->vnode->read_queue, 0, sizeof(QUEUE));
	tnode->vnode->write_queue = kmalloc(sizeof(QUEUE));
	memset(tnode->vnode->write_queue, 0, sizeof(QUEUE));

	return tnode->vnode;
}


struct file_vnode *vfs_search_file_vnode(char *path) {
	/* This function loads/finds a file vnode. If it doesn't find one of the nodes
	 * in the path cached, it will load them. If one of the nodes in the path
	 * just doesn't exist, it returns NULL.
	 */
	if (root_tnode == NULL) { return NULL; } /* The vfs hasn't been initialised yet. */
	char **arr = fs_parse_path(path);
	struct folder_tnode *cur_dir = root_tnode;
	if (root_tnode->vnode->mounted) {
		cur_dir = root_tnode->vnode->mount_point;
	}
	struct folder_tnode *par_dir = cur_dir; /* Parent of cur_dir*/
	size_t depth = 0;

	if ((arr[0] == NULL) || (cur_dir == NULL)) {
		fs_free_path(arr);
		return NULL;
	}

	while (arr[depth + 1] != NULL) {
		cur_dir = vfs_search_dd(par_dir->vnode, arr[depth]);

		if (cur_dir == NULL) {
			/* The node does not exist. */
			fs_free_path(arr);
			return NULL;
		}

		if (cur_dir->vnode == NULL) {
			/* The vnode exists, but hasn't been loaded yet. Load it. */
			if (vfs_load_folder_at(par_dir->vnode, cur_dir) == NULL) {
				fs_free_path(arr);
				return NULL;
			}
		}

		depth++;
		par_dir = cur_dir;
	}

	struct file_tnode *ret = vfs_search_df(cur_dir->vnode, arr[depth]);
	if (ret == NULL) {
		/* The file we're searching for does not exist. */
		fs_free_path(arr);
		return NULL;
	}
	if (ret->vnode == NULL) {
		/* It exists, but hasn't been loaded yet. */
		vfs_load_file_at(cur_dir->vnode, ret);
	}
	fs_free_path(arr);
	return ret->vnode;
}

size_t vfs_unload_fnode(struct file_vnode *f) {
	if (f == NULL) { return ERR_INVALID_PARAM; }

	destroy_semaphore(f->mutex);
	destroy_queue(f->read_queue);
	destroy_queue(f->write_queue);

	if (f->pipe_mem != NULL) {
		kfree(f->pipe_mem);
	}

	kfree(f);

	return GENERIC_SUCCESS;
}

size_t vfs_unload_dnode(struct folder_vnode *f) {
	if (f == NULL) { return ERR_INVALID_PARAM; }

	destroy_semaphore(f->mutex);
	free_dir_list(f);
	kfree(f);

	return GENERIC_SUCCESS;
}

struct file_vnode *mknode_file(char *path) {
	/* Firstly, we need to go through the path because we need the vnode of the
	 * file's parent.
	 */
	char **arr = fs_parse_path(path);
	struct folder_tnode *cur_dir = root_tnode;
	if (root_tnode->vnode->mounted) {
		cur_dir = root_tnode->vnode->mount_point;
	}
	struct folder_tnode *par_dir = cur_dir; /* Parent of cur_dir*/
	size_t depth = 0;

	if (arr[0] == NULL) {
		fs_free_path(arr);
		return NULL;
	}

	while (arr[depth + 1] != NULL) {
		cur_dir = vfs_search_dd(par_dir->vnode, arr[depth]);

		if (cur_dir == NULL) {
			fs_free_path(arr);
			return NULL;
		}

		if (cur_dir->vnode == NULL) {
			/* The vnode hasn't been loaded yet. Load it. */
			if (vfs_load_folder_at(par_dir->vnode, cur_dir) == NULL) {
				fs_free_path(arr);
				return NULL;
			}
		}

		depth++;
		par_dir = cur_dir;
	}

	/* Check if the file already exists. */
	struct file_tnode *ret = vfs_search_df(par_dir->vnode, arr[depth]);
	if (ret != NULL) {
		/* If it exists, just return. */
		return ret->vnode;    /* Might change to return NULL later. */
	}

	/* Create the file, as well as the tnode. */
	ret = kmalloc(sizeof(struct file_tnode));
	ret->file_name = kmalloc(strlen(arr[depth]) + 1);
	memcpy(ret->file_name, arr[depth], strlen(arr[depth]) + 1);

	ret->vnode = kmalloc(sizeof(struct file_vnode));

	/* Allocate the mutex and the queues. */
	ret->vnode->mutex = create_semaphore(1);
	ret->vnode->read_queue = kmalloc(sizeof(QUEUE));
	memset(ret->vnode->read_queue, 0, sizeof(QUEUE));
	ret->vnode->write_queue = kmalloc(sizeof(QUEUE));
	memset(ret->vnode->write_queue, 0, sizeof(QUEUE));


	ret->vnode->fs = par_dir->vnode->fs;
	ret->vnode->type_perm = 0; /* TODO: Replace with parameter. */
	ret->vnode->size = 0;
	ret->vnode->link_count = 1;
	ret->vnode->cached_links = 1;
	ret->vnode->inode_num = ret->vnode->fs->driver->mknod(ret->vnode->fs, par_dir->vnode->inode_num, ret->file_name, 0, 0, 0);

	ret->vnode->open = vfs_open_file;
	ret->vnode->close = vfs_close_file;
	ret->vnode->read = vfs_read_file;
	ret->vnode->write = vfs_write_file;

	vfs_dir_load_list(par_dir->vnode);

	return ret->vnode;
}


size_t vfs_mount_fs(struct file_system *fs, struct folder_vnode *node) {
	if (node->mounted == 1) {
		return ERR_INCOMPAT_PARAM; /* come up with a better error type. */
	}

	acquire_semaphore(node->mutex);
	node->mounted = 1;
	node->mount_point = fs->root_node;
	release_semaphore(node->mutex);
	return GENERIC_SUCCESS;
}


size_t init_vfs(struct file_system *root) {
	/* Mount the root file system to "/", create the "/" node etc. here. */
	if (root == NULL) { return ERR_INVALID_PARAM; }

	if (root_tnode == NULL) {
		root_tnode = kmalloc(sizeof(*root_tnode));
		memset(root_tnode, 0, sizeof(*root_tnode));
		root_tnode->folder_name = "/";
	}
	if (root_tnode->vnode == NULL) {
		root_tnode->vnode = kmalloc(sizeof(struct folder_vnode));
	}

	memset(root_tnode->vnode, 0, sizeof(*root_tnode->vnode));
	root_tnode->vnode->mutex = create_semaphore(1);

	return vfs_mount_fs(root, root_tnode->vnode);
}



#ifdef DEBUG
#include <tty.h>
void vfs_print_nodes() {
	struct folder_tnode *fi = root_tnode->vnode->subfolders;
	while (fi) {
		kputs(fi->folder_name);
		kputs("\n");
	}
}
#endif
