/* Virtual File System and
** File System Interface Layer
*/

/* 
** Copyright 2002-2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <kernel.h>
#include <boot/stage2.h>
#include <vfs.h>
#include <vm.h>
#include <vm_cache.h>
#include <debug.h>
#include <khash.h>
#include <lock.h>
#include <thread.h>
#include <malloc.h>
#include <arch/cpu.h>
#include <elf.h>
#include <Errors.h>
#include <kerrors.h>
#include <fd.h>
#include <fs/node_monitor.h>

#include <OS.h>
#include <StorageDefs.h>
#include <fs_info.h>

#include "builtin_fs.h"

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <limits.h>
#include <stddef.h>

#ifndef TRACE_VFS
#	define TRACE_VFS 0
#endif
#if TRACE_VFS
#	define PRINT(x) dprintf x
#	define FUNCTION(x) dprintf x
#else
#	define PRINT(x) ;
#	define FUNCTION(x) ;
#endif

#define MAX_SYM_LINKS SYMLINKS_MAX

struct vnode {
	struct vnode	*next;
	struct vm_cache	*cache;
	mount_id		mount_id;
	list_link		mount_link;
	vnode_id		id;
	fs_vnode		private_node;
	struct fs_mount	*mount;
	struct vnode	*covered_by;
	int32			ref_count;
	bool			delete_me;
	bool			busy;
};

struct vnode_hash_key {
	mount_id	mount_id;
	vnode_id	vnode_id;
};

typedef struct file_system {
	struct file_system	*next;
	struct fs_ops		*ops;
	const char			*name;
	image_id			image;
	int32				ref_count;
} file_system;

#define FS_CALL(vnode, op) (vnode->mount->fs->ops->op)
#define FS_MOUNT_CALL(mount, op) (mount->fs->ops->op)

struct fs_mount {
	struct fs_mount	*next;
	file_system		*fs;
	mount_id		id;
	void			*cookie;
	char			*mount_point;
	recursive_lock	rlock;
	struct vnode	*root_vnode;
	struct vnode	*covers_vnode;
	struct list		vnodes;
	bool			unmounting;
};

static file_system *gFileSystems;
static mutex gFileSystemsMutex;

static mutex gMountMutex;
static mutex gMountOpMutex;
static mutex gVnodeMutex;

#define VNODE_HASH_TABLE_SIZE 1024
static void *gVnodeTable;
static struct vnode *gRoot;

#define MOUNTS_HASH_TABLE_SIZE 16
static void *gMountsTable;
static mount_id gNextMountID = 0;


/* function declarations */

// file descriptor operation prototypes
static ssize_t file_read(struct file_descriptor *, off_t pos, void *buffer, size_t *);
static ssize_t file_write(struct file_descriptor *, off_t pos, const void *buffer, size_t *);
static off_t file_seek(struct file_descriptor *, off_t pos, int seek_type);
static void file_free_fd(struct file_descriptor *);
static status_t file_close(struct file_descriptor *);
static status_t dir_read(struct file_descriptor *, struct dirent *buffer, size_t bufferSize, uint32 *_count);
static status_t dir_rewind(struct file_descriptor *);
static void dir_free_fd(struct file_descriptor *);
static status_t dir_close(struct file_descriptor *);
static status_t attr_dir_read(struct file_descriptor *, struct dirent *buffer, size_t bufferSize, uint32 *_count);
static status_t attr_dir_rewind(struct file_descriptor *);
static void attr_dir_free_fd(struct file_descriptor *);
static status_t attr_dir_close(struct file_descriptor *);
static ssize_t attr_read(struct file_descriptor *, off_t pos, void *buffer, size_t *);
static ssize_t attr_write(struct file_descriptor *, off_t pos, const void *buffer, size_t *);
static off_t attr_seek(struct file_descriptor *, off_t pos, int seek_type);
static void attr_free_fd(struct file_descriptor *);
static status_t attr_close(struct file_descriptor *);
static status_t attr_read_stat(struct file_descriptor *, struct stat *);
static status_t attr_write_stat(struct file_descriptor *, const struct stat *, int statMask);
static status_t index_dir_read(struct file_descriptor *, struct dirent *buffer, size_t bufferSize, uint32 *_count);
static status_t index_dir_rewind(struct file_descriptor *);
static void index_dir_free_fd(struct file_descriptor *);
static status_t index_dir_close(struct file_descriptor *);

static status_t common_ioctl(struct file_descriptor *, ulong, void *buf, size_t len);
static status_t common_read_stat(struct file_descriptor *, struct stat *);
static status_t common_write_stat(struct file_descriptor *, const struct stat *, int statMask);

static status_t dir_vnode_to_path(struct vnode *vnode, char *buffer, size_t bufferSize);
static void inc_vnode_ref_count(struct vnode *vnode);
static status_t dec_vnode_ref_count(struct vnode *vnode, bool reenter);
static inline void put_vnode(struct vnode *vnode);

struct fd_ops gFileOps = {
	file_read,
	file_write,
	file_seek,
	common_ioctl,
	NULL,		// select()
	NULL,		// deselect()
	NULL,		// read_dir()
	NULL,		// rewind_dir()
	common_read_stat,
	common_write_stat,
	file_close,
	file_free_fd
};

struct fd_ops gDirectoryOps = {
	NULL,		// read()
	NULL,		// write()
	NULL,		// seek()
	common_ioctl,
	NULL,		// select()
	NULL,		// deselect()
	dir_read,
	dir_rewind,
	common_read_stat,
	common_write_stat,
	dir_close,
	dir_free_fd
};

struct fd_ops gAttributeDirectoryOps = {
	NULL,		// read()
	NULL,		// write()
	NULL,		// seek()
	common_ioctl,
	NULL,		// select()
	NULL,		// deselect()
	attr_dir_read,
	attr_dir_rewind,
	common_read_stat,
	common_write_stat,
	attr_dir_close,
	attr_dir_free_fd
};

struct fd_ops gAttributeOps = {
	attr_read,
	attr_write,
	attr_seek,
	common_ioctl,
	NULL,		// select()
	NULL,		// deselect()
	NULL,		// read_dir()
	NULL,		// rewind_dir()
	attr_read_stat,
	attr_write_stat,
	attr_close,
	attr_free_fd
};

struct fd_ops gIndexDirectoryOps = {
	NULL,		// read()
	NULL,		// write()
	NULL,		// seek()
	NULL,		// ioctl()
	NULL,		// select()
	NULL,		// deselect()
	index_dir_read,
	index_dir_rewind,
	NULL,		// read_stat()
	NULL,		// write_stat()
	index_dir_close,
	index_dir_free_fd
};

#if 0
struct fd_ops gIndexOps = {
	NULL,		// read()
	NULL,		// write()
	NULL,		// seek()
	NULL,		// ioctl()
	NULL,		// select()
	NULL,		// deselect()
	NULL,		// dir_read()
	NULL,		// dir_rewind()
	index_read_stat,	// read_stat()
	NULL,		// write_stat()
	NULL,		// dir_close()
	NULL		// free_fd()
};
#endif


static int
mount_compare(void *_m, const void *_key)
{
	struct fs_mount *mount = _m;
	const mount_id *id = _key;

	if (mount->id == *id)
		return 0;

	return -1;
}


static uint32
mount_hash(void *_m, const void *_key, uint32 range)
{
	struct fs_mount *mount = _m;
	const mount_id *id = _key;

	if (mount)
		return mount->id % range;

	return *id % range;
}


/** Finds the mounted device (the fs_mount structure) with the given ID.
 *	Note, you must hold the gMountMutex lock when you call this function.
 */

static struct fs_mount *
find_mount(mount_id id)
{
	struct fs_mount *mount;
	ASSERT_LOCKED_MUTEX(&gMountMutex);

	mount = hash_lookup(gMountsTable, &id);

	return mount;
}


static struct fs_mount *
get_mount(mount_id id)
{
	struct fs_mount *mount;

	mutex_lock(&gMountMutex);

	mount = find_mount(id);
	if (mount) {
		// ToDo: the volume is locked (against removal) by locking
		//	its root node - investigate if that's a good idea
		if (mount->root_vnode)
			inc_vnode_ref_count(mount->root_vnode);
		else
			mount = NULL;
	}

	mutex_unlock(&gMountMutex);

	return mount;
}


static void
put_mount(struct fs_mount *mount)
{
	if (mount)
		put_vnode(mount->root_vnode);
}


/**	Creates a new file_system structure.
 *	The gFileSystems lock must be hold when you call this function.
 */

static file_system *
new_file_system(const char *name, struct fs_ops *ops)
{
	file_system *fs;

	ASSERT_LOCKED_MUTEX(&gFileSystemsMutex);

	fs = (struct file_system *)malloc(sizeof(struct file_system));
	if (fs == NULL)
		return NULL;

	fs->name = name;
	fs->ops = ops;
	fs->image = -1;
	fs->ref_count = 0;

	// add it to the queue

	fs->next = gFileSystems;
	gFileSystems = fs;

	return B_OK;
}


static status_t
unload_file_system(file_system *fs)
{
	void (*uninit)();

	// The image_id is invalid if it's an internal file system
	if (fs->image < B_OK)
		return B_OK;

	uninit = (void *)elf_lookup_symbol(fs->image, "uninit_file_system");
	if (uninit != NULL)
		uninit();

	// ToDo: unloading is not yet supported - we need a unload image_id first...
	free(fs);

	return B_OK;
}


static file_system *
load_file_system(const char *name)
{
	char path[SYS_MAX_PATH_LEN];
	file_system *fs;
	struct fs_ops **ops;
	int32 *version;
	void (*init)();
	image_id image;

	// search in the user directory
	sprintf(path, "/boot/home/config/add-ons/kernel/file_systems/%s", name);
	image = elf_load_kspace(path, "");
	if (image == B_ENTRY_NOT_FOUND) {
		// search in the system directory
		sprintf(path, "/boot/addons/fs/%s", name);
		//sprintf(path, "/boot/beos/system/add-ons/kernel/file_systems/%s", name);
		image = elf_load_kspace(path, "");
	}
	if (image < B_OK)
		return NULL;

	init = (void *)elf_lookup_symbol(image, "init_file_system");
	version = (int32 *)elf_lookup_symbol(image, "api_version");
	ops = (fs_ops **)elf_lookup_symbol(image, "fs_entry");
	if (init == NULL || version == NULL || ops == NULL) {
		dprintf("vfs: add-on \"%s\" doesn't export all necessary symbols.\n", name);
		goto err;
	}

	if (*version != B_CURRENT_FS_API_VERSION) {
		dprintf("vfs: add-on \"%s\" supports unknown API version %ld\n", name, *version);
		goto err;
	}

	fs = new_file_system(name, *ops);
	if (fs == NULL)
		goto err;

	fs->image = image;

	// initialize file system add-on
	init();

	return fs;

err:
	elf_unload_kspace(path);
	return NULL;
}


static status_t
put_file_system(file_system *fs)
{
	status_t status;

	mutex_lock(&gFileSystemsMutex);

	if (--fs->ref_count == 0)
		status = unload_file_system(fs);
	else
		status = B_OK;

	mutex_unlock(&gFileSystemsMutex);
	return status;
}


static file_system *
get_file_system(const char *name)
{
	file_system *fs;

	mutex_lock(&gFileSystemsMutex);

	for (fs = gFileSystems; fs != NULL; fs = fs->next) {
		if (!strcmp(name, fs->name))
			break;
	}

	if (fs == NULL)
		fs = load_file_system(name);

	// if we find a suitable file system, increment its reference counter
	if (fs)
		fs->ref_count++;

	mutex_unlock(&gFileSystemsMutex);
	return fs;
}


static int
vnode_compare(void *_vnode, const void *_key)
{
	struct vnode *vnode = _vnode;
	const struct vnode_hash_key *key = _key;

	if (vnode->mount_id == key->mount_id && vnode->id == key->vnode_id)
		return 0;

	return -1;
}


static uint32
vnode_hash(void *_vnode, const void *_key, uint32 range)
{
	struct vnode *vnode = _vnode;
	const struct vnode_hash_key *key = _key;

#define VHASH(mountid, vnodeid) (((uint32)((vnodeid) >> 32) + (uint32)(vnodeid)) ^ (uint32)(mountid))

	if (vnode != NULL)
		return (VHASH(vnode->mount_id, vnode->id) % range);
	else
		return (VHASH(key->mount_id, key->vnode_id) % range);

#undef VHASH
}


static void
add_vnode_to_mount_list(struct vnode *vnode, struct fs_mount *mount)
{
	recursive_lock_lock(&mount->rlock);

	list_add_link_to_head(&mount->vnodes, &vnode->mount_link);

	recursive_lock_unlock(&mount->rlock);
}


static void
remove_vnode_from_mount_list(struct vnode *vnode, struct fs_mount *mount)
{
	recursive_lock_lock(&mount->rlock);

	list_remove_link(&vnode->mount_link);
	vnode->mount_link.next = vnode->mount_link.prev = NULL;

	recursive_lock_unlock(&mount->rlock);
}


static struct vnode *
create_new_vnode(void)
{
	struct vnode *vnode;

	vnode = (struct vnode *)malloc(sizeof(struct vnode));
	if (vnode == NULL)
		return NULL;

	memset(vnode, 0, sizeof(struct vnode));
	return vnode;
}


static status_t
dec_vnode_ref_count(struct vnode *vnode, bool reenter)
{
	int err;
	int old_ref;

	mutex_lock(&gVnodeMutex);

	if (vnode->busy == true)
		panic("dec_vnode_ref_count called on vnode that was busy! vnode %p\n", vnode);

	old_ref = atomic_add(&vnode->ref_count, -1);

	PRINT(("dec_vnode_ref_count: vnode %p, ref now %ld\n", vnode, vnode->ref_count));

	if (old_ref == 1) {
		vnode->busy = true;

		mutex_unlock(&gVnodeMutex);

		/* if we have a vm_cache attached, remove it */
		if (vnode->cache)
			vm_cache_release_ref((vm_cache_ref *)vnode->cache);
		vnode->cache = NULL;

		if (vnode->delete_me)
			FS_CALL(vnode, remove_vnode)(vnode->mount->cookie, vnode->private_node, reenter);
		else
			FS_CALL(vnode, put_vnode)(vnode->mount->cookie, vnode->private_node, reenter);

		remove_vnode_from_mount_list(vnode, vnode->mount);

		mutex_lock(&gVnodeMutex);
		hash_remove(gVnodeTable, vnode);
		mutex_unlock(&gVnodeMutex);

		free(vnode);

		err = 1;
	} else {
		mutex_unlock(&gVnodeMutex);
		err = 0;
	}
	return err;
}


static void
inc_vnode_ref_count(struct vnode *vnode)
{
	atomic_add(&vnode->ref_count, 1);
	PRINT(("inc_vnode_ref_count: vnode %p, ref now %ld\n", vnode, vnode->ref_count));
}


static struct vnode *
lookup_vnode(mount_id mountID, vnode_id vnodeID)
{
	struct vnode_hash_key key;

	key.mount_id = mountID;
	key.vnode_id = vnodeID;

	return hash_lookup(gVnodeTable, &key);
}


static status_t
get_vnode(mount_id mountID, vnode_id vnodeID, struct vnode **_vnode, int reenter)
{
	struct vnode *vnode;
	int err;

	FUNCTION(("get_vnode: mountid %ld vnid 0x%Lx %p\n", mountID, vnodeID, _vnode));

	mutex_lock(&gVnodeMutex);

	while (true) {
		vnode = lookup_vnode(mountID, vnodeID);
		if (vnode && vnode->busy) {
			// ToDo: this is an endless loop if the vnode is not
			//	becoming unbusy anymore (for whatever reason)
			mutex_unlock(&gVnodeMutex);
			snooze(10000); // 10 ms
			mutex_lock(&gVnodeMutex);
			continue;
		}
		break;
	}

	PRINT(("get_vnode: tried to lookup vnode, got %p\n", vnode));

	if (vnode) {
		inc_vnode_ref_count(vnode);
	} else {
		// we need to create a new vnode and read it in
		vnode = create_new_vnode();
		if (!vnode) {
			err = B_NO_MEMORY;
			goto err;
		}
		vnode->mount_id = mountID;
		vnode->id = vnodeID;

		mutex_lock(&gMountMutex);	

		vnode->mount = find_mount(mountID);
		if (!vnode->mount) {
			mutex_unlock(&gMountMutex);
			err = B_ENTRY_NOT_FOUND;
			goto err;
		}
		vnode->busy = true;
		hash_insert(gVnodeTable, vnode);
		mutex_unlock(&gVnodeMutex);

		add_vnode_to_mount_list(vnode, vnode->mount);
		mutex_unlock(&gMountMutex);

		err = FS_CALL(vnode, get_vnode)(vnode->mount->cookie, vnodeID, &vnode->private_node, reenter);
		if (err < 0 || vnode->private_node == NULL) {
			remove_vnode_from_mount_list(vnode, vnode->mount);
			if (vnode->private_node == NULL)
				err = B_BAD_VALUE;
		}
		mutex_lock(&gVnodeMutex);
		if (err < 0)
			goto err1;

		vnode->busy = false;
		vnode->ref_count = 1;
	}

	mutex_unlock(&gVnodeMutex);

	PRINT(("get_vnode: returning %p\n", vnode));

	*_vnode = vnode;
	return B_OK;

err1:
	hash_remove(gVnodeTable, vnode);
err:
	mutex_unlock(&gVnodeMutex);
	if (vnode)
		free(vnode);

	return err;
}


static inline void
put_vnode(struct vnode *vnode)
{
	dec_vnode_ref_count(vnode, false);
}


static status_t
entry_ref_to_vnode(mount_id mountID, vnode_id directoryID, const char *name, struct vnode **_vnode)
{
	struct vnode *directory, *vnode;
	vnode_id id;
	int status;
	int type;

	status = get_vnode(mountID, directoryID, &directory, false);
	if (status < 0)
		return status;

	status = FS_CALL(directory, lookup)(directory->mount->cookie,
		directory->private_node, name, &id, &type);
	put_vnode(directory);

	if (status < 0)
		return status;

	mutex_lock(&gVnodeMutex);
	vnode = lookup_vnode(mountID, id);
	mutex_unlock(&gVnodeMutex);

	if (vnode == NULL) {
		// fs_lookup() should have left the vnode referenced, so chances
		// are good that this will never happen
		panic("entry_ref_to_vnode: could not lookup vnode (mountid 0x%lx vnid 0x%Lx)\n", mountID, id);
		return B_ENTRY_NOT_FOUND;
	}

	*_vnode = vnode;
	return B_OK;
}


static status_t
vnode_path_to_vnode(struct vnode *vnode, char *path, bool traverseLeafLink,
	int count, struct vnode **_vnode, int *_type)
{
	status_t status = 0;
	int type = 0;

	FUNCTION(("vnode_path_to_vnode(path = %s)\n", path));

	if (!path)
		return EINVAL;

	while (true) {
		struct vnode *nextVnode;
		vnode_id vnodeID;
		char *nextPath;

		PRINT(("vnode_path_to_vnode: top of loop. p = %p, p = '%s'\n", path, path));

		// done?
		if (path[0] == '\0')
			break;

		// walk to find the next path component ("path" will point to a single
		// path component), and filter out multiple slashes
		for (nextPath = path + 1; *nextPath != '\0' && *nextPath != '/'; nextPath++);

		if (*nextPath == '/') {
			*nextPath = '\0';
			do
				nextPath++;
			while (*nextPath == '/');
		}

		// See if the .. is at the root of a mount and move to the covered
		// vnode so we pass the '..' parse to the underlying filesystem
		if (!strcmp("..", path)
			&& vnode->mount->root_vnode == vnode
			&& vnode->mount->covers_vnode) {

			nextVnode = vnode->mount->covers_vnode;
			inc_vnode_ref_count(nextVnode);
			put_vnode(vnode);
			vnode = nextVnode;
		}

		// Check if we have the right to search the current directory vnode.
		// If a file system doesn't have the access() function, we assume that
		// searching a directory is always allowed
		if (FS_CALL(vnode, access))
			status = FS_CALL(vnode, access)(vnode->mount->cookie, vnode->private_node, X_OK);

		// Tell the filesystem to get the vnode of this path component (if we got the
		// permission from the call above)
		if (status >= B_OK)
			status = FS_CALL(vnode, lookup)(vnode->mount->cookie, vnode->private_node, path, &vnodeID, &type);

		if (status < B_OK) {
			put_vnode(vnode);
			return status;
		}

		// Lookup the vnode, the call to fs_lookup should have caused a get_vnode to be called
		// from inside the filesystem, thus the vnode would have to be in the list and it's
		// ref count incremented at this point
		mutex_lock(&gVnodeMutex);
		nextVnode = lookup_vnode(vnode->mount_id, vnodeID);
		mutex_unlock(&gVnodeMutex);

		if (!nextVnode) {
			// pretty screwed up here - the file system found the vnode, but the hash
			// lookup failed, so our internal structures are messed up
			panic("path_to_vnode: could not lookup vnode (mountid 0x%lx vnid 0x%Lx)\n", vnode->mount_id, vnodeID);
			put_vnode(vnode);
			return B_ENTRY_NOT_FOUND;
		}

		// If the new node is a symbolic link, resolve it (if we've been told to do it)
		if (S_ISLNK(type) && !(!traverseLeafLink && nextPath[0] == '\0')) {
			char *buffer;

			// it's not exactly nice style using goto in this way, but hey, it works :-/
			if (count + 1 > MAX_SYM_LINKS) {
				status = B_LINK_LIMIT;
				goto resolve_link_error;
			}

			buffer = malloc(SYS_MAX_PATH_LEN);
			if (buffer == NULL) {
				status = B_NO_MEMORY;
				goto resolve_link_error;
			}

			status = FS_CALL(nextVnode, read_link)(nextVnode->mount->cookie, nextVnode->private_node, buffer, SYS_MAX_PATH_LEN);
			if (status < B_OK) {
				free(buffer);

		resolve_link_error:
				put_vnode(vnode);
				put_vnode(nextVnode);

				return status;
			}
			put_vnode(nextVnode);

			// Check if we start from the root directory or the current
			// directory ("vnode" still points to that one).
			// Cut off all leading slashes if it's the root directory
			path = buffer;
			if (path[0] == '/') {
				// we don't need the old directory anymore
				put_vnode(vnode);

				while (*++path == '/')
					;
				vnode = gRoot;
				inc_vnode_ref_count(vnode);
			}

			status = vnode_path_to_vnode(vnode, path, traverseLeafLink, count + 1, &nextVnode, _type);

			free(buffer);

			if (status < B_OK) {
				put_vnode(vnode);
				return status;
			}
		}

		// decrease the ref count on the old dir we just looked up into
		put_vnode(vnode);

		path = nextPath;
		vnode = nextVnode;

		// see if we hit a mount point
		if (vnode->covered_by) {
			nextVnode = vnode->covered_by;
			inc_vnode_ref_count(nextVnode);
			put_vnode(vnode);
			vnode = nextVnode;
		}
	}

	*_vnode = vnode;
	if (_type)
		*_type = type;

	return B_OK;
}


static status_t
path_to_vnode(char *path, bool traverseLink, struct vnode **_vnode, bool kernel)
{
	struct vnode *start;

	FUNCTION(("path_to_vnode(path = \"%s\")\n", path));

	if (!path)
		return EINVAL;

	// figure out if we need to start at root or at cwd
	if (*path == '/') {
		while (*++path == '/')
			;
		start = gRoot;
		inc_vnode_ref_count(start);
	} else {
		struct io_context *context = get_current_io_context(kernel);

		mutex_lock(&context->io_mutex);
		start = context->cwd;
		inc_vnode_ref_count(start);
		mutex_unlock(&context->io_mutex);
	}

	return vnode_path_to_vnode(start, path, traverseLink, 0, _vnode, NULL);
}


/** Returns the vnode in the next to last segment of the path, and returns
 *	the last portion in filename.
 *	The path buffer must be able to store at least one additional character.
 */

static status_t
path_to_dir_vnode(char *path, struct vnode **_vnode, char *filename, bool kernel)
{
	char *p = strrchr(path, '/');
		// '/' are not allowed in file names!

	FUNCTION(("path_to_dir_vnode(path = %s)\n", path));

	if (!p) {
		// this path is single segment with no '/' in it
		// ex. "foo"
		strcpy(filename, path);
		strcpy(path, ".");
	} else {
		// replace the filename portion of the path with a '.'
		strcpy(filename, ++p);

		if (p[0] != '\0'){
			p[0] = '.';
			p[1] = '\0';
		}
	}
	return path_to_vnode(path, true, _vnode, kernel);
}


/**	Gets the full path to a given directory vnode.
 *	It uses the fs_get_vnode_name() call to get the name of a vnode; if a
 *	file system doesn't support this call, it will fall back to iterating
 *	through the parent directory to get the name of the child.
 *
 *	To protect against circular loops, it supports a maximum tree depth
 *	of 256 levels.
 *
 *	Note that the path may not be correct the time this function returns!
 *	It doesn't use any locking to prevent returning the correct path, as
 *	paths aren't safe anyway: the path to a file can change at any time.
 *
 *	It might be a good idea, though, to check if the returned path exists
 *	in the calling function (it's not done here because of efficiency)
 */

static status_t
dir_vnode_to_path(struct vnode *vnode, char *buffer, size_t bufferSize)
{
	/* this implementation is currently bound to SYS_MAX_PATH_LEN */
	char path[SYS_MAX_PATH_LEN];
	int32 insert = sizeof(path);
	int32 maxLevel = 256;
	int32 length;
	status_t status;

	if (vnode == NULL || buffer == NULL)
		return EINVAL;

	// we don't use get_vnode() here because this call is more
	// efficient and does all we need from get_vnode()
	inc_vnode_ref_count(vnode);

	path[--insert] = '\0';
	
	while (true) {
		// the name buffer is also used for fs_read_dir()
		char nameBuffer[sizeof(struct dirent) + B_FILE_NAME_LENGTH];
		char *name = &((struct dirent *)nameBuffer)->d_name[0];
		struct vnode *parentVnode;
		vnode_id parentID, id;
		int type;

		// lookup the parent vnode
		status = FS_CALL(vnode, lookup)(vnode->mount->cookie,vnode->private_node,"..",&parentID,&type);
		if (status < B_OK)
			goto out;

		mutex_lock(&gVnodeMutex);
		parentVnode = lookup_vnode(vnode->mount_id, parentID);
		mutex_unlock(&gVnodeMutex);
		
		if (parentVnode == NULL) {
			panic("dir_vnode_to_path: could not lookup vnode (mountid 0x%lx vnid 0x%Lx)\n", vnode->mount_id, parentID);
			status = B_ENTRY_NOT_FOUND;
			goto out;
		}

		// Does the file system support getting the name of a vnode?
		// If so, get it here...
		if (status == B_OK && FS_CALL(vnode, get_vnode_name))
			status = FS_CALL(vnode, get_vnode_name)(vnode->mount->cookie,vnode->private_node,name,B_FILE_NAME_LENGTH);

		// ... if not, find it out later (by iterating through
		// the parent directory, searching for the id)
		id = vnode->id;

		// release the current vnode, we only need its parent from now on
		put_vnode(vnode);
		vnode = parentVnode;

		if (status < B_OK)
			goto out;

		// ToDo: add an explicit check for loops in about 10 levels to do
		// real loop detection

		// don't go deeper as 'maxLevel' to prevent circular loops
		if (maxLevel-- < 0) {
			status = ELOOP;
			goto out;
		}

		if (parentID == id) {
			// we have reached the root level directory of this file system
			// which means we have constructed the full path
			break;
		}

		if (!FS_CALL(vnode, get_vnode_name)) {
			// If we don't got the vnode's name yet, we have to search for it
			// in the parent directory now
			fs_cookie cookie;

			status = FS_CALL(vnode, open_dir)(vnode->mount->cookie, vnode->private_node, &cookie);
			if (status >= B_OK) {
				struct dirent *dirent = (struct dirent *)nameBuffer;
				while (true) {
					uint32 num = 1;
					status = FS_CALL(vnode, read_dir)(vnode->mount->cookie, vnode->private_node,
						cookie, dirent, sizeof(nameBuffer), &num);
					if (status < B_OK)
						break;
					
					if (id == dirent->d_ino)
						// found correct entry!
						break;
				}
				FS_CALL(vnode, close_dir)(vnode->mount->cookie, vnode->private_node, cookie);
			}

			if (status < B_OK)
				goto out;
		}

		// add the name infront of the current path
		name[B_FILE_NAME_LENGTH - 1] = '\0';
		length = strlen(name);
		insert -= length;
		if (insert <= 0) {
			status = ENOBUFS;
			goto out;
		}
		memcpy(path + insert, name, length);
		path[--insert] = '/';
	}

	// add the mountpoint
	length = strlen(vnode->mount->mount_point);
	if (bufferSize - (sizeof(path) - insert) < (uint32)length + 1) {
		status = ENOBUFS;
		goto out;
	}

	memcpy(buffer, vnode->mount->mount_point, length);
	if (insert != sizeof(path))
		memcpy(buffer + length, path + insert, sizeof(path) - insert);

out:
	put_vnode(vnode);
	return status;
}


/**	Checks the length of every path component, and adds a '.'
 *	if the path ends in a slash.
 *	The given path buffer must be able to store at least one
 *	additional character.
 */

static status_t
check_path(char *to)
{
	int32 length = 0;

	// check length of every path component

	while (*to) {
		char *begin;
		if (*to == '/')
			to++, length++;

		begin = to;
		while (*to != '/' && *to)
			to++, length++;

		if (to - begin > B_FILE_NAME_LENGTH)
			return B_NAME_TOO_LONG;
	}

	if (length == 0)
		return B_ENTRY_NOT_FOUND;

	// complete path if there is a slash at the end

	if (*(to - 1) == '/') {
		if (length > SYS_MAX_PATH_LEN - 2)
			return B_NAME_TOO_LONG;

		to[0] = '.';
		to[1] = '\0';
	}

	return B_OK;
}


static struct file_descriptor *
get_fd_and_vnode(int fd, struct vnode **_vnode, bool kernel)
{
	struct file_descriptor *descriptor = get_fd(get_current_io_context(kernel), fd);
	if (descriptor == NULL)
		return NULL;

	if (descriptor->u.vnode == NULL) {
		put_fd(descriptor);
		return NULL;
	}

	*_vnode = descriptor->u.vnode;
	return descriptor;
}


static struct vnode *
get_vnode_from_fd(struct io_context *ioContext, int fd)
{
	struct file_descriptor *descriptor;
	struct vnode *vnode;

	descriptor = get_fd(ioContext, fd);
	if (descriptor == NULL)
		return NULL;

	vnode = descriptor->u.vnode;
	if (vnode != NULL)
		inc_vnode_ref_count(vnode);

	put_fd(descriptor);
	return vnode;
}


static status_t
fd_and_path_to_vnode(int fd, char *path, bool traverseLeafLink, struct vnode **_vnode, bool kernel)
{
	struct vnode *vnode;

	if (fd != -1) {
		struct file_descriptor *descriptor = get_fd_and_vnode(fd, &vnode, kernel);
		if (descriptor == NULL)
			return B_FILE_ERROR;

		inc_vnode_ref_count(vnode);
		put_fd(descriptor);
		
		*_vnode = vnode;
		return B_OK;
	}

	return path_to_vnode(path, traverseLeafLink, _vnode, kernel);
}


static int
get_new_fd(int type, struct vnode *vnode, fs_cookie cookie, int openMode, bool kernel)
{
	struct file_descriptor *descriptor;
	int fd;

	descriptor = alloc_fd();
	if (!descriptor)
		return B_NO_MEMORY;

	descriptor->u.vnode = vnode;
	descriptor->cookie = cookie;
	switch (type) {
		case FDTYPE_FILE:
			descriptor->ops = &gFileOps;
			break;
		case FDTYPE_DIR:
			descriptor->ops = &gDirectoryOps;
			break;
		case FDTYPE_ATTR:
			descriptor->ops = &gAttributeOps;
			break;
		case FDTYPE_ATTR_DIR:
			descriptor->ops = &gAttributeDirectoryOps;
			break;
		case FDTYPE_INDEX_DIR:
			descriptor->ops = &gIndexDirectoryOps;
			break;
		default:
			panic("get_new_fd() called with unknown type %d\n", type);
			break;
	}
	descriptor->type = type;
	descriptor->open_mode = openMode;

	fd = new_fd(get_current_io_context(kernel), descriptor);
	if (fd < 0) {
		free(descriptor);
		return B_NO_MORE_FDS;
	}

	return fd;
}


//	#pragma mark -
//	Functions the VFS exports for other parts of the kernel


status_t
vfs_new_vnode(mount_id mountID, vnode_id vnodeID, fs_vnode privateNode)
{
	struct vnode *vnode;
	status_t status = B_OK;

	if (privateNode == NULL)
		return B_BAD_VALUE;

	mutex_lock(&gVnodeMutex);

	// file system integrity check:
	// test if the vnode already exists and bail out if this is the case!

	// ToDo: the R5 implementation obviously checks for a different cookie
	//	and doesn't panic if they are equal

	vnode = lookup_vnode(mountID, vnodeID);
	if (vnode != NULL)
		panic("vnode %ld:%Ld already exists (node = %p, vnode->node = %p)!", mountID, vnodeID, privateNode, vnode->private_node);

	vnode = create_new_vnode();
	if (!vnode) {
		status = B_NO_MEMORY;
		goto err;
	}

	vnode->mount_id = mountID;
	vnode->id = vnodeID;
	vnode->private_node = privateNode;

	// add the vnode to the mount structure
	mutex_lock(&gMountMutex);	
	vnode->mount = find_mount(mountID);
	if (!vnode->mount) {
		mutex_unlock(&gMountMutex);
		status = B_ENTRY_NOT_FOUND;
		goto err;
	}
	hash_insert(gVnodeTable, vnode);

	add_vnode_to_mount_list(vnode, vnode->mount);
	mutex_unlock(&gMountMutex);

	vnode->ref_count = 1;

err:	
	mutex_unlock(&gVnodeMutex);

	return status;
}


status_t
vfs_get_vnode(mount_id mountID, vnode_id vnodeID, fs_vnode *_fsNode)
{
	struct vnode *vnode;

	int status = get_vnode(mountID, vnodeID, &vnode, true);
	if (status < 0)
		return status;

	*_fsNode = vnode->private_node;
	return B_OK;
}


status_t
vfs_put_vnode(mount_id mountID, vnode_id vnodeID)
{
	struct vnode *vnode;

	mutex_lock(&gVnodeMutex);
	vnode = lookup_vnode(mountID, vnodeID);
	mutex_unlock(&gVnodeMutex);

	if (vnode)
		dec_vnode_ref_count(vnode, true);

	return B_OK;
}


void
vfs_vnode_acquire_ref(void *vnode)
{
	FUNCTION(("vfs_vnode_acquire_ref: vnode 0x%p\n", vnode));
	inc_vnode_ref_count((struct vnode *)vnode);
}


void
vfs_vnode_release_ref(void *vnode)
{
	FUNCTION(("vfs_vnode_release_ref: vnode 0x%p\n", vnode));
	dec_vnode_ref_count((struct vnode *)vnode, false);
}


status_t
vfs_remove_vnode(mount_id mountID, vnode_id vnodeID)
{
	struct vnode *vnode;

	mutex_lock(&gVnodeMutex);

	vnode = lookup_vnode(mountID, vnodeID);
	if (vnode)
		vnode->delete_me = true;

	mutex_unlock(&gVnodeMutex);
	return 0;
}


void *
vfs_get_cache_ptr(void *vnode)
{
	return ((struct vnode *)vnode)->cache;
}


int
vfs_set_cache_ptr(void *vnode, void *cache)
{
	if (atomic_test_and_set((int32 *)&(((struct vnode *)vnode)->cache), (int32)cache, 0) == 0)
		return 0;

	return -1;
}


int
vfs_get_vnode_from_fd(int fd, bool kernel, void **vnode)
{
	struct io_context *ioctx;

	ioctx = get_current_io_context(kernel);
	*vnode = get_vnode_from_fd(ioctx, fd);

	if (*vnode == NULL)
		return B_FILE_ERROR;

	return B_NO_ERROR;
}


int
vfs_get_vnode_from_path(const char *path, bool kernel, void **_vnode)
{
	struct vnode *vnode;
	int err;
	char buf[SYS_MAX_PATH_LEN+1];

	PRINT(("vfs_get_vnode_from_path: entry. path = '%s', kernel %d\n", path, kernel));

	strncpy(buf, path, SYS_MAX_PATH_LEN);
	buf[SYS_MAX_PATH_LEN] = 0;

	err = path_to_vnode(buf, true, &vnode, kernel);
	if (err >= 0)
		*_vnode = vnode;

	return err;
}


status_t
vfs_get_module_path(const char *basePath, const char *moduleName, char *pathBuffer,
	size_t bufferSize)
{
	struct vnode *dir, *file;
	status_t status;
	size_t length;
	char *path;

	if (strlcpy(pathBuffer, basePath, bufferSize) > bufferSize)
		return B_BUFFER_OVERFLOW;

	status = path_to_vnode(pathBuffer, true, &dir, true);
	if (status < B_OK)
		return status;

	length = strlen(pathBuffer);
	if (pathBuffer[length - 1] != '/') {
		// ToDo: bufferSize race condition
		pathBuffer[length] = '/';
		length++;
	}

	path = pathBuffer + length;
	bufferSize -= length;

	while (moduleName) {
		int type;

		char *nextPath = strchr(moduleName, '/');
		if (nextPath == NULL)
			length = strlen(moduleName);
		else
			length = nextPath - moduleName;

		if (length + 1>= bufferSize) {
			status = B_BUFFER_OVERFLOW;
			goto err;
		}

		memcpy(path, moduleName, length);
		path[length] = '\0';
		moduleName = nextPath;

		status = vnode_path_to_vnode(dir, path, true, 0, &file, &type);
		if (status < B_OK)
			goto err;

		put_vnode(dir);

		if (S_ISDIR(type)) {
			// goto the next directory
			path[length] = '/';
			path[length + 1] = '\0';
			path += length + 1;

			dir = file;
		} else if (S_ISREG(type)) {
			// it's a file so it should be what we've searched for
			put_vnode(file);

			return B_OK;
		} else {
			PRINT(("vfs_get_module_path(): something is strange here...\n"));
			status = B_ERROR;
			goto err;
		}
	}

	// if we got here, the moduleName just pointed to a directory, not to
	// a real module - what should we do in this case?
	status = B_ENTRY_NOT_FOUND;

err:
	put_vnode(dir);
	return status;
}


int
vfs_put_vnode_ptr(void *vnode)
{
	struct vnode *v = vnode;

	put_vnode(v);

	return 0;
}


ssize_t
vfs_can_page(void *_v)
{
	struct vnode *vnode = _v;

	FUNCTION(("vfs_canpage: vnode 0x%p\n", vnode));

	if (FS_CALL(vnode, can_page))
		return FS_CALL(vnode, can_page)(vnode->mount->cookie, vnode->private_node);

	return 0;
}


ssize_t
vfs_read_page(void *_v, iovecs *vecs, off_t pos)
{
	struct vnode *vnode = _v;

	FUNCTION(("vfs_readpage: vnode %p, vecs %p, pos %Ld\n", vnode, vecs, pos));

	return FS_CALL(vnode, read_pages)(vnode->mount->cookie, vnode->private_node, vecs, pos);
}


ssize_t
vfs_write_page(void *_v, iovecs *vecs, off_t pos)
{
	struct vnode *vnode = _v;

	FUNCTION(("vfs_writepage: vnode %p, vecs %p, pos %Ld\n", vnode, vecs, pos));

	return FS_CALL(vnode, write_pages)(vnode->mount->cookie, vnode->private_node, vecs, pos);
}


/** Sets up a new io_control structure, and inherits the properties
 *	of the parent io_control if it is given.
 */

void *
vfs_new_io_context(void *_parentContext)
{
	size_t table_size;
	struct io_context *context;
	struct io_context *parentContext;

	context = malloc(sizeof(struct io_context));
	if (context == NULL)
		return NULL;

	memset(context, 0, sizeof(struct io_context));

	parentContext = (struct io_context *)_parentContext;
	if (parentContext)
		table_size = parentContext->table_size;
	else
		table_size = DEFAULT_FD_TABLE_SIZE;

	context->fds = malloc(sizeof(struct file_descriptor *) * table_size);
	if (context->fds == NULL) {
		free(context);
		return NULL;
	}

	memset(context->fds, 0, sizeof(struct file_descriptor *) * table_size);

	if (mutex_init(&context->io_mutex, "I/O context") < 0) {
		free(context->fds);
		free(context);
		return NULL;
	}

	// Copy all parent files which don't have the O_CLOEXEC flag set

	if (parentContext) {
		size_t i;

		mutex_lock(&parentContext->io_mutex);

		context->cwd = parentContext->cwd;
		if (context->cwd)
			inc_vnode_ref_count(context->cwd);

		for (i = 0; i < table_size; i++) {
			if (parentContext->fds[i] && (parentContext->fds[i]->open_mode & O_CLOEXEC) == 0) {
				context->fds[i] = parentContext->fds[i];
				atomic_add(&context->fds[i]->ref_count, 1);
			}
		}

		mutex_unlock(&parentContext->io_mutex);
	} else {
		context->cwd = gRoot;

		if (context->cwd)
			inc_vnode_ref_count(context->cwd);
	}

	context->table_size = table_size;

	list_init(&context->node_monitors);
	context->max_monitors = MAX_NODE_MONITORS;

	return context;
}


int
vfs_free_io_context(void *_ioContext)
{
	struct io_context *context = (struct io_context *)_ioContext;
	uint32 i;

	if (context->cwd)
		dec_vnode_ref_count(context->cwd, false);

	mutex_lock(&context->io_mutex);

	for (i = 0; i < context->table_size; i++) {
		if (context->fds[i])
			put_fd(context->fds[i]);
	}

	mutex_unlock(&context->io_mutex);

	mutex_destroy(&context->io_mutex);

	remove_node_monitors(context);
	free(context->fds);
	free(context);

	return 0;
}


static int
vfs_resize_fd_table(struct io_context *context, const int newSize)
{
	void *fds;
	int	status = B_OK;

	if (newSize <= 0 || newSize > MAX_FD_TABLE_SIZE)
		return EINVAL;

	mutex_lock(&context->io_mutex);

	if ((size_t)newSize < context->table_size) {
		// shrink the fd table
		int i;

		// Make sure none of the fds being dropped are in use
		for(i = context->table_size; i-- > newSize;) {
			if (context->fds[i]) {
				status = EBUSY;
				goto out;
			}
		}

		fds = malloc(sizeof(struct file_descriptor *) * newSize);
		if (fds == NULL) {
			status = ENOMEM;
			goto out;
		}

		memcpy(fds, context->fds, sizeof(struct file_descriptor *) * newSize);
	} else {
		// enlarge the fd table

		fds = malloc(sizeof(struct file_descriptor *) * newSize);
		if (fds == NULL) {
			status = ENOMEM;
			goto out;
		}

		// copy the fd array, and zero the additional slots
		memcpy(fds, context->fds, sizeof(void *) * context->table_size);
		memset((char *)fds + (sizeof(void *) * context->table_size), 0,
			sizeof(void *) * (newSize - context->table_size));
	}

	free(context->fds);
	context->fds = fds;
	context->table_size = newSize;

out:
	mutex_unlock(&context->io_mutex);
	return status;
}


int
vfs_getrlimit(int resource, struct rlimit * rlp)
{
	if (!rlp)
		return -1;

	switch (resource) {
		case RLIMIT_NOFILE:
		{
			struct io_context *ioctx = get_current_io_context(false);

			mutex_lock(&ioctx->io_mutex);

			rlp->rlim_cur = ioctx->table_size;
			rlp->rlim_max = MAX_FD_TABLE_SIZE;

			mutex_unlock(&ioctx->io_mutex);

			return 0;
		}

		default:
			return -1;
	}
}


int
vfs_setrlimit(int resource, const struct rlimit * rlp)
{
	if (!rlp)
		return -1;

	switch (resource) {
		case RLIMIT_NOFILE:
			return vfs_resize_fd_table(get_current_io_context(false), rlp->rlim_cur);

		default:
			return -1;
	}
}


#ifdef DEBUG

int
vfs_test(void)
{
	int fd;
	int err;

	dprintf("vfs_test() entry\n");

	fd = sys_open_dir("/");
	dprintf("fd = %d\n", fd);
	sys_close(fd);

	fd = sys_open_dir("/");
	dprintf("fd = %d\n", fd);

	sys_create_dir("/foo", 0755);
	sys_create_dir("/foo/bar", 0755);
	sys_create_dir("/foo/bar/gar", 0755);
	sys_create_dir("/foo/bar/tar", 0755);

#if 1
	fd = sys_open_dir("/foo/bar");
	if (fd < 0)
		panic("unable to open /foo/bar\n");

	{
		char buf[64];
		ssize_t len;

		sys_seek(fd, 0, SEEK_SET);
		for (;;) {
			len = sys_read(fd, buf, -1, sizeof(buf));
			if (len > 0)
				dprintf("readdir returned name = '%s'\n", buf);
			else {
				dprintf("readdir returned %s\n", strerror(len));
				break;
			}
		}
	}

	// do some unlink tests
	err = sys_unlink("/foo/bar");
	if (err == B_NO_ERROR)
		panic("unlink of full directory should not have passed\n");
	sys_unlink("/foo/bar/gar");
	sys_unlink("/foo/bar/tar");
	err = sys_unlink("/foo/bar");
	if (err != B_NO_ERROR)
		panic("unlink of empty directory should have worked\n");

	sys_create_dir("/test", 0755);
	sys_create_dir("/test", 0755);
	err = sys_mount("/test", NULL, "rootfs", NULL);
	if (err < 0)
		panic("failed mount test\n");

#endif
#if 1

	fd = sys_open_dir("/boot");
	sys_close(fd);

	fd = sys_open_dir("/boot");
	if (fd < 0)
		panic("unable to open dir /boot\n");
	{
		char buf[256];
		struct dirent *dirent = (struct dirent *)buf;
		ssize_t len;

		sys_rewind_dir(fd);
		for (;;) {
			len = sys_read_dir(fd, dirent, sizeof(buf), 1);
//			if(len < 0)
//				panic("readdir returned %Ld\n", (long long)len);
			if (len > 0)
				dprintf("sys_read returned name = '%s'\n", dirent->d_name);
			else {
				dprintf("sys_read returned %s\n", strerror(len));
				break;
			}
		}
	}
	sys_close(fd);

	fd = sys_open("/boot/kernel", O_RDONLY);
	if (fd < 0)
		panic("unable to open kernel file '/boot/kernel'\n");
	{
		char buf[64];
		ssize_t len;

		len = sys_read(fd, buf, 0, sizeof(buf));
		if (len < 0)
			panic("failed on read\n");
		dprintf("read returned %Ld\n", (long long)len);
	}
	sys_close(fd);
	{
		struct stat stat;

		err = sys_read_stat("/boot/kernel", &stat);
		if (err < 0)
			panic("err stating '/boot/kernel'\n");
		dprintf("stat results:\n");
		dprintf("\tvnid 0x%Lx\n\ttype %d\n\tsize 0x%Lx\n", stat.st_ino, stat.st_mode, stat.st_size);
	}

#endif
	dprintf("vfs_test() done\n");
	return 0;
}

#endif


int
vfs_bootstrap_all_filesystems(void)
{
	int err;

	// bootstrap the root filesystem
	bootstrap_rootfs();

	err = sys_mount("/", NULL, "rootfs", NULL);
	if (err < 0)
		panic("error mounting rootfs!\n");

	sys_setcwd(-1, "/");

	// bootstrap the bootfs
	bootstrap_bootfs();

	sys_create_dir("/boot", 0755);
	err = sys_mount("/boot", NULL, "bootfs", NULL);
	if (err < 0)
		panic("error mounting bootfs\n");

	// bootstrap the devfs
	bootstrap_devfs();

	sys_create_dir("/dev", 0755);
	err = sys_mount("/dev", NULL, "devfs", NULL);
	if (err < 0)
		panic("error mounting devfs\n");

	// bootstrap the pipefs
	bootstrap_pipefs();

	sys_create_dir("/pipe", 0755);
	err = sys_mount("/pipe", NULL, "pipefs", NULL);
	if (err < 0)
		panic("error mounting pipefs\n");

	return B_NO_ERROR;
}


int
vfs_register_filesystem(const char *name, struct fs_ops *ops)
{
	status_t status = B_OK;
	file_system *fs;

	if (name == NULL || *name == '\0' || ops == NULL)
		return B_BAD_VALUE;

	mutex_lock(&gFileSystemsMutex);

	fs = new_file_system(name, ops);
	if (fs == NULL)
		status = B_NO_MEMORY;

	mutex_unlock(&gFileSystemsMutex);
	return status;
}


int
vfs_init(kernel_args *ka)
{
	{
		struct vnode *v;
		gVnodeTable = hash_init(VNODE_HASH_TABLE_SIZE, (addr)&v->next - (addr)v,
			&vnode_compare, &vnode_hash);
		if (gVnodeTable == NULL)
			panic("vfs_init: error creating vnode hash table\n");
	}
	{
		struct fs_mount *mount;
		gMountsTable = hash_init(MOUNTS_HASH_TABLE_SIZE, (addr)&mount->next - (addr)mount,
			&mount_compare, &mount_hash);
		if (gMountsTable == NULL)
			panic("vfs_init: error creating mounts hash table\n");
	}

	node_monitor_init();

	gFileSystems = NULL;
	gRoot = NULL;

	if (mutex_init(&gFileSystemsMutex, "vfs_lock") < 0)
		panic("vfs_init: error allocating file systems lock\n");

	if (mutex_init(&gMountOpMutex, "vfs_mount_op_lock") < 0)
		panic("vfs_init: error allocating mount op lock\n");

	if (mutex_init(&gMountMutex, "vfs_mount_lock") < 0)
		panic("vfs_init: error allocating mount lock\n");

	if (mutex_init(&gVnodeMutex, "vfs_vnode_lock") < 0)
		panic("vfs_init: error allocating vnode lock\n");

	return 0;
}


//	#pragma mark -
//	The filetype-dependent implementations (fd_ops + open/create/rename/remove, ...)


/** Calls fs_open() on the given vnode and returns a new
 *	file descriptor for it
 */

static int
create_vnode(struct vnode *directory, const char *name, int openMode, int perms, bool kernel)
{
	struct vnode *vnode;
	fs_cookie cookie;
	vnode_id newID;
	int status;

	if (FS_CALL(directory, create) == NULL)
		return EROFS;

	status = FS_CALL(directory, create)(directory->mount->cookie, directory->private_node, name, openMode, perms, &cookie, &newID);
	if (status < B_OK)
		return status;

	mutex_lock(&gVnodeMutex);
	vnode = lookup_vnode(directory->mount_id, newID);
	mutex_unlock(&gVnodeMutex);

	if (vnode == NULL) {
		dprintf("vfs: fs_create() returned success but there is no vnode!");
		return EINVAL;
	}

	if ((status = get_new_fd(FDTYPE_FILE, vnode, cookie, openMode, kernel)) >= 0)
		return status;

	// something went wrong, clean up

	FS_CALL(vnode, close)(vnode->mount->cookie, vnode->private_node, cookie);
	FS_CALL(vnode, free_cookie)(vnode->mount->cookie, vnode->private_node, cookie);
	put_vnode(vnode);

	FS_CALL(directory, unlink)(directory->mount->cookie, directory->private_node, name);
	
	return status;
}


/** Calls fs_open() on the given vnode and returns a new
 *	file descriptor for it
 */

static int
open_vnode(struct vnode *vnode, int omode, bool kernel)
{
	fs_cookie cookie;
	int status;

	status = FS_CALL(vnode, open)(vnode->mount->cookie, vnode->private_node, omode, &cookie);
	if (status < 0)
		return status;

	status = get_new_fd(FDTYPE_FILE, vnode, cookie, omode, kernel);
	if (status < 0) {
		FS_CALL(vnode, close)(vnode->mount->cookie, vnode->private_node, cookie);
		FS_CALL(vnode, free_cookie)(vnode->mount->cookie, vnode->private_node, cookie);
	}
	return status;
}


/** Calls fs open_dir() on the given vnode and returns a new
 *	file descriptor for it
 */

static int
open_dir_vnode(struct vnode *vnode, bool kernel)
{
	fs_cookie cookie;
	int status;

	status = FS_CALL(vnode, open_dir)(vnode->mount->cookie, vnode->private_node, &cookie);
	if (status < B_OK)
		return status;

	// file is opened, create a fd
	status = get_new_fd(FDTYPE_DIR, vnode, cookie, 0, kernel);
	if (status >= 0)
		return status;

	FS_CALL(vnode, close_dir)(vnode->mount->cookie, vnode->private_node, cookie);
	FS_CALL(vnode, free_dir_cookie)(vnode->mount->cookie, vnode->private_node, cookie);

	return status;
}


/** Calls fs open_attr_dir() on the given vnode and returns a new
 *	file descriptor for it.
 *	Used by attr_dir_open(), and attr_dir_open_fd().
 */

static int
open_attr_dir_vnode(struct vnode *vnode, bool kernel)
{
	fs_cookie cookie;
	int status;

	status = FS_CALL(vnode, open_attr_dir)(vnode->mount->cookie, vnode->private_node, &cookie);
	if (status < 0)
		return status;

	// file is opened, create a fd
	status = get_new_fd(FDTYPE_ATTR_DIR, vnode, cookie, 0, kernel);
	if (status >= 0)
		return status;

	FS_CALL(vnode, close_attr_dir)(vnode->mount->cookie, vnode->private_node, cookie);
	FS_CALL(vnode, free_attr_dir_cookie)(vnode->mount->cookie, vnode->private_node, cookie);

	return status;
}


static int
file_create_entry_ref(mount_id mountID, vnode_id directoryID, const char *name, int openMode, int perms, bool kernel)
{
	struct vnode *directory;
	int status;

	FUNCTION(("file_create_entry_ref: name = '%s', omode %x, perms %d, kernel %d\n", name, openMode, perms, kernel));

	// get directory to put the new file in	
	status = get_vnode(mountID, directoryID, &directory, false);
	if (status < B_OK)
		return status;

	status = create_vnode(directory, name, openMode, perms, kernel);
	put_vnode(directory);

	return status;
}


static int
file_create(char *path, int openMode, int perms, bool kernel)
{
	char name[B_FILE_NAME_LENGTH];
	struct vnode *directory;
	int status;

	FUNCTION(("file_create: path '%s', omode %x, perms %d, kernel %d\n", path, openMode, perms, kernel));

	// get directory to put the new file in	
	status = path_to_dir_vnode(path, &directory, name, kernel);
	if (status < 0)
		return status;

	status = create_vnode(directory, name, openMode, perms, kernel);

	put_vnode(directory);
	return status;
}


static int
file_open_entry_ref(mount_id mountID, vnode_id directoryID, const char *name, int openMode, bool kernel)
{
	struct vnode *vnode;
	int status;

	if (name == NULL || *name == '\0')
		return B_BAD_VALUE;

	FUNCTION(("file_open_entry_ref()\n"));

	// get the vnode matching the entry_ref
	status = entry_ref_to_vnode(mountID, directoryID, name, &vnode);
	if (status < B_OK)
		return status;

	status = open_vnode(vnode, openMode, kernel);
	if (status < B_OK)
		put_vnode(vnode);

	return status;
}


static int
file_open(char *path, int omode, bool kernel)
{
	struct vnode *vnode = NULL;
	int status;

	FUNCTION(("file_open: entry. path = '%s', omode %d, kernel %d\n", path, omode, kernel));

	// get the vnode matching the path
	status = path_to_vnode(path, (omode & O_NOTRAVERSE) == 0, &vnode, kernel);
	if (status < B_OK)
		return status;

	status = open_vnode(vnode, omode, kernel);
	if (status < B_OK)
		put_vnode(vnode);
		
	return status;
}


static status_t
file_close(struct file_descriptor *descriptor)
{
	struct vnode *vnode = descriptor->u.vnode;

	FUNCTION(("file_close(descriptor = %p)\n", descriptor));

	if (FS_CALL(vnode, close))
		return FS_CALL(vnode, close)(vnode->mount->cookie, vnode->private_node, descriptor->cookie);

	return B_OK;
}


static void
file_free_fd(struct file_descriptor *descriptor)
{
	struct vnode *vnode = descriptor->u.vnode;

	if (vnode != NULL) {
		FS_CALL(vnode, free_cookie)(vnode->mount->cookie, vnode->private_node, descriptor->cookie);
		put_vnode(vnode);
	}
}


static ssize_t
file_read(struct file_descriptor *descriptor, off_t pos, void *buffer, size_t *length)
{
	struct vnode *vnode = descriptor->u.vnode;

	FUNCTION(("file_read: buf %p, pos %Ld, len %p = %ld\n", buffer, pos, length, *length));
	return FS_CALL(vnode, read)(vnode->mount->cookie, vnode->private_node, descriptor->cookie, pos, buffer, length);
}


static ssize_t
file_write(struct file_descriptor *descriptor, off_t pos, const void *buffer, size_t *length)
{
	struct vnode *vnode = descriptor->u.vnode;

	FUNCTION(("file_write: buf %p, pos %Ld, len %p\n", buffer, pos, length));
	return FS_CALL(vnode, write)(vnode->mount->cookie, vnode->private_node, descriptor->cookie, pos, buffer, length);
}


static off_t
file_seek(struct file_descriptor *descriptor, off_t pos, int seekType)
{
	off_t offset;

	switch (seekType) {
		case SEEK_SET:
			offset = 0;
			break;
		case SEEK_CUR:
			offset = descriptor->pos;
			break;
		case SEEK_END:
		{
			struct vnode *vnode = descriptor->u.vnode;
			struct stat stat;
			status_t status;

			if (FS_CALL(vnode, read_stat) == NULL)
				return EOPNOTSUPP;

			status = FS_CALL(vnode, read_stat)(vnode->mount->cookie, vnode->private_node, &stat);
			if (status < B_OK)
				return status;

			offset = stat.st_size;
			break;
		}
		default:
			return B_BAD_VALUE;
	}

	// assumes off_t is 64 bits wide
	if (offset > 0 && LONGLONG_MAX - offset < pos)
		return EOVERFLOW;

	pos += offset;
	if (pos < 0)
		return B_BAD_VALUE;

	return descriptor->pos = pos;
}


static int
dir_create_entry_ref(mount_id mountID, vnode_id parentID, const char *name, int perms, bool kernel)
{
	struct vnode *vnode;
	vnode_id newID;
	int status;

	if (name == NULL || *name == '\0')
		return B_BAD_VALUE;

	FUNCTION(("dir_create_entry_ref(dev = %ld, ino = %Ld, name = '%s', perms = %d)\n", mountID, parentID, name, perms));
	
	status = get_vnode(mountID, parentID, &vnode, kernel);
	if (status < B_OK)
		return status;

	if (FS_CALL(vnode, create_dir))
		status = FS_CALL(vnode, create_dir)(vnode->mount->cookie, vnode->private_node, name, perms, &newID);
	else
		status = EROFS;

	put_vnode(vnode);
	return status;
}


static int
dir_create(char *path, int perms, bool kernel)
{
	char filename[SYS_MAX_NAME_LEN];
	struct vnode *vnode;
	vnode_id newID;
	int status;

	FUNCTION(("dir_create: path '%s', perms %d, kernel %d\n", path, perms, kernel));

	status = path_to_dir_vnode(path, &vnode, filename, kernel);
	if (status < 0)
		return status;

	if (FS_CALL(vnode, create_dir))
		status = FS_CALL(vnode, create_dir)(vnode->mount->cookie, vnode->private_node, filename, perms, &newID);
	else
		status = EROFS;

	put_vnode(vnode);
	return status;
}


static int
dir_open_node_ref(mount_id mountID, vnode_id directoryID, bool kernel)
{
	struct vnode *vnode;
	int status;

	FUNCTION(("dir_open_entry_ref()\n"));

	// get the vnode matching the node_ref
	status = get_vnode(mountID, directoryID, &vnode, false);
	if (status < B_OK)
		return status;

	status = open_dir_vnode(vnode, kernel);
	if (status < B_OK)
		put_vnode(vnode);

	return status;
}


static int
dir_open_entry_ref(mount_id mountID, vnode_id parentID, const char *name, bool kernel)
{
	struct vnode *vnode;
	int status;

	FUNCTION(("dir_open_entry_ref()\n"));

	if (name == NULL || *name == '\0')
		return B_BAD_VALUE;

	// get the vnode matching the entry_ref
	status = entry_ref_to_vnode(mountID, parentID, name, &vnode);
	if (status < B_OK)
		return status;

	status = open_dir_vnode(vnode, kernel);
	if (status < B_OK)
		put_vnode(vnode);

	return status;
}


static int
dir_open(char *path, bool kernel)
{
	struct vnode *vnode;
	int status;

	FUNCTION(("dir_open: path = '%s', kernel %d\n", path, kernel));

	status = path_to_vnode(path, true, &vnode, kernel);
	if (status < B_OK)
		return status;

	status = open_dir_vnode(vnode, kernel);
	if (status < B_OK)
		put_vnode(vnode);

	return status;
}


static status_t
dir_close(struct file_descriptor *descriptor)
{
	struct vnode *vnode = descriptor->u.vnode;

	FUNCTION(("dir_close(descriptor = %p)\n", descriptor));

	if (FS_CALL(vnode, close_dir))
		return FS_CALL(vnode, close_dir)(vnode->mount->cookie, vnode->private_node, descriptor->cookie);

	return B_OK;
}


static void
dir_free_fd(struct file_descriptor *descriptor)
{
	struct vnode *vnode = descriptor->u.vnode;

	if (vnode != NULL) {
		FS_CALL(vnode, free_dir_cookie)(vnode->mount->cookie, vnode->private_node, descriptor->cookie);
		put_vnode(vnode);
	}
}


static status_t 
dir_read(struct file_descriptor *descriptor, struct dirent *buffer, size_t bufferSize, uint32 *_count)
{
	struct vnode *vnode = descriptor->u.vnode;

	if (FS_CALL(vnode, read_dir))
		return FS_CALL(vnode, read_dir)(vnode->mount->cookie,vnode->private_node,descriptor->cookie,buffer,bufferSize,_count);
	
	return EOPNOTSUPP;
}


static status_t 
dir_rewind(struct file_descriptor *descriptor)
{
	struct vnode *vnode = descriptor->u.vnode;

	if (FS_CALL(vnode, rewind_dir))
		return FS_CALL(vnode, rewind_dir)(vnode->mount->cookie,vnode->private_node,descriptor->cookie);

	return EOPNOTSUPP;
}


static status_t
dir_remove(char *path, bool kernel)
{
	char name[B_FILE_NAME_LENGTH];
	struct vnode *directory;
	status_t status;
	
	status = path_to_dir_vnode(path, &directory, name, kernel);
	if (status < B_OK)
		return status;

	if (FS_CALL(directory, remove_dir))
		status = FS_CALL(directory, remove_dir)(directory->mount->cookie, directory->private_node, name);
	else
		status = EROFS;

	put_vnode(directory);
	return status;
}


static status_t
common_ioctl(struct file_descriptor *descriptor, ulong op, void *buffer, size_t length)
{
	struct vnode *vnode = descriptor->u.vnode;

	if (FS_CALL(vnode, ioctl))
		return FS_CALL(vnode, ioctl)(vnode->mount->cookie,vnode->private_node,descriptor->cookie,op,buffer,length);

	return EOPNOTSUPP;
}


static status_t
common_sync(int fd, bool kernel)
{
	struct file_descriptor *descriptor;
	struct vnode *vnode;
	int status;

	FUNCTION(("vfs_fsync: entry. fd %d kernel %d\n", fd, kernel));

	descriptor = get_fd_and_vnode(fd, &vnode, kernel);
	if (descriptor == NULL)
		return B_FILE_ERROR;

	if (FS_CALL(vnode, fsync) != NULL)
		status = FS_CALL(vnode, fsync)(vnode->mount->cookie, vnode->private_node);
	else
		status = EOPNOTSUPP;

	put_fd(descriptor);
	return status;
}


static int
common_read_link(char *path, char *buffer, size_t bufferSize, bool kernel)
{
	struct vnode *vnode;
	int status;

	status = path_to_vnode(path, false, &vnode, kernel);
	if (status < B_OK)
		return status;

	if (FS_CALL(vnode, read_link) != NULL)
		status = FS_CALL(vnode, read_link)(vnode->mount->cookie, vnode->private_node, buffer, bufferSize);
	else
		status = B_BAD_VALUE;

	put_vnode(vnode);

	return status;
}


static status_t
common_write_link(char *path, char *toPath, bool kernel)
{
	struct vnode *vnode;
	int status;

	status = path_to_vnode(path, false, &vnode, kernel);
	if (status < B_OK)
		return status;

	if (FS_CALL(vnode, write_link) != NULL)
		status = FS_CALL(vnode, write_link)(vnode->mount->cookie, vnode->private_node, toPath);
	else
		status = EOPNOTSUPP;

	put_vnode(vnode);

	return status;
}


static status_t
common_create_symlink(char *path, const char *toPath, int mode, bool kernel)
{
	// path validity checks have to be in the calling function!
	char name[B_FILE_NAME_LENGTH];
	struct vnode *vnode;
	int status;

	FUNCTION(("common_create_symlink(path = %s, toPath = %s, mode = %d, kernel = %d)\n", path, toPath, mode, kernel));

	status = path_to_dir_vnode(path, &vnode, name, kernel);
	if (status < B_OK)
		return status;

	if (FS_CALL(vnode, create_symlink) != NULL)
		status = FS_CALL(vnode, create_symlink)(vnode->mount->cookie, vnode->private_node, name, toPath, mode);
	else
		status = EROFS;

	put_vnode(vnode);

	return status;
}


static status_t
common_create_link(char *path, char *toPath, bool kernel)
{
	// path validity checks have to be in the calling function!
	char name[B_FILE_NAME_LENGTH];
	struct vnode *directory, *vnode;
	int status;

	FUNCTION(("common_create_link(path = %s, toPath = %s, kernel = %d)\n", path, toPath, kernel));

	status = path_to_dir_vnode(path, &directory, name, kernel);
	if (status < B_OK)
		return status;

	status = path_to_vnode(toPath, true, &vnode, kernel);
	if (status < B_OK)
		goto err;

	if (directory->mount != vnode->mount) {
		status = B_CROSS_DEVICE_LINK;
		goto err1;
	}

	if (FS_CALL(vnode, link) != NULL)
		status = FS_CALL(vnode, link)(directory->mount->cookie, directory->private_node, name, vnode->private_node);
	else
		status = EROFS;

err1:
	put_vnode(vnode);
err:
	put_vnode(directory);

	return status;
}


static status_t
common_unlink(char *path, bool kernel)
{
	char filename[SYS_MAX_NAME_LEN];
	struct vnode *vnode;
	int status;

	FUNCTION(("common_unlink: path '%s', kernel %d\n", path, kernel));

	status = path_to_dir_vnode(path, &vnode, filename, kernel);
	if (status < 0)
		return status;

	if (FS_CALL(vnode, unlink) != NULL)
		status = FS_CALL(vnode, unlink)(vnode->mount->cookie, vnode->private_node, filename);
	else
		status = EROFS;

	put_vnode(vnode);

	return status;
}


static status_t
common_access(char *path, int mode, bool kernel)
{
	struct vnode *vnode;
	int status;

	status = path_to_vnode(path, true, &vnode, kernel);
	if (status < B_OK)
		return status;

	if (FS_CALL(vnode, access) != NULL)
		status = FS_CALL(vnode, access)(vnode->mount->cookie, vnode->private_node, mode);
	else
		status = EOPNOTSUPP;

	put_vnode(vnode);

	return status;
}


static status_t
common_rename(char *path, char *newPath, bool kernel)
{
	struct vnode *fromVnode, *toVnode;
	char fromName[SYS_MAX_NAME_LEN];
	char toName[SYS_MAX_NAME_LEN];
	int status;

	FUNCTION(("common_rename(path = %s, newPath = %s, kernel = %d)\n", path, newPath, kernel));

	status = path_to_dir_vnode(path, &fromVnode, fromName, kernel);
	if (status < 0)
		return status;

	status = path_to_dir_vnode(newPath, &toVnode, toName, kernel);
	if (status < 0)
		goto err;

	if (fromVnode->mount_id != toVnode->mount_id) {
		status = B_CROSS_DEVICE_LINK;
		goto err1;
	}

	if (FS_CALL(fromVnode, rename) != NULL)
		status = FS_CALL(fromVnode, rename)(fromVnode->mount->cookie, fromVnode->private_node, fromName, toVnode->private_node, toName);
	else
		status = EROFS;

err1:
	put_vnode(toVnode);
err:
	put_vnode(fromVnode);

	return status;
}


static status_t
common_read_stat(struct file_descriptor *descriptor, struct stat *stat)
{
	struct vnode *vnode = descriptor->u.vnode;

	FUNCTION(("common_read_stat: stat %p\n", stat));
	return FS_CALL(vnode, read_stat)(vnode->mount->cookie, vnode->private_node, stat);
}


static status_t
common_write_stat(struct file_descriptor *descriptor, const struct stat *stat, int statMask)
{
	struct vnode *vnode = descriptor->u.vnode;
	
	FUNCTION(("common_write_stat(vnode = %p, stat = %p, statMask = %d)\n", vnode, stat, statMask));
	if (!FS_CALL(vnode, write_stat))
		return EROFS;

	return FS_CALL(vnode, write_stat)(vnode->mount->cookie, vnode->private_node, stat, statMask);
}


static status_t
common_path_read_stat(char *path, bool traverseLeafLink, struct stat *stat, bool kernel)
{
	struct vnode *vnode;
	status_t status;

	FUNCTION(("common_path_read_stat: path '%s', stat %p,\n", path, stat));

	status = path_to_vnode(path, traverseLeafLink, &vnode, kernel);
	if (status < 0)
		return status;

	status = FS_CALL(vnode, read_stat)(vnode->mount->cookie, vnode->private_node, stat);

	put_vnode(vnode);
	return status;
}


static status_t
common_path_write_stat(char *path, bool traverseLeafLink, const struct stat *stat, int statMask, bool kernel)
{
	struct vnode *vnode;
	int status;

	FUNCTION(("common_write_stat: path '%s', stat %p, stat_mask %d, kernel %d\n", path, stat, statMask, kernel));

	status = path_to_vnode(path, traverseLeafLink, &vnode, kernel);
	if (status < 0)
		return status;

	if (FS_CALL(vnode, write_stat))
		status = FS_CALL(vnode, write_stat)(vnode->mount->cookie, vnode->private_node, stat, statMask);
	else
		status = EROFS;

	put_vnode(vnode);

	return status;
}


static status_t
attr_dir_open(int fd, char *path, bool kernel)
{
	struct vnode *vnode;
	int status;

	FUNCTION(("attr_dir_open(fd = %d, path = '%s', kernel = %d)\n", fd, path, kernel));

	status = fd_and_path_to_vnode(fd, path, true, &vnode, kernel);
	if (status < B_OK)
		return status;

	status = open_attr_dir_vnode(vnode, kernel);
	if (status < 0)
		put_vnode(vnode);

	return status;
}


static status_t
attr_dir_close(struct file_descriptor *descriptor)
{
	struct vnode *vnode = descriptor->u.vnode;

	FUNCTION(("dir_close(descriptor = %p)\n", descriptor));

	if (FS_CALL(vnode, close_attr_dir))
		return FS_CALL(vnode, close_attr_dir)(vnode->mount->cookie, vnode->private_node, descriptor->cookie);

	return B_OK;
}


static void
attr_dir_free_fd(struct file_descriptor *descriptor)
{
	struct vnode *vnode = descriptor->u.vnode;

	if (vnode != NULL) {
		FS_CALL(vnode, free_attr_dir_cookie)(vnode->mount->cookie, vnode->private_node, descriptor->cookie);
		put_vnode(vnode);
	}
}


static status_t 
attr_dir_read(struct file_descriptor *descriptor, struct dirent *buffer, size_t bufferSize, uint32 *_count)
{
	struct vnode *vnode = descriptor->u.vnode;

	if (FS_CALL(vnode, read_attr_dir))
		return FS_CALL(vnode, read_attr_dir)(vnode->mount->cookie, vnode->private_node, descriptor->cookie, buffer, bufferSize, _count);

	return EOPNOTSUPP;
}


static status_t 
attr_dir_rewind(struct file_descriptor *descriptor)
{
	struct vnode *vnode = descriptor->u.vnode;

	if (FS_CALL(vnode, rewind_attr_dir))
		return FS_CALL(vnode, rewind_attr_dir)(vnode->mount->cookie, vnode->private_node, descriptor->cookie);

	return EOPNOTSUPP;
}


static int
attr_create(int fd, const char *name, uint32 type, int openMode, bool kernel)
{
	struct vnode *vnode;
	fs_cookie cookie;
	int status;

	if (name == NULL || *name == '\0')
		return B_BAD_VALUE;

	vnode = get_vnode_from_fd(get_current_io_context(kernel), fd);
	if (vnode == NULL)
		return B_FILE_ERROR;

	if (FS_CALL(vnode, create_attr) == NULL) {
		status = EROFS;
		goto err;
	}

	status = FS_CALL(vnode, create_attr)(vnode->mount->cookie, vnode->private_node, name, type, openMode, &cookie);
	if (status < B_OK)
		goto err;

	if ((status = get_new_fd(FDTYPE_ATTR, vnode, cookie, openMode, kernel)) >= 0)
		return status;

	FS_CALL(vnode, close_attr)(vnode->mount->cookie, vnode->private_node, cookie);
	FS_CALL(vnode, free_attr_cookie)(vnode->mount->cookie, vnode->private_node, cookie);

	FS_CALL(vnode, remove_attr)(vnode->mount->cookie, vnode->private_node, name);

err:
	put_vnode(vnode);

	return status;
}


static int
attr_open(int fd, const char *name, int openMode, bool kernel)
{
	struct vnode *vnode;
	fs_cookie cookie;
	int status;

	if (name == NULL || *name == '\0')
		return B_BAD_VALUE;

	vnode = get_vnode_from_fd(get_current_io_context(kernel), fd);
	if (vnode == NULL)
		return B_FILE_ERROR;

	if (FS_CALL(vnode, open_attr) == NULL) {
		status = EOPNOTSUPP;
		goto err;
	}

	status = FS_CALL(vnode, open_attr)(vnode->mount->cookie, vnode->private_node, name, openMode, &cookie);
	if (status < B_OK)
		goto err;

	if ((status = get_new_fd(FDTYPE_ATTR, vnode, cookie, openMode, kernel)) >= 0)
		return status;

	FS_CALL(vnode, close_attr)(vnode->mount->cookie, vnode->private_node, cookie);
	FS_CALL(vnode, free_attr_cookie)(vnode->mount->cookie, vnode->private_node, cookie);

err:
	put_vnode(vnode);

	return status;
}


static status_t
attr_close(struct file_descriptor *descriptor)
{
	struct vnode *vnode = descriptor->u.vnode;

	FUNCTION(("attr_close(descriptor = %p)\n", descriptor));

	if (FS_CALL(vnode, close_attr))
		return FS_CALL(vnode, close_attr)(vnode->mount->cookie, vnode->private_node, descriptor->cookie);

	return B_OK;
}


static void
attr_free_fd(struct file_descriptor *descriptor)
{
	struct vnode *vnode = descriptor->u.vnode;

	if (vnode != NULL) {
		FS_CALL(vnode, free_attr_cookie)(vnode->mount->cookie, vnode->private_node, descriptor->cookie);
		put_vnode(vnode);
	}
}


static ssize_t
attr_read(struct file_descriptor *descriptor, off_t pos, void *buffer, size_t *length)
{
	struct vnode *vnode = descriptor->u.vnode;

	FUNCTION(("attr_read: buf %p, pos %Ld, len %p = %ld\n", buffer, pos, length, *length));
	if (!FS_CALL(vnode, read_attr))
		return EOPNOTSUPP;

	return FS_CALL(vnode, read_attr)(vnode->mount->cookie, vnode->private_node, descriptor->cookie, pos, buffer, length);
}


static ssize_t
attr_write(struct file_descriptor *descriptor, off_t pos, const void *buffer, size_t *length)
{
	struct vnode *vnode = descriptor->u.vnode;

	FUNCTION(("attr_write: buf %p, pos %Ld, len %p\n", buffer, pos, length));
	if (!FS_CALL(vnode, write_attr))
		return EOPNOTSUPP;

	return FS_CALL(vnode, write_attr)(vnode->mount->cookie, vnode->private_node, descriptor->cookie, pos, buffer, length);
}


static off_t
attr_seek(struct file_descriptor *descriptor, off_t pos, int seekType)
{
	off_t offset;

	switch (seekType) {
		case SEEK_SET:
			offset = 0;
			break;
		case SEEK_CUR:
			offset = descriptor->pos;
			break;
		case SEEK_END:
		{
			struct vnode *vnode = descriptor->u.vnode;
			struct stat stat;
			status_t status;

			if (FS_CALL(vnode, read_stat) == NULL)
				return EOPNOTSUPP;

			status = FS_CALL(vnode, read_attr_stat)(vnode->mount->cookie, vnode->private_node, descriptor->cookie, &stat);
			if (status < B_OK)
				return status;

			offset = stat.st_size;
			break;
		}
		default:
			return B_BAD_VALUE;
	}

	// assumes off_t is 64 bits wide
	if (offset > 0 && LONGLONG_MAX - offset < pos)
		return EOVERFLOW;

	pos += offset;
	if (pos < 0)
		return B_BAD_VALUE;

	return descriptor->pos = pos;
}


static status_t
attr_read_stat(struct file_descriptor *descriptor, struct stat *stat)
{
	struct vnode *vnode = descriptor->u.vnode;

	FUNCTION(("attr_read_stat: stat 0x%p\n", stat));
	if (!FS_CALL(vnode, read_attr_stat))
		return EOPNOTSUPP;

	return FS_CALL(vnode, read_attr_stat)(vnode->mount->cookie, vnode->private_node, descriptor->cookie, stat);
}


static status_t
attr_write_stat(struct file_descriptor *descriptor, const struct stat *stat, int statMask)
{
	struct vnode *vnode = descriptor->u.vnode;

	FUNCTION(("attr_write_stat: stat = %p, statMask %d\n", stat, statMask));

	if (!FS_CALL(vnode, write_attr_stat))
		return EROFS;

	return FS_CALL(vnode, write_attr_stat)(vnode->mount->cookie, vnode->private_node, descriptor->cookie, stat, statMask);
}


static status_t
attr_remove(int fd, const char *name, bool kernel)
{
	struct file_descriptor *descriptor;
	struct vnode *vnode;
	int status;

	if (name == NULL || *name == '\0')
		return B_BAD_VALUE;

	FUNCTION(("attr_remove: fd = %d, name = \"%s\", kernel %d\n", fd, name, kernel));

	descriptor = get_fd_and_vnode(fd, &vnode, kernel);
	if (descriptor == NULL)
		return B_FILE_ERROR;

	if (FS_CALL(vnode, remove_attr))
		status = FS_CALL(vnode, remove_attr)(vnode->mount->cookie, vnode->private_node, name);
	else
		status = EROFS;

	put_vnode(vnode);

	return status;
}


static status_t
attr_rename(int fromfd, const char *fromName, int tofd, const char *toName, bool kernel)
{
	struct file_descriptor *fromDescriptor, *toDescriptor;
	struct vnode *fromVnode, *toVnode;
	int status;

	if (fromName == NULL || *fromName == '\0' || toName == NULL || *toName == '\0')
		return B_BAD_VALUE;

	FUNCTION(("attr_rename: from fd = %d, from name = \"%s\", to fd = %d, to name = \"%s\", kernel %d\n", fromfd, fromName, tofd, toName, kernel));

	fromDescriptor = get_fd_and_vnode(fromfd, &fromVnode, kernel);
	if (fromDescriptor == NULL)
		return B_FILE_ERROR;

	toDescriptor = get_fd_and_vnode(tofd, &toVnode, kernel);
	if (toDescriptor == NULL) {
		status = B_FILE_ERROR;
		goto err;
	}

	// are the files on the same volume?
	if (fromVnode->mount_id != toVnode->mount_id) {
		status = B_CROSS_DEVICE_LINK;
		goto err1;
	}

	if (FS_CALL(fromVnode, rename_attr))
		status = FS_CALL(fromVnode, rename_attr)(fromVnode->mount->cookie, fromVnode->private_node, fromName, toVnode->private_node, toName);
	else
		status = EROFS;

err1:
	put_vnode(toVnode);
err:
	put_vnode(fromVnode);

	return status;
}


static status_t
index_dir_open(mount_id mountID, bool kernel)
{
	struct fs_mount *mount;
	fs_cookie cookie;
	status_t status;

	FUNCTION(("index_dir_open(mountID = %ld, kernel = %d)\n", mountID, kernel));

	mount = get_mount(mountID);
	if (mount == NULL)
		return B_BAD_VALUE;

	if (FS_MOUNT_CALL(mount, open_index_dir) == NULL) {
		status = EOPNOTSUPP;
		goto out;
	}

	status = FS_MOUNT_CALL(mount, open_index_dir)(mount->cookie, &cookie);
	if (status < B_OK)
		goto out;

	// get fd for the index directory
	status = get_new_fd(FDTYPE_INDEX_DIR, (void *)mount, cookie, 0, kernel);
	if (status >= 0)
		goto out;

	// something went wrong
	FS_MOUNT_CALL(mount, close_index_dir)(mount->cookie, cookie);
	FS_MOUNT_CALL(mount, free_index_dir_cookie)(mount->cookie, cookie);

out:
	put_mount(mount);
	return status;
}


static status_t
index_dir_close(struct file_descriptor *descriptor)
{
	struct fs_mount *mount = descriptor->u.mount;

	FUNCTION(("dir_close(descriptor = %p)\n", descriptor));

	if (FS_MOUNT_CALL(mount, close_index_dir))
		return FS_MOUNT_CALL(mount, close_index_dir)(mount->cookie, descriptor->cookie);

	return B_OK;
}


static void
index_dir_free_fd(struct file_descriptor *descriptor)
{
	struct fs_mount *mount = descriptor->u.mount;

	if (mount != NULL) {
		FS_MOUNT_CALL(mount, free_index_dir_cookie)(mount->cookie, descriptor->cookie);
		// ToDo: find a replacement ref_count object - perhaps the root dir?
		//put_vnode(vnode);
	}
}


static status_t 
index_dir_read(struct file_descriptor *descriptor, struct dirent *buffer, size_t bufferSize, uint32 *_count)
{
	struct fs_mount *mount = descriptor->u.mount;

	if (FS_MOUNT_CALL(mount, read_index_dir))
		return FS_MOUNT_CALL(mount, read_index_dir)(mount->cookie, descriptor->cookie, buffer, bufferSize, _count);

	return EOPNOTSUPP;
}


static status_t 
index_dir_rewind(struct file_descriptor *descriptor)
{
	struct fs_mount *mount = descriptor->u.mount;

	if (FS_MOUNT_CALL(mount, rewind_index_dir))
		return FS_MOUNT_CALL(mount, rewind_index_dir)(mount->cookie, descriptor->cookie);

	return EOPNOTSUPP;
}


static status_t
index_create(mount_id mountID, const char *name, uint32 type, uint32 flags, bool kernel)
{
	struct fs_mount *mount;
	status_t status;

	FUNCTION(("index_create(mountID = %ld, name = %s, kernel = %d)\n", mountID, name, kernel));

	mount = get_mount(mountID);
	if (mount == NULL)
		return B_BAD_VALUE;

	if (FS_MOUNT_CALL(mount, create_index) == NULL) {
		status = EROFS;
		goto out;
	}

	status = FS_MOUNT_CALL(mount, create_index)(mount->cookie, name, type, flags);

out:
	put_mount(mount);
	return status;
}


#if 0
static status_t
index_read_stat(struct file_descriptor *descriptor, struct stat *stat)
{
	struct vnode *vnode = descriptor->u.vnode;

	// ToDo: currently unused!
	FUNCTION(("index_read_stat: stat 0x%p\n", stat));
	if (!FS_CALL(vnode, read_index_stat))
		return EOPNOTSUPP;

	return EOPNOTSUPP;
	//return FS_CALL(vnode, read_index_stat)(vnode->mount->cookie, vnode->private_node, descriptor->cookie, stat);
}


static void
index_free_fd(struct file_descriptor *descriptor)
{
	struct vnode *vnode = descriptor->u.vnode;

	if (vnode != NULL) {
		FS_CALL(vnode, free_index_cookie)(vnode->mount->cookie, vnode->private_node, descriptor->cookie);
		put_vnode(vnode);
	}
}
#endif


static status_t
index_name_read_stat(mount_id mountID, const char *name, struct stat *stat, bool kernel)
{
	struct fs_mount *mount;
	status_t status;

	FUNCTION(("index_remove(mountID = %ld, name = %s, kernel = %d)\n", mountID, name, kernel));

	mount = get_mount(mountID);
	if (mount == NULL)
		return B_BAD_VALUE;

	if (FS_MOUNT_CALL(mount, read_index_stat) == NULL) {
		status = EOPNOTSUPP;
		goto out;
	}

	status = FS_MOUNT_CALL(mount, read_index_stat)(mount->cookie, name, stat);

out:
	put_mount(mount);
	return status;
}


static status_t
index_remove(mount_id mountID, const char *name, bool kernel)
{
	struct fs_mount *mount;
	status_t status;

	FUNCTION(("index_remove(mountID = %ld, name = %s, kernel = %d)\n", mountID, name, kernel));

	mount = get_mount(mountID);
	if (mount == NULL)
		return B_BAD_VALUE;

	if (FS_MOUNT_CALL(mount, remove_index) == NULL) {
		status = EROFS;
		goto out;
	}

	status = FS_MOUNT_CALL(mount, remove_index)(mount->cookie, name);

out:
	put_mount(mount);
	return status;
}


//	#pragma mark -
//	General File System functions


static status_t
fs_mount(char *path, const char *device, const char *fsName, void *args, bool kernel)
{
	struct fs_mount *mount;
	struct vnode *covered_vnode = NULL;
	vnode_id root_id;
	int err = 0;

	FUNCTION(("fs_mount: entry. path = '%s', fs_name = '%s'\n", path, fsName));

	// The path is always safe, we just have to make sure that fsName is
	// almost valid - we can't make any assumptions about device and args,
	// though.
	if (fsName == NULL || fsName[0] == '\0')
		return B_BAD_VALUE;

	mutex_lock(&gMountOpMutex);

	mount = (struct fs_mount *)malloc(sizeof(struct fs_mount));
	if (mount == NULL) {
		err = B_NO_MEMORY;
		goto err;
	}

	list_init_etc(&mount->vnodes, offsetof(struct vnode, mount_link));

	mount->mount_point = strdup(path);
	if (mount->mount_point == NULL) {
		err = B_NO_MEMORY;
		goto err1;
	}

	mount->fs = get_file_system(fsName);
	if (mount->fs == NULL) {
		err = ENODEV;
		goto err2;
	}

	recursive_lock_init(&mount->rlock, "mount rlock");
	mount->id = gNextMountID++;
	mount->unmounting = false;

	if (!gRoot) {
		// we haven't mounted anything yet
		if (strcmp(path, "/") != 0) {
			err = B_ERROR;
			goto err3;
		}

		err = FS_MOUNT_CALL(mount, mount)(mount->id, device, NULL, &mount->cookie, &root_id);
		if (err < 0) {
			// ToDo: why should we hide the error code from the file system here?
			//err = ERR_VFS_GENERAL;
			goto err3;
		}

		mount->covers_vnode = NULL; // this is the root mount
	} else {
		err = path_to_vnode(path, true, &covered_vnode, kernel);
		if (err < 0)
			goto err2;

		if (!covered_vnode) {
			err = B_ERROR;
			goto err2;
		}

		// XXX insert check to make sure covered_vnode is a DIR, or maybe it's okay for it not to be

		if (covered_vnode != gRoot
			&& covered_vnode->mount->root_vnode == covered_vnode) {
			err = ERR_VFS_ALREADY_MOUNTPOINT;
			goto err2;
		}

		mount->covers_vnode = covered_vnode;

		// mount it
		err = FS_MOUNT_CALL(mount, mount)(mount->id, device, NULL, &mount->cookie, &root_id);
		if (err < 0)
			goto err4;
	}

	mutex_lock(&gMountMutex);

	// insert mount struct into list
	hash_insert(gMountsTable, mount);

	mutex_unlock(&gMountMutex);

	err = get_vnode(mount->id, root_id, &mount->root_vnode, 0);
	if (err < 0)
		goto err5;

	// XXX may be a race here
	if (mount->covers_vnode)
		mount->covers_vnode->covered_by = mount->root_vnode;

	if (!gRoot)
		gRoot = mount->root_vnode;

	mutex_unlock(&gMountOpMutex);

	return 0;

err5:
	FS_MOUNT_CALL(mount, unmount)(mount->cookie);
err4:
	if (mount->covers_vnode)
		put_vnode(mount->covers_vnode);
err3:
	recursive_lock_destroy(&mount->rlock);
	put_file_system(mount->fs);
err2:
	free(mount->mount_point);
err1:
	free(mount);
err:
	mutex_unlock(&gMountOpMutex);

	return err;
}


static status_t
fs_unmount(char *path, bool kernel)
{
	struct fs_mount *mount;
	struct vnode *vnode;
	int err;

	FUNCTION(("vfs_unmount: entry. path = '%s', kernel %d\n", path, kernel));

	err = path_to_vnode(path, true, &vnode, kernel);
	if (err < 0)
		return ERR_VFS_PATH_NOT_FOUND;

	mutex_lock(&gMountOpMutex);

	mount = find_mount(vnode->mount_id);
	if (!mount)
		panic("vfs_unmount: find_mount() failed on root vnode @%p of mount\n", vnode);

	if (mount->root_vnode != vnode) {
		// not mountpoint
		put_vnode(vnode);
		err = ERR_VFS_NOT_MOUNTPOINT;
		goto err;
	}

	/* grab the vnode master mutex to keep someone from creating a vnode
	while we're figuring out if we can continue */
	mutex_lock(&gVnodeMutex);

	/* simulate the root vnode having it's refcount decremented */
	mount->root_vnode->ref_count -= 2;

	// cycle through the list of vnodes associated with this mount and
	// make sure all of them are not busy or have refs on them
	vnode = NULL;
	while ((vnode = list_get_next_item(&mount->vnodes, vnode)) != NULL) {
		if (vnode->busy || vnode->ref_count != 0) {
			mount->root_vnode->ref_count += 2;
			mutex_unlock(&gVnodeMutex);
			put_vnode(mount->root_vnode);

			err = EBUSY;
			goto err;
		}
	}

	/* we can safely continue, mark all of the vnodes busy and this mount
	structure in unmounting state */
	while ((vnode = list_get_next_item(&mount->vnodes, vnode)) != NULL) {
		if (vnode != mount->root_vnode)
			vnode->busy = true;
	}
	mount->unmounting = true;

	mutex_unlock(&gVnodeMutex);

	mount->covers_vnode->covered_by = NULL;
	put_vnode(mount->covers_vnode);

	/* release the ref on the root vnode twice */
	put_vnode(mount->root_vnode);
	put_vnode(mount->root_vnode);

	// ToDo: when full vnode cache in place, will need to force
	// a putvnode/removevnode here

	/* remove the mount structure from the hash table */
	mutex_lock(&gMountMutex);
	hash_remove(gMountsTable, mount);
	mutex_unlock(&gMountMutex);

	mutex_unlock(&gMountOpMutex);

	FS_MOUNT_CALL(mount, unmount)(mount->cookie);

	// release the file system
	put_file_system(mount->fs);

	free(mount->mount_point);
	free(mount);

	return 0;

err:
	mutex_unlock(&gMountOpMutex);
	return err;
}


static status_t
fs_sync(void)
{
	struct hash_iterator iter;
	struct fs_mount *mount;

	FUNCTION(("vfs_sync: entry.\n"));

	/* cycle through and call sync on each mounted fs */
	mutex_lock(&gMountOpMutex);
	mutex_lock(&gMountMutex);

	hash_open(gMountsTable, &iter);
	while ((mount = hash_next(gMountsTable, &iter))) {
		if (FS_MOUNT_CALL(mount, sync))
			FS_MOUNT_CALL(mount, sync)(mount->cookie);
	}
	hash_close(gMountsTable, &iter, false);

	mutex_unlock(&gMountMutex);
	mutex_unlock(&gMountOpMutex);

	return 0;
}


static status_t
fs_read_info(dev_t device, struct fs_info *info)
{
	struct fs_mount *mount;
	int status;

	mutex_lock(&gMountMutex);	

	mount = find_mount(device);
	if (mount == NULL) {
		status = EINVAL;
		goto out;
	}

	if (FS_MOUNT_CALL(mount, read_fs_info))
		status = FS_MOUNT_CALL(mount, read_fs_info)(mount->cookie, info);
	else
		status = EOPNOTSUPP;

	// fill in other info the file system doesn't (have to) know about
	info->dev = mount->id;
	info->root = mount->root_vnode->id;

out:
	mutex_unlock(&gMountMutex);
	return status;
}


static status_t
fs_write_info(dev_t device, const struct fs_info *info, int mask)
{
	struct fs_mount *mount;
	int status;

	mutex_lock(&gMountMutex);	

	mount = find_mount(device);
	if (mount == NULL) {
		status = EINVAL;
		goto out;
	}

	if (FS_MOUNT_CALL(mount, write_fs_info))
		status = FS_MOUNT_CALL(mount, write_fs_info)(mount->cookie, info, mask);
	else
		status = EROFS;

out:
	mutex_unlock(&gMountMutex);
	return status;
}


static status_t
get_cwd(char *buffer, size_t size, bool kernel)
{
	// Get current working directory from io context
	struct io_context *context = get_current_io_context(kernel);
	int status;

	FUNCTION(("vfs_get_cwd: buf %p, size %ld\n", buffer, size));

	mutex_lock(&context->io_mutex);

	if (context->cwd)
		status = dir_vnode_to_path(context->cwd, buffer, size);
	else
		status = B_ERROR;

	mutex_unlock(&context->io_mutex);
	return status; 
}


static status_t
set_cwd(int fd, char *path, bool kernel)
{
	struct io_context *context;
	struct vnode *vnode = NULL;
	struct vnode *oldDirectory;
	struct stat stat;
	int rc;

	FUNCTION(("set_cwd: path = \'%s\'\n", path));

	// Get vnode for passed path, and bail if it failed
	rc = fd_and_path_to_vnode(fd, path, true, &vnode, kernel);
	if (rc < 0)
		return rc;

	rc = FS_CALL(vnode, read_stat)(vnode->mount->cookie, vnode->private_node, &stat);
	if (rc < 0)
		goto err;

	if (!S_ISDIR(stat.st_mode)) {
		// nope, can't cwd to here
		rc = B_NOT_A_DIRECTORY;
		goto err;
	}

	// Get current io context and lock
	context = get_current_io_context(kernel);
	mutex_lock(&context->io_mutex);

	// save the old current working directory first
	oldDirectory = context->cwd;
	context->cwd = vnode;

	mutex_unlock(&context->io_mutex);

	if (oldDirectory)
		put_vnode(oldDirectory);

	return B_NO_ERROR;

err:
	put_vnode(vnode);
	return rc;
}


//	#pragma mark -
//	Calls from within the kernel


int
sys_mount(const char *path, const char *device, const char *fs_name, void *args)
{
	char pathBuffer[SYS_MAX_PATH_LEN + 1];
	strlcpy(pathBuffer, path, SYS_MAX_PATH_LEN - 1);

	return fs_mount(pathBuffer, device, fs_name, args, true);
}


int
sys_unmount(const char *path)
{
	char pathBuffer[SYS_MAX_PATH_LEN + 1];
	strlcpy(pathBuffer, path, SYS_MAX_PATH_LEN - 1);

	return fs_unmount(pathBuffer, true);
}


status_t
_kern_read_fs_info(dev_t device, struct fs_info *info)
{
	if (info == NULL)
		return B_BAD_VALUE;

	return fs_read_info(device, info);
}


status_t
_kern_write_fs_info(dev_t device, const struct fs_info *info, int mask)
{
	if (info == NULL)
		return B_BAD_VALUE;

	return fs_write_info(device, info, mask);
}


int
sys_sync(void)
{
	return fs_sync();
}


int
sys_open_entry_ref(dev_t device, ino_t inode, const char *name, int omode)
{
	char nameCopy[B_FILE_NAME_LENGTH];
	strlcpy(nameCopy, name, sizeof(nameCopy) - 1);

	return file_open_entry_ref(device, inode, nameCopy, omode, true);
}


int
sys_open(const char *path, int omode)
{
	char pathBuffer[SYS_MAX_PATH_LEN + 1];
	strlcpy(pathBuffer, path, SYS_MAX_PATH_LEN - 1);

	return file_open(pathBuffer, omode, true);
}


int
sys_open_dir_node_ref(dev_t device, ino_t inode)
{
	return dir_open_node_ref(device, inode, true);
}


int
sys_open_dir_entry_ref(dev_t device, ino_t inode, const char *name)
{
	return dir_open_entry_ref(device, inode, name, true);
}


int
sys_open_dir(const char *path)
{
	char pathBuffer[SYS_MAX_PATH_LEN + 1];
	strlcpy(pathBuffer, path, SYS_MAX_PATH_LEN - 1);

	return dir_open(pathBuffer, true);
}


int
sys_fsync(int fd)
{
	return common_sync(fd, true);
}


int
sys_create_entry_ref(dev_t device, ino_t inode, const char *name, int omode, int perms)
{
	return file_create_entry_ref(device, inode, name, omode, perms, true);
}


int
sys_create(const char *path, int omode, int perms)
{
	char buffer[SYS_MAX_PATH_LEN + 1];
	strlcpy(buffer, path, SYS_MAX_PATH_LEN - 1);

	return file_create(buffer, omode, perms, true);
}


int
sys_create_dir_entry_ref(dev_t device, ino_t inode, const char *name, int perms)
{
	return dir_create_entry_ref(device, inode, name, perms, true);
}


int
sys_create_dir(const char *path, int perms)
{
	char pathBuffer[SYS_MAX_PATH_LEN + 1];
	strlcpy(pathBuffer, path, SYS_MAX_PATH_LEN - 1);

	return dir_create(pathBuffer, perms, true);
}


int
sys_remove_dir(const char *path)
{
	char pathBuffer[SYS_MAX_PATH_LEN + 1];
	strlcpy(pathBuffer, path, SYS_MAX_PATH_LEN - 1);

	return dir_remove(pathBuffer, true);
}


int
sys_read_link(const char *path, char *buffer, size_t bufferSize)
{
	char pathBuffer[SYS_MAX_PATH_LEN + 1];
	strlcpy(pathBuffer, path, SYS_MAX_PATH_LEN - 1);

	return common_read_link(pathBuffer, buffer, bufferSize, true);
}


int
sys_write_link(const char *path, const char *toPath)
{
	char pathBuffer[SYS_MAX_PATH_LEN + 1];
	char toPathBuffer[SYS_MAX_PATH_LEN + 1];
	int status;

	strlcpy(pathBuffer, path, SYS_MAX_PATH_LEN - 1);
	strlcpy(toPathBuffer, toPath, SYS_MAX_PATH_LEN - 1);

	status = check_path(toPathBuffer);
	if (status < B_OK)
		return status;

	return common_write_link(pathBuffer, toPathBuffer, true);
}


int
sys_create_symlink(const char *path, const char *toPath, int mode)
{
	char pathBuffer[SYS_MAX_PATH_LEN + 1];
	char toPathBuffer[SYS_MAX_PATH_LEN + 1];
	int status;

	strlcpy(pathBuffer, path, SYS_MAX_PATH_LEN - 1);
	strlcpy(toPathBuffer, toPath, SYS_MAX_PATH_LEN - 1);

	status = check_path(toPathBuffer);
	if (status < B_OK)
		return status;

	return common_create_symlink(pathBuffer, toPathBuffer, mode, true);
}


int
sys_create_link(const char *path, const char *toPath)
{
	char pathBuffer[SYS_MAX_PATH_LEN + 1];
	char toPathBuffer[SYS_MAX_PATH_LEN + 1];
	
	strlcpy(pathBuffer, path, SYS_MAX_PATH_LEN - 1);
	strlcpy(toPathBuffer, toPath, SYS_MAX_PATH_LEN - 1);

	return common_create_link(pathBuffer, toPathBuffer, true);
}


int
sys_unlink(const char *path)
{
	char pathBuffer[SYS_MAX_PATH_LEN + 1];
	strlcpy(pathBuffer, path, SYS_MAX_PATH_LEN - 1);

	return common_unlink(pathBuffer, true);
}


int
sys_rename(const char *oldPath, const char *newPath)
{
	char oldPathBuffer[SYS_MAX_PATH_LEN + 1];
	char newPathBuffer[SYS_MAX_PATH_LEN + 1];

	strlcpy(oldPathBuffer, oldPath, SYS_MAX_PATH_LEN - 1);
	strlcpy(newPathBuffer, newPath, SYS_MAX_PATH_LEN - 1);

	return common_rename(oldPathBuffer, newPathBuffer, true);
}


int
sys_access(const char *path, int mode)
{
	char pathBuffer[SYS_MAX_PATH_LEN + 1];
	strlcpy(pathBuffer, path, SYS_MAX_PATH_LEN - 1);

	return common_access(pathBuffer, mode, true);
}


int
sys_read_path_stat(const char *path, bool traverseLeafLink, struct stat *stat)
{
	char pathBuffer[SYS_MAX_PATH_LEN + 1];
	strlcpy(pathBuffer, path, SYS_MAX_PATH_LEN - 1);

	return common_path_read_stat(pathBuffer, traverseLeafLink, stat, true);
}


int
sys_write_path_stat(const char *path, bool traverseLeafLink, const struct stat *stat, int statMask)
{
	char pathBuffer[SYS_MAX_PATH_LEN + 1];
	strlcpy(pathBuffer, path, SYS_MAX_PATH_LEN - 1);

	return common_path_write_stat(pathBuffer, traverseLeafLink, stat, statMask, true);
}


int
sys_open_attr_dir(int fd, const char *path)
{
	char pathBuffer[SYS_MAX_PATH_LEN + 1];

	if (fd == -1)
		strlcpy(pathBuffer, path, SYS_MAX_PATH_LEN - 1);

	return attr_dir_open(fd, pathBuffer, true);
}


int
sys_create_attr(int fd, const char *name, uint32 type, int openMode)
{
	return attr_create(fd, name, type, openMode, true);
}


int
sys_open_attr(int fd, const char *name, int openMode)
{
	return attr_open(fd, name, openMode, true);
}


int
sys_remove_attr(int fd, const char *name)
{
	return attr_remove(fd, name, true);
}


int
sys_rename_attr(int fromFile, const char *fromName, int toFile, const char *toName)
{
	return attr_rename(fromFile, fromName, toFile, toName, true);
}


int
sys_open_index_dir(dev_t device)
{
	return index_dir_open(device, true);
}


int
sys_create_index(dev_t device, const char *name, uint32 type, uint32 flags)
{
	return index_create(device, name, type, flags, true);
}


int
sys_read_index_stat(dev_t device, const char *name, struct stat *stat)
{
	return index_name_read_stat(device, name, stat, true);
}


int
sys_remove_index(dev_t device, const char *name)
{
	return index_remove(device, name, true);
}


int
sys_getcwd(char *buffer, size_t size)
{
	char path[SYS_MAX_PATH_LEN];
	int status;

	PRINT(("sys_getcwd: buf %p, %ld\n", buffer, size));

	// Call vfs to get current working directory
	status = get_cwd(path, SYS_MAX_PATH_LEN - 1, true);
	if (status < 0)
		return status;

	path[SYS_MAX_PATH_LEN - 1] = '\0';
	strlcpy(buffer, path, size);

	return status;
}


int
sys_setcwd(int fd, const char *path)
{
	char pathBuffer[SYS_MAX_PATH_LEN + 1];

	if (fd == -1)
		strlcpy(pathBuffer, path, SYS_MAX_PATH_LEN - 1);

	return set_cwd(fd, pathBuffer, true);
}


//	#pragma mark -
//	Calls from userland (with extra address checks)


int
user_mount(const char *upath, const char *udevice, const char *ufs_name, void *args)
{
	char path[SYS_MAX_PATH_LEN + 1];
	char fs_name[SYS_MAX_OS_NAME_LEN + 1];
	char device[SYS_MAX_PATH_LEN + 1];
	int rc;

	if (!CHECK_USER_ADDRESS(upath)
		|| !CHECK_USER_ADDRESS(ufs_name)
		|| !CHECK_USER_ADDRESS(udevice))
		return B_BAD_ADDRESS;

	rc = user_strlcpy(path, upath, SYS_MAX_PATH_LEN);
	if (rc < 0)
		return rc;

	rc = user_strlcpy(fs_name, ufs_name, SYS_MAX_OS_NAME_LEN);
	if (rc < 0)
		return rc;

	if (udevice) {
		rc = user_strlcpy(device, udevice, SYS_MAX_PATH_LEN);
		if (rc < 0)
			return rc;
	} else
		device[0] = '\0';

	return fs_mount(path, device, fs_name, args, false);
}


int
user_unmount(const char *userPath)
{
	char path[SYS_MAX_PATH_LEN + 1];
	int status;

	status = user_strlcpy(path, userPath, SYS_MAX_PATH_LEN);
	if (status < 0)
		return status;

	return fs_unmount(path, false);
}


status_t
_user_read_fs_info(dev_t device, struct fs_info *userInfo)
{
	struct fs_info info;
	status_t status;

	if (userInfo == NULL)
		return B_BAD_VALUE;

	if (!CHECK_USER_ADDRESS(userInfo))
		return B_BAD_ADDRESS;

	status = fs_read_info(device, &info);
	if (status != B_OK)
		return status;

	if (user_memcpy(userInfo, &info, sizeof(struct fs_info)) < B_OK)
		return B_BAD_ADDRESS;

	return B_OK;
}


status_t
_user_write_fs_info(dev_t device, const struct fs_info *userInfo, int mask)
{
	struct fs_info info;

	if (userInfo == NULL)
		return B_BAD_VALUE;

	if (!CHECK_USER_ADDRESS(userInfo)
		|| user_memcpy(&info, userInfo, sizeof(struct fs_info)) < B_OK)
		return B_BAD_ADDRESS;

	return fs_write_info(device, &info, mask);
}


int
user_sync(void)
{
	return fs_sync();
}


int
user_open_entry_ref(dev_t device, ino_t inode, const char *userName, int omode)
{
	char name[B_FILE_NAME_LENGTH];
	int status;

	if (!CHECK_USER_ADDRESS(userName))
		return B_BAD_ADDRESS;

	status = user_strlcpy(name, userName, sizeof(name) - 1);
	if (status < B_OK)
		return status;

	return file_open_entry_ref(device, inode, name, omode, false);
}


int
user_open(const char *userPath, int omode)
{
	char path[SYS_MAX_PATH_LEN + 1];
	int status;

	if (!CHECK_USER_ADDRESS(userPath))
		return B_BAD_ADDRESS;

	status = user_strlcpy(path, userPath, SYS_MAX_PATH_LEN);
	if (status < 0)
		return status;

	return file_open(path, omode, false);
}


int
user_open_dir_node_ref(dev_t device, ino_t inode)
{
	return dir_open_node_ref(device, inode, false);
}


int
user_open_dir_entry_ref(dev_t device, ino_t inode, const char *uname)
{
	char name[B_FILE_NAME_LENGTH];
	int status;

	if (!CHECK_USER_ADDRESS(uname))
		return B_BAD_ADDRESS;

	status = user_strlcpy(name, uname, sizeof(name));
	if (status < B_OK)
		return status;

	return dir_open_entry_ref(device, inode, name, false);
}


int
user_open_dir(const char *userPath)
{
	char path[SYS_MAX_PATH_LEN + 1];
	int status;

	if (!CHECK_USER_ADDRESS(userPath))
		return B_BAD_ADDRESS;

	status = user_strlcpy(path, userPath, SYS_MAX_PATH_LEN);
	if (status < 0)
		return status;

	return dir_open(path, false);
}


int
user_fsync(int fd)
{
	return common_sync(fd, false);
}


int
user_create_entry_ref(dev_t device, ino_t inode, const char *userName, int openMode, int perms)
{
	char name[B_FILE_NAME_LENGTH];
	int status;

	if (!CHECK_USER_ADDRESS(userName))
		return B_BAD_ADDRESS;

	status = user_strlcpy(name, userName, sizeof(name));
	if (status < 0)
		return status;

	return file_create_entry_ref(device, inode, name, openMode, perms, false);
}


int
user_create(const char *userPath, int openMode, int perms)
{
	char path[SYS_MAX_PATH_LEN + 1];
	int status;

	if (!CHECK_USER_ADDRESS(userPath))
		return B_BAD_ADDRESS;

	status = user_strlcpy(path, userPath, SYS_MAX_PATH_LEN);
	if (status < 0)
		return status;

	return file_create(path, openMode, perms, false);
}


int
user_create_dir_entry_ref(dev_t device, ino_t inode, const char *userName, int perms)
{
	char name[B_FILE_NAME_LENGTH];
	int status;

	if (!CHECK_USER_ADDRESS(userName))
		return B_BAD_ADDRESS;

	status = user_strlcpy(name, userName, sizeof(name));
	if (status < 0)
		return status;

	return dir_create_entry_ref(device, inode, name, perms, false);
}


int
user_create_dir(const char *userPath, int perms)
{
	char path[SYS_MAX_PATH_LEN + 1];
	int status;

	if (!CHECK_USER_ADDRESS(userPath))
		return B_BAD_ADDRESS;

	status = user_strlcpy(path, userPath, SYS_MAX_PATH_LEN);
	if (status < 0)
		return status;

	return dir_create(path, perms, false);
}


int
user_remove_dir(const char *userPath)
{
	char path[SYS_MAX_PATH_LEN + 1];
	int status;

	if (!CHECK_USER_ADDRESS(userPath))
		return B_BAD_ADDRESS;

	status = user_strlcpy(path, userPath, SYS_MAX_PATH_LEN);
	if (status < 0)
		return status;

	return dir_remove(path, false);
}


int
user_read_link(const char *userPath, char *userBuffer, size_t bufferSize)
{
	char path[SYS_MAX_PATH_LEN + 1];
	char buffer[SYS_MAX_PATH_LEN + 1];
	int status;

	if (!CHECK_USER_ADDRESS(userPath)
		|| !CHECK_USER_ADDRESS(userBuffer))
		return B_BAD_ADDRESS;

	status = user_strlcpy(path, userPath, SYS_MAX_PATH_LEN);
	if (status < 0)
		return status;

	if (bufferSize > SYS_MAX_PATH_LEN)
		bufferSize = SYS_MAX_PATH_LEN;

	status = common_read_link(path, buffer, bufferSize, false);
	if (status < B_OK)
		return status;

	// ToDo: think about buffer length and the return value at read_link()
	status = user_strlcpy(userBuffer, buffer, bufferSize);
	if (status >= 0)
		status = B_OK;
	return status;
}


int
user_write_link(const char *userPath, const char *userToPath)
{
	char path[SYS_MAX_PATH_LEN + 1];
	char toPath[SYS_MAX_PATH_LEN + 1];
	int status;
	
	if (!CHECK_USER_ADDRESS(userPath)
		|| !CHECK_USER_ADDRESS(userToPath))
		return B_BAD_ADDRESS;

	status = user_strlcpy(path, userPath, SYS_MAX_PATH_LEN);
	if (status < 0)
		return status;

	status = user_strlcpy(toPath, userToPath, SYS_MAX_PATH_LEN);
	if (status < 0)
		return status;

	status = check_path(toPath);
	if (status < B_OK)
		return status;

	return common_write_link(path, toPath, false);
}


int
user_create_symlink(const char *userPath, const char *userToPath, int mode)
{
	char path[SYS_MAX_PATH_LEN + 1];
	char toPath[SYS_MAX_PATH_LEN + 1];
	int status;
	
	if (!CHECK_USER_ADDRESS(userPath)
		|| !CHECK_USER_ADDRESS(userToPath))
		return B_BAD_ADDRESS;

	status = user_strlcpy(path, userPath, SYS_MAX_PATH_LEN);
	if (status < 0)
		return status;

	status = user_strlcpy(toPath, userToPath, SYS_MAX_PATH_LEN);
	if (status < 0)
		return status;

	status = check_path(toPath);
	if (status < B_OK)
		return status;

	return common_create_symlink(path, toPath, mode, false);
}


int
user_create_link(const char *userPath, const char *userToPath)
{
	char path[SYS_MAX_PATH_LEN + 1];
	char toPath[SYS_MAX_PATH_LEN + 1];
	int status;

	if (!CHECK_USER_ADDRESS(userPath)
		|| !CHECK_USER_ADDRESS(userToPath))
		return B_BAD_ADDRESS;

	status = user_strlcpy(path, userPath, SYS_MAX_PATH_LEN);
	if (status < 0)
		return status;

	status = user_strlcpy(toPath, userToPath, SYS_MAX_PATH_LEN);
	if (status < 0)
		return status;

	status = check_path(toPath);
	if (status < B_OK)
		return status;

	return common_create_link(path, toPath, false);
}


int
user_unlink(const char *userPath)
{
	char path[SYS_MAX_PATH_LEN + 1];
	int status;

	if (!CHECK_USER_ADDRESS(userPath))
		return B_BAD_ADDRESS;

	status = user_strlcpy(path, userPath, SYS_MAX_PATH_LEN);
	if (status < 0)
		return status;

	return common_unlink(path, false);
}


int
user_rename(const char *userOldPath, const char *userNewPath)
{
	char oldPath[SYS_MAX_PATH_LEN + 1];
	char newPath[SYS_MAX_PATH_LEN + 1];
	int status;

	if (!CHECK_USER_ADDRESS(userOldPath) || !CHECK_USER_ADDRESS(userNewPath))
		return B_BAD_ADDRESS;

	status = user_strlcpy(oldPath, userOldPath, SYS_MAX_PATH_LEN);
	if (status < 0)
		return status;

	status = user_strlcpy(newPath, userNewPath, SYS_MAX_PATH_LEN);
	if (status < 0)
		return status;

	return common_rename(oldPath, newPath, false);
}


int
user_access(const char *userPath, int mode)
{
	char path[SYS_MAX_PATH_LEN + 1];
	int status;

	if (!CHECK_USER_ADDRESS(userPath))
		return B_BAD_ADDRESS;

	status = user_strlcpy(path, userPath, SYS_MAX_PATH_LEN);
	if (status < 0)
		return status;

	return common_access(path, mode, false);
}


int
user_read_path_stat(const char *userPath, bool traverseLink, struct stat *userStat)
{
	char path[SYS_MAX_PATH_LEN + 1];
	struct stat stat;
	int status;

	if (!CHECK_USER_ADDRESS(userPath)
		|| !CHECK_USER_ADDRESS(userStat)
		|| user_strlcpy(path, userPath, SYS_MAX_PATH_LEN) < B_OK)
		return B_BAD_ADDRESS;

	status = common_path_read_stat(path, traverseLink, &stat, false);
	if (status < 0)
		return status;

	return user_memcpy(userStat, &stat, sizeof(struct stat));
}


int
user_write_path_stat(const char *userPath, bool traverseLeafLink, const struct stat *userStat, int statMask)
{
	char path[SYS_MAX_PATH_LEN + 1];
	struct stat stat;

	if (!CHECK_USER_ADDRESS(userStat)
		|| !CHECK_USER_ADDRESS(userPath)
		|| user_strlcpy(path, userPath, SYS_MAX_PATH_LEN) < B_OK
		|| user_memcpy(&stat, userStat, sizeof(struct stat)) < B_OK)
		return B_BAD_ADDRESS;

	return common_path_write_stat(path, traverseLeafLink, &stat, statMask, false);
}


int
user_open_attr_dir(int fd, const char *userPath)
{
	char pathBuffer[SYS_MAX_PATH_LEN + 1];

	if (fd == -1) {
		if (!CHECK_USER_ADDRESS(userPath)
			|| user_strlcpy(pathBuffer, userPath, SYS_MAX_PATH_LEN) < B_OK)
			return B_BAD_ADDRESS;
	}

	return attr_dir_open(fd, pathBuffer, false);
}


int
user_create_attr(int fd, const char *userName, uint32 type, int openMode)
{
	char name[B_FILE_NAME_LENGTH];

	if (!CHECK_USER_ADDRESS(userName)
		|| user_strlcpy(name, userName, B_FILE_NAME_LENGTH) < B_OK)
		return B_BAD_ADDRESS;

	return attr_create(fd, name, type, openMode, false);
}


int
user_open_attr(int fd, const char *userName, int openMode)
{
	char name[B_FILE_NAME_LENGTH];

	if (!CHECK_USER_ADDRESS(userName)
		|| user_strlcpy(name, userName, B_FILE_NAME_LENGTH) < B_OK)
		return B_BAD_ADDRESS;

	return attr_open(fd, name, openMode, false);
}


int
user_remove_attr(int fd, const char *userName)
{
	char name[B_FILE_NAME_LENGTH];

	if (!CHECK_USER_ADDRESS(userName)
		|| user_strlcpy(name, userName, B_FILE_NAME_LENGTH) < B_OK)
		return B_BAD_ADDRESS;

	return attr_remove(fd, name, false);
}


int
user_rename_attr(int fromFile, const char *userFromName, int toFile, const char *userToName)
{
	char fromName[B_FILE_NAME_LENGTH];
	char toName[B_FILE_NAME_LENGTH];

	if (!CHECK_USER_ADDRESS(userFromName)
		|| !CHECK_USER_ADDRESS(userToName))
		return B_BAD_ADDRESS;

	if (user_strlcpy(fromName, userFromName, B_FILE_NAME_LENGTH) < B_OK
		|| user_strlcpy(toName, userToName, B_FILE_NAME_LENGTH) < B_OK)
		return B_BAD_ADDRESS;

	return attr_rename(fromFile, fromName, toFile, toName, false);
}


int
user_open_index_dir(dev_t device)
{
	return index_dir_open(device, false);
}


int
user_create_index(dev_t device, const char *userName, uint32 type, uint32 flags)
{
	char name[B_FILE_NAME_LENGTH];
	
	if (!CHECK_USER_ADDRESS(userName)
		|| user_strlcpy(name, userName, B_FILE_NAME_LENGTH) < B_OK)
		return B_BAD_ADDRESS;

	return index_create(device, name, type, flags, false);
}


int
user_read_index_stat(dev_t device, const char *userName, struct stat *userStat)
{
	char name[B_FILE_NAME_LENGTH];
	struct stat stat;
	status_t status;

	if (!CHECK_USER_ADDRESS(userName)
		|| !CHECK_USER_ADDRESS(userStat)
		|| user_strlcpy(name, userName, B_FILE_NAME_LENGTH) < B_OK)
		return B_BAD_ADDRESS;

	status = index_name_read_stat(device, name, &stat, false);
	if (status == B_OK) {
		if (user_memcpy(userStat, &stat, sizeof(stat)) < B_OK)
			return B_BAD_ADDRESS;
	}

	return status;
}


int
user_remove_index(dev_t device, const char *userName)
{
	char name[B_FILE_NAME_LENGTH];
	
	if (!CHECK_USER_ADDRESS(userName)
		|| user_strlcpy(name, userName, B_FILE_NAME_LENGTH) < B_OK)
		return B_BAD_ADDRESS;

	return index_remove(device, name, false);
}


int
user_getcwd(char *userBuffer, size_t size)
{
	char buffer[SYS_MAX_PATH_LEN];
	int status;

	PRINT(("user_getcwd: buf %p, %ld\n", userBuffer, size));

	if (!CHECK_USER_ADDRESS(userBuffer))
		return B_BAD_ADDRESS;

	if (size > SYS_MAX_PATH_LEN)
		size = SYS_MAX_PATH_LEN;

	status = get_cwd(buffer, size, false);
	if (status < 0)
		return status;

	// Copy back the result
	if (user_strlcpy(userBuffer, buffer, size) < B_OK)
		return B_BAD_ADDRESS;

	return status;
}


int
user_setcwd(int fd, const char *userPath)
{
	char path[SYS_MAX_PATH_LEN];

	PRINT(("user_setcwd: path = %p\n", userPath));

	if (fd == -1) {
		if (!CHECK_USER_ADDRESS(userPath)
			|| user_strlcpy(path, userPath, SYS_MAX_PATH_LEN) < B_OK)
			return B_BAD_ADDRESS;
	}

	return set_cwd(fd, path, false);
}

