/* Virtual File System and
** File System Interface Layer
*/

/* 
** Copyright 2002, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <kernel.h>
#include <stage2.h>
#include <vfs.h>
#include <vm.h>
#include <vm_cache.h>
#include <debug.h>
#include <khash.h>
#include <lock.h>
#include <thread.h>
#include <memheap.h>
#include <arch/cpu.h>
#include <elf.h>
#include <rootfs.h>
#include <bootfs.h>
#include <devfs.h>
#include <Errors.h>
#include <kerrors.h>
#include <atomic.h>
#include <fd.h>

#include <OS.h>

#include <rootfs.h>

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <resource.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <StorageDefs.h>
#include <fs_info.h>

#ifndef TRACE_VFS
#	define TRACE_VFS 1
#endif
#if TRACE_VFS
#	define PRINT(x) dprintf x
#	define FUNCTION(x) dprintf x
#else
#	define PRINT(x) ;
#	define FUNCTION(x) ;
#endif

#define MAX_SYM_LINKS SYMLINKS_MAX

// Passed in buffers from user-space shouldn't be in the kernel
#define CHECK_USER_ADDRESS(x) \
	((addr)(x) < KERNEL_BASE || (addr)(x) > KERNEL_TOP)

struct vnode {
	struct vnode    *next;
	struct vnode    *mount_prev;
	struct vnode    *mount_next;
	struct vm_cache *cache;
	fs_id            fs_id;
	vnode_id         id;
	fs_vnode         private_node;
	struct fs_mount *mount;
	struct vnode    *covered_by;
	int              ref_count;
	bool             delete_me;
	bool             busy;
};

struct vnode_hash_key {
	fs_id      fs_id;
	vnode_id   vnode_id;
};

struct fs_container {
	struct fs_container *next;
	struct fs_calls *calls;
	const char *name;
};
#define FS_CALL(vnode,call) (vnode->mount->fs->calls->call)
static struct fs_container *fs_list;

struct fs_mount {
	struct fs_mount     *next;
	struct fs_container *fs;
	fs_id                id;
	void                *cookie;
	char                *mount_point;
	recursive_lock       rlock;
	struct vnode        *root_vnode;
	struct vnode        *covers_vnode;
	struct vnode        *vnodes_head;
	struct vnode        *vnodes_tail;
	bool                 unmounting;
};

static mutex gRegisterMutex;
static mutex gMountMutex;
static mutex gMountOpMutex;
static mutex gVnodeMutex;

/* function declarations */
static int vfs_mount(char *path, const char *device, const char *fs_name, void *args, bool kernel);
static int vfs_unmount(char *path, bool kernel);

static ssize_t vfs_read(struct file_descriptor *, void *, off_t, size_t *);
static ssize_t vfs_write(struct file_descriptor *, const void *, off_t, size_t *);
static int vfs_ioctl(struct file_descriptor *, ulong, void *buf, size_t len);
static status_t vfs_read_dir(struct file_descriptor *,struct dirent *buffer,size_t bufferSize,uint32 *_count);
static status_t vfs_rewind_dir(struct file_descriptor *);
static int vfs_read_stat(struct file_descriptor *, struct stat *);
static int vfs_close(struct file_descriptor *, int, struct io_context *);

static int common_ioctl(struct file_descriptor *, ulong, void *buf, size_t len);
static int common_read_stat(struct file_descriptor *, struct stat *);

static ssize_t file_read(struct file_descriptor *, off_t pos, void *buffer, size_t *);
static ssize_t file_write(struct file_descriptor *, off_t pos, const void *buffer, size_t *);
static off_t file_seek(struct file_descriptor *, off_t pos, int seek_type);
static status_t dir_read(struct file_descriptor *,struct dirent *buffer,size_t bufferSize,uint32 *_count);
static status_t dir_rewind(struct file_descriptor *);
static void file_free_fd(struct file_descriptor *);
static int file_close(struct file_descriptor *);
static void dir_free_fd(struct file_descriptor *);
static int dir_close(struct file_descriptor *);

static int vfs_open(char *path, int omode, bool kernel);
static int vfs_open_dir(char *path, bool kernel);
static int vfs_create(char *path, int omode, int perms, bool kernel);
static int vfs_create_dir(char *path, int perms, bool kernel);

static status_t dir_vnode_to_path(struct vnode *vnode, char *buffer, size_t bufferSize);

struct fd_ops file_ops = {
	"file",
	file_read,
	file_write,
	file_seek,
	common_ioctl,
	NULL,
	NULL,
	common_read_stat,
	file_close,
	file_free_fd
};

struct fd_ops dir_ops = {
	"directory",
	NULL,
	NULL,
	NULL,
	common_ioctl,
	dir_read,
	dir_rewind,
	common_read_stat,
	dir_close,
	dir_free_fd
};

#define VNODE_HASH_TABLE_SIZE 1024
static void *vnode_table;
static struct vnode *root_vnode;
static vnode_id next_vnode_id = 0;

#define MOUNTS_HASH_TABLE_SIZE 16
static void *mounts_table;
static fs_id next_fsid = 0;


static int
mount_compare(void *_m, const void *_key)
{
	struct fs_mount *mount = _m;
	const fs_id *id = _key;

	if (mount->id == *id)
		return 0;

	return -1;
}


static unsigned int
mount_hash(void *_m, const void *_key, unsigned int range)
{
	struct fs_mount *mount = _m;
	const fs_id *id = _key;

	if (mount)
		return mount->id % range;

	return *id % range;
}


static struct fs_mount *
find_mount(fs_id id)
{
	struct fs_mount *mount;

	mutex_lock(&gMountMutex);

	mount = hash_lookup(mounts_table, &id);

	mutex_unlock(&gMountMutex);

	return mount;
}


static int
vnode_compare(void *_v, const void *_key)
{
	struct vnode *v = _v;
	const struct vnode_hash_key *key = _key;

	if (v->fs_id == key->fs_id && v->id == key->vnode_id)
		return 0;

	return -1;
}


static unsigned int
vnode_hash(void *_v, const void *_key, unsigned int range)
{
	struct vnode *vnode = _v;
	const struct vnode_hash_key *key = _key;

#define VHASH(fsid, vnid) (((uint32)((vnid)>>32) + (uint32)(vnid)) ^ (uint32)(fsid))

	if (vnode != NULL)
		return (VHASH(vnode->fs_id, vnode->id) % range);
	else
		return (VHASH(key->fs_id, key->vnode_id) % range);

#undef VHASH
}


static void
add_vnode_to_mount_list(struct vnode *v, struct fs_mount *mount)
{
	recursive_lock_lock(&mount->rlock);

	v->mount_next = mount->vnodes_head;
	v->mount_prev = NULL;
	if (v->mount_next)
		v->mount_next->mount_prev = v;

	mount->vnodes_head = v;
	if (!mount->vnodes_tail)
		mount->vnodes_tail = v;

	recursive_lock_unlock(&mount->rlock);
}


static void
remove_vnode_from_mount_list(struct vnode *v, struct fs_mount *mount)
{
	recursive_lock_lock(&mount->rlock);

	if (v->mount_next)
		v->mount_next->mount_prev = v->mount_prev;
	else
		mount->vnodes_tail = v->mount_prev;
	if (v->mount_prev)
		v->mount_prev->mount_next = v->mount_next;
	else
		mount->vnodes_head = v->mount_next;

	v->mount_prev = v->mount_next = NULL;

	recursive_lock_unlock(&mount->rlock);
}


static struct vnode *
create_new_vnode(void)
{
	struct vnode *v;

	v = (struct vnode *)kmalloc(sizeof(struct vnode));
	if (v == NULL)
		return NULL;

	memset(v, 0, sizeof(struct vnode));//max_commit - old_store_commitment + commitment(v);
	return v;
}


static int
dec_vnode_ref_count(struct vnode *vnode, bool reenter)
{
	int err;
	int old_ref;

	mutex_lock(&gVnodeMutex);

	if (vnode->busy == true)
		panic("dec_vnode_ref_count called on vnode that was busy! vnode %p\n", vnode);

	old_ref = atomic_add(&vnode->ref_count, -1);

	PRINT(("dec_vnode_ref_count: vnode 0x%p, ref now %d\n", vnode, vnode->ref_count));

	if (old_ref == 1) {
		vnode->busy = true;

		mutex_unlock(&gVnodeMutex);

		/* if we have a vm_cache attached, remove it */
		if (vnode->cache)
			vm_cache_release_ref((vm_cache_ref *)vnode->cache);
		vnode->cache = NULL;

		if (vnode->delete_me)
			FS_CALL(vnode,fs_remove_vnode)(vnode->mount->cookie, vnode->private_node, reenter);
		else
			FS_CALL(vnode,fs_put_vnode)(vnode->mount->cookie, vnode->private_node, reenter);

		remove_vnode_from_mount_list(vnode, vnode->mount);

		mutex_lock(&gVnodeMutex);
		hash_remove(vnode_table, vnode);
		mutex_unlock(&gVnodeMutex);

		kfree(vnode);
		err = 1;
	} else {
		mutex_unlock(&gVnodeMutex);
		err = 0;
	}
	return err;
}


static int
inc_vnode_ref_count(struct vnode *vnode)
{
	atomic_add(&vnode->ref_count, 1);
	PRINT(("inc_vnode_ref_count: vnode 0x%p, ref now %d\n", vnode, vnode->ref_count));
	return 0;
}


static struct vnode *
lookup_vnode(fs_id fsID, vnode_id vnodeID)
{
	struct vnode_hash_key key;

	key.fs_id = fsID;
	key.vnode_id = vnodeID;

	return hash_lookup(vnode_table, &key);
}


static int
get_vnode(fs_id fsID, vnode_id vnodeID, struct vnode **_vnode, int reenter)
{
	struct vnode *vnode;
	int err;

	FUNCTION(("get_vnode: fsid %d vnid 0x%Lx 0x%p\n", fsID, vnodeID,_vnode));

	mutex_lock(&gVnodeMutex);

	do {
		vnode = lookup_vnode(fsID, vnodeID);
		if (vnode) {
			if (vnode->busy) {
				mutex_unlock(&gVnodeMutex);
				snooze(10000); // 10 ms
				mutex_lock(&gVnodeMutex);
				continue;
			}
		}
	} while (0);

	PRINT(("get_vnode: tried to lookup vnode, got 0x%p\n", vnode));

	if (vnode) {
		inc_vnode_ref_count(vnode);
	} else {
		// we need to create a new vnode and read it in
		vnode = create_new_vnode();
		if (!vnode) {
			err = ENOMEM;
			goto err;
		}
		vnode->fs_id = fsID;
		vnode->id = vnodeID;
		vnode->mount = find_mount(fsID);
		if (!vnode->mount) {
			err = ERR_INVALID_HANDLE;
			goto err;
		}
		vnode->busy = true;
		hash_insert(vnode_table, vnode);
		mutex_unlock(&gVnodeMutex);

		add_vnode_to_mount_list(vnode, vnode->mount);

		err = FS_CALL(vnode,fs_get_vnode)(vnode->mount->cookie, vnodeID, &vnode->private_node, reenter);
		if (err < 0 && vnode->private_node == NULL) {
			remove_vnode_from_mount_list(vnode, vnode->mount);
			if (vnode->private_node == NULL)
				err = EINVAL;
		}
		mutex_lock(&gVnodeMutex);
		if (err < 0)
			goto err1;

		vnode->busy = false;
		vnode->ref_count = 1;
	}

	mutex_unlock(&gVnodeMutex);

	PRINT(("get_vnode: returning 0x%p\n", vnode));

	*_vnode = vnode;
	return B_OK;

err1:
	hash_remove(vnode_table, vnode);
err:
	mutex_unlock(&gVnodeMutex);
	if (vnode)
		kfree(vnode);

	return err;
}


static inline void
put_vnode(struct vnode *vnode)
{
	dec_vnode_ref_count(vnode, false);
}


static struct file_descriptor *
get_fd_and_vnode(int fd, struct vnode **_vnode, bool kernel)
{
	struct file_descriptor *descriptor = get_fd(get_current_io_context(kernel), fd);
	if (descriptor == NULL)
		return NULL;

	if (descriptor->vnode == NULL) {
		put_fd(descriptor);
		return NULL;
	}

	*_vnode = descriptor->vnode;
	return descriptor;
}


static void
file_free_fd(struct file_descriptor *descriptor)
{
	struct vnode *vnode = descriptor->vnode;

	if (vnode != NULL) {
		FS_CALL(vnode,fs_free_cookie)(vnode->mount->cookie, vnode->private_node, descriptor->cookie);
		dec_vnode_ref_count(vnode, false);
	}
}


static void
dir_free_fd(struct file_descriptor *descriptor)
{
	struct vnode *vnode = descriptor->vnode;

	if (vnode != NULL) {
		//FS_CALL(vnode,fs_close_dir)(vnode->mount->cookie, vnode->private_node, descriptor->cookie);
		FS_CALL(vnode,fs_free_dir_cookie)(vnode->mount->cookie, vnode->private_node, descriptor->cookie);
		dec_vnode_ref_count(vnode, false);
	}
}


static struct vnode *
get_vnode_from_fd(struct io_context *ioContext, int fd)
{
	struct file_descriptor *descriptor;
	struct vnode *vnode;

	descriptor = get_fd(ioContext, fd);
	if (descriptor == NULL)
		return NULL;

	vnode = descriptor->vnode;
	if (vnode != NULL)
		inc_vnode_ref_count(vnode);

	put_fd(descriptor);
	return vnode;
}


static struct fs_container *
find_fs(const char *fs_name)
{
	struct fs_container *fs = fs_list;

	while (fs != NULL) {
		if (strcmp(fs_name, fs->name) == 0)
			return fs;

		fs = fs->next;
	}
	return NULL;
}


static status_t
entry_ref_to_vnode(fs_id fsID,vnode_id directoryID,const char *name,struct vnode **_vnode)
{
	struct vnode *directory, *vnode;
	vnode_id id;
	int status;
	int type;

	status = get_vnode(fsID,directoryID,&directory,false);
	if (status < 0)
		return status;

	status = FS_CALL(directory,fs_lookup)(directory->mount->cookie,
		directory->private_node, name, &id, &type);
	put_vnode(directory);

	if (status < 0)
		return status;

	mutex_lock(&gVnodeMutex);
	vnode = lookup_vnode(fsID, id);
	mutex_unlock(&gVnodeMutex);

	if (vnode == NULL) {
		// fs_lookup() should have left the vnode referenced, so chances
		// are good that this will never happen
		panic("entry_ref_to_vnode: could not lookup vnode (fsid 0x%x vnid 0x%Lx)\n", fsID, id);
		return B_ENTRY_NOT_FOUND;
	}

	*_vnode = vnode;
	return B_OK;
}


static int
vnode_path_to_vnode(struct vnode *vnode, char *path, bool traverseLeafLink, struct vnode **_vnode, int count)
{
	int status = 0;

	if (!path)
		return EINVAL;

	while (true) {
		struct vnode *nextVnode;
		vnode_id vnodeID;
		char *nextPath;
		int type;

		PRINT(("path_to_vnode: top of loop. p = %p, *p = %c, p = '%s'\n", path, *path, path));

		// done?
		if (*path == '\0')
			break;

		// walk to find the next path component ("path" will point to a single
		// path component), and filter out multiple slashes
		for (nextPath = path + 1;*nextPath != '\0' && *nextPath != '/';nextPath++);

		if (*nextPath == '/') {
			*nextPath = '\0';
			do
				nextPath++;
			while (*nextPath == '/');
		}

		// see if the .. is at the root of a mount
		if (strcmp("..", path) == 0 && vnode->mount->root_vnode == vnode) {
			// move to the covered vnode so we pass the '..' parse to the underlying filesystem
			if (vnode->mount->covers_vnode) {
				nextVnode = vnode->mount->covers_vnode;
				inc_vnode_ref_count(nextVnode);
				dec_vnode_ref_count(vnode, false);
				vnode = nextVnode;
			}
		}

		// tell the filesystem to get the vnode of this path component
		status = FS_CALL(vnode,fs_lookup)(vnode->mount->cookie, vnode->private_node, path, &vnodeID, &type);
		if (status < 0) {
			put_vnode(vnode);
			return status;
		}

		// lookup the vnode, the call to fs_lookup should have caused a get_vnode to be called
		// from inside the filesystem, thus the vnode would have to be in the list and it's
		// ref count incremented at this point
		mutex_lock(&gVnodeMutex);
		nextVnode = lookup_vnode(vnode->fs_id, vnodeID);
		mutex_unlock(&gVnodeMutex);

		if (!nextVnode) {
			// pretty screwed up here
			panic("path_to_vnode: could not lookup vnode (fsid 0x%x vnid 0x%Lx)\n", vnode->fs_id, vnodeID);
			put_vnode(vnode);
			return ERR_VFS_PATH_NOT_FOUND;
		}

		// If the new node is a symbolic link, resolve it (if we've been told to do it)
		if (S_ISLNK(type) && !(!traverseLeafLink && nextPath[0] == '\0')) {
			char *buffer;

			// it's not exactly nice style using goto in this way, but hey, it works :-/
			if (count + 1 > MAX_SYM_LINKS) {
				status = B_LINK_LIMIT;
				goto resolve_link_error;
			}

			buffer = kmalloc(SYS_MAX_PATH_LEN);
			if (buffer == NULL) {
				status = B_NO_MEMORY;
				goto resolve_link_error;
			}

			status = FS_CALL(nextVnode,fs_read_link)(nextVnode->mount->cookie, nextVnode->private_node, buffer, SYS_MAX_PATH_LEN);
			if (status < B_OK) {
				kfree(buffer);

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
				vnode = root_vnode;
				inc_vnode_ref_count(vnode);
			}

			status = vnode_path_to_vnode(vnode, path, traverseLeafLink, &nextVnode, count + 1);

			kfree(buffer);

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
	return B_OK;
}


static int
path_to_vnode(char *path, bool traverseLink, struct vnode **_vnode, bool kernel)
{
	struct vnode *start;
	int linkCount = 0;
	int status = 0;

	if (!path)
		return EINVAL;

	// figure out if we need to start at root or at cwd
	if (*path == '/') {
		while (*++path == '/')
			;
		start = root_vnode;
		inc_vnode_ref_count(start);
	} else {
		struct io_context *context = get_current_io_context(kernel);

		mutex_lock(&context->io_mutex);
		start = context->cwd;
		inc_vnode_ref_count(start);
		mutex_unlock(&context->io_mutex);
	}

	return vnode_path_to_vnode(start, path, traverseLink, _vnode, 0);
}


/** Returns the vnode in the next to last segment of the path, and returns
 *	the last portion in filename.
 *	The path buffer must be able to store at least one additional character.
 */

static int
path_to_dir_vnode(char *path, struct vnode **_vnode, char *filename, bool kernel)
{
	char *p = strrchr(path, '/');

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
		status = FS_CALL(vnode,fs_lookup)(vnode->mount->cookie,vnode->private_node,"..",&parentID,&type);
		if (status < B_OK)
			goto out;

		mutex_lock(&gVnodeMutex);
		parentVnode = lookup_vnode(vnode->fs_id, parentID);
		mutex_unlock(&gVnodeMutex);
		
		if (parentVnode == NULL) {
			panic("dir_vnode_to_path: could not lookup vnode (fsid 0x%x vnid 0x%Lx)\n", vnode->fs_id, parentID);
			status = B_ENTRY_NOT_FOUND;
			goto out;
		}

		// Does the file system support getting the name of a vnode?
		// If so, get it here...
		if (status == B_OK && FS_CALL(vnode,fs_get_vnode_name))
			status = FS_CALL(vnode,fs_get_vnode_name)(vnode->mount->cookie,vnode->private_node,name,B_FILE_NAME_LENGTH);

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

		if (!FS_CALL(vnode,fs_get_vnode_name)) {
			// If we don't got the vnode's name yet, we have to search for it
			// in the parent directory now
			file_cookie cookie;

			status = FS_CALL(vnode,fs_open_dir)(vnode->mount->cookie,vnode->private_node,&cookie);
			if (status >= B_OK) {
				struct dirent *dirent = (struct dirent *)nameBuffer;
				while (true) {
					uint32 num = 1;
					status = FS_CALL(vnode,fs_read_dir)(vnode->mount->cookie,vnode->private_node,cookie,dirent,sizeof(nameBuffer),&num);
					if (status < B_OK)
						break;
					
					if (id == dirent->d_ino)
						// found correct entry!
						break;
				}
				FS_CALL(vnode,fs_close_dir)(vnode->mount->cookie,vnode->private_node,cookie);
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
	if (bufferSize - (sizeof(path) - insert) < length + 1) {
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


static status_t
fd_and_path_to_vnode(int fd, char *path, bool traverseLeafLink, struct vnode **_vnode, bool kernel)
{
	struct vnode *vnode;

	if (fd != -1) {
		struct file_descriptor *descriptor = get_fd_and_vnode(fd, &vnode, kernel);
		if (descriptor == NULL)
			return EBADF;

		inc_vnode_ref_count(vnode);
		put_fd(descriptor);
		
		*_vnode = vnode;
		return B_OK;
	}

	return path_to_vnode(path, traverseLeafLink, _vnode, kernel);
}


//	#pragma mark -
//	Functions the VFS exports for other parts of the kernel


int
vfs_get_vnode(fs_id fsID, vnode_id vnodeID, fs_vnode *_fsNode)
{
	struct vnode *vnode;

	int status = get_vnode(fsID, vnodeID, &vnode, true);
	if (status < 0)
		return status;

	*_fsNode = vnode->private_node;
	return B_OK;
}


int
vfs_put_vnode(fs_id fsID, vnode_id vnodeID)
{
	struct vnode *vnode;

	mutex_lock(&gVnodeMutex);
	vnode = lookup_vnode(fsID, vnodeID);
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


int
vfs_remove_vnode(fs_id fsid, vnode_id vnid)
{
	struct vnode *vnode;

	mutex_lock(&gVnodeMutex);

	vnode = lookup_vnode(fsid, vnid);
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
	if (test_and_set((int *)&(((struct vnode *)vnode)->cache), (int)cache, 0) == 0)
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
		return ERR_INVALID_HANDLE;

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

	if (FS_CALL(vnode,fs_can_page))
		return FS_CALL(vnode,fs_can_page)(vnode->mount->cookie, vnode->private_node);

	return 0;
}


ssize_t
vfs_read_page(void *_v, iovecs *vecs, off_t pos)
{
	struct vnode *vnode = _v;

	FUNCTION(("vfs_readpage: vnode %p, vecs %p, pos %Ld\n", vnode, vecs, pos));

	return FS_CALL(vnode,fs_read_page)(vnode->mount->cookie, vnode->private_node, vecs, pos);
}


ssize_t
vfs_write_page(void *_v, iovecs *vecs, off_t pos)
{
	struct vnode *vnode = _v;

	FUNCTION(("vfs_writepage: vnode %p, vecs %p, pos %Ld\n", vnode, vecs, pos));

	return FS_CALL(vnode,fs_write_page)(vnode->mount->cookie, vnode->private_node, vecs, pos);
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

	context = kmalloc(sizeof(struct io_context));
	if (context == NULL)
		return NULL;

	memset(context, 0, sizeof(struct io_context));

	parentContext = (struct io_context *)_parentContext;
	if (parentContext)
		table_size = parentContext->table_size;
	else
		table_size = DEFAULT_FD_TABLE_SIZE;

	context->fds = kmalloc(sizeof(struct file_descriptor *) * table_size);
	if (context->fds == NULL) {
		kfree(context);
		return NULL;
	}

	memset(context->fds, 0, sizeof(struct file_descriptor *) * table_size);

	if (mutex_init(&context->io_mutex, "I/O context") < 0) {
		kfree(context->fds);
		kfree(context);
		return NULL;
	}

	/*
	 * copy parent files
	 */
	if (parentContext) {
		size_t i;

		mutex_lock(&parentContext->io_mutex);

		context->cwd = parentContext->cwd;
		if (context->cwd)
			inc_vnode_ref_count(context->cwd);

		for (i = 0; i < table_size; i++) {
			if (parentContext->fds[i]) {
				context->fds[i] = parentContext->fds[i];
				atomic_add(&context->fds[i]->ref_count, 1);
			}
		}

		mutex_unlock(&parentContext->io_mutex);
	} else {
		context->cwd = root_vnode;

		if (context->cwd)
			inc_vnode_ref_count(context->cwd);
	}

	context->table_size = table_size;

	return context;
}


int
vfs_free_io_context(void *_ioContext)
{
	struct io_context *context = (struct io_context *)_ioContext;
	int i;

	if (context->cwd)
		dec_vnode_ref_count(context->cwd, false);

	mutex_lock(&context->io_mutex);

	for (i = 0; i < context->table_size; i++) {
		if (context->fds[i])
			put_fd(context->fds[i]);
	}

	mutex_unlock(&context->io_mutex);

	mutex_destroy(&context->io_mutex);

	kfree(context->fds);
	kfree(context);
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

	if (newSize < context->table_size) {
		// shrink the fd table
		int i;

		// Make sure none of the fds being dropped are in use
		for(i = context->table_size; i-- > newSize;) {
			if (context->fds[i]) {
				status = EBUSY;
				goto out;
			}
		}

		fds = kmalloc(sizeof(struct file_descriptor *) * newSize);
		if (fds == NULL) {
			status = ENOMEM;
			goto out;
		}

		memcpy(fds, context->fds, sizeof(struct file_descriptor *) * newSize);
	} else {
		// enlarge the fd table

		fds = kmalloc(sizeof(struct file_descriptor *) * newSize);
		if (fds == NULL) {
			status = ENOMEM;
			goto out;
		}

		// copy the fd array, and zero the additional slots
		memcpy(fds, context->fds, sizeof(void *) * context->table_size);
		memset((char *)fds + (sizeof(void *) * context->table_size), 0,
			sizeof(void *) * (newSize - context->table_size));
	}

	kfree(context->fds);
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


image_id
vfs_load_fs_module(const char *name)
{
	image_id id;
	void (*bootstrap)();
	char path[SYS_MAX_PATH_LEN];

//	sprintf(path, "/boot/addons/fs/%s", name);

	id = elf_load_kspace(path, "");
	if (id < 0)
		return id;

	bootstrap = (void *)elf_lookup_symbol(id, "fs_bootstrap");
	if (!bootstrap)
		return ERR_VFS_INVALID_FS;

	bootstrap();

	return id;
}


int
vfs_bootstrap_all_filesystems(void)
{
	int err;
	int fd;

	// bootstrap the root filesystem
	bootstrap_rootfs();

	err = sys_mount("/", NULL, "rootfs", NULL);
	if (err < 0)
		panic("error mounting rootfs!\n");

	sys_setcwd(-1, "/");

	// bootstrap the bootfs
	bootstrap_bootfs();

	sys_create_dir("/boot",0755);
	err = sys_mount("/boot", NULL, "bootfs", NULL);
	if (err < 0)
		panic("error mounting bootfs\n");

	// bootstrap the devfs
	bootstrap_devfs();

	sys_create_dir("/dev",0755);
	err = sys_mount("/dev", NULL, "devfs", NULL);
	if (err < 0)
		panic("error mounting devfs\n");

	fd = sys_open_dir("/boot/addons/fs");
	if (fd >= 0) {
		char buffer[sizeof(struct dirent) + 1 + SYS_MAX_NAME_LEN];
		struct dirent *dirent = (struct dirent *)buffer;
		ssize_t length;

		while ((length = sys_read_dir(fd, dirent, sizeof(buffer), 1)) > 0)
			vfs_load_fs_module(dirent->d_name);

		sys_close(fd);
	}

	return B_NO_ERROR;
}


int
vfs_register_filesystem(const char *name, struct fs_calls *calls)
{
	struct fs_container *container;

	container = (struct fs_container *)kmalloc(sizeof(struct fs_container));
	if (container == NULL)
		return ENOMEM;

	container->name = name;
	container->calls = calls;

	mutex_lock(&gRegisterMutex);

	container->next = fs_list;
	fs_list = container;

	mutex_unlock(&gRegisterMutex);
	return 0;
}


int
vfs_init(kernel_args *ka)
{
	{
		struct vnode *v;
		vnode_table = hash_init(VNODE_HASH_TABLE_SIZE, (addr)&v->next - (addr)v,
			&vnode_compare, &vnode_hash);
		if (vnode_table == NULL)
			panic("vfs_init: error creating vnode hash table\n");
	}
	{
		struct fs_mount *mount;
		mounts_table = hash_init(MOUNTS_HASH_TABLE_SIZE, (addr)&mount->next - (addr)mount,
			&mount_compare, &mount_hash);
		if (mounts_table == NULL)
			panic("vfs_init: error creating mounts hash table\n");
	}
	fs_list = NULL;
	root_vnode = NULL;

	if (mutex_init(&gRegisterMutex, "vfs_lock") < 0)
		panic("vfs_init: error allocating vfs lock\n");

	if (mutex_init(&gMountOpMutex, "vfs_mount_op_lock") < 0)
		panic("vfs_init: error allocating vfs_mount_op lock\n");

	if (mutex_init(&gMountMutex, "vfs_mount_lock") < 0)
		panic("vfs_init: error allocating vfs_mount lock\n");

	if (mutex_init(&gVnodeMutex, "vfs_vnode_lock") < 0)
		panic("vfs_init: error allocating vfs_vnode lock\n");

	return 0;
}


//	#pragma mark -
//	The filetype-dependent implementations (fd_ops + open/create/rename/remove, ...)


static int
new_file_fd(struct vnode *vnode, file_cookie cookie, int openMode, bool kernel)
{
	struct file_descriptor *descriptor;
	int fd;

	descriptor = alloc_fd();
	if (!descriptor)
		return ENOMEM;

	descriptor->vnode = vnode;
	descriptor->cookie = cookie;
	descriptor->ops = &file_ops;
	descriptor->type = FDTYPE_FILE;
	descriptor->open_mode = openMode;

	fd = new_fd(get_current_io_context(kernel), descriptor);
	if (fd < 0)
		return ERR_VFS_FD_TABLE_FULL;

	return fd;
}


/** Calls fs_open() on the given vnode and returns a new
 *	file descriptor for it
 */

static int
create_vnode(struct vnode *directory, const char *name, int omode, int perms, bool kernel)
{
	struct vnode *vnode;
	file_cookie cookie;
	vnode_id newID;
	int status;

	if (FS_CALL(directory,fs_create) == NULL)
		return EROFS;

	status = FS_CALL(directory,fs_create)(directory->mount->cookie, directory->private_node, name, omode, perms, &cookie, &newID);
	if (status < B_OK)
		return status;

	mutex_lock(&gVnodeMutex);
	vnode = lookup_vnode(directory->fs_id, newID);
	mutex_unlock(&gVnodeMutex);

	if (vnode == NULL) {
		dprintf("vfs: fs_create() returned success but there is no vnode!");
		return EINVAL;
	}

	if ((status = new_file_fd(vnode,cookie,omode,kernel)) >= 0)
		return status;
		
	FS_CALL(vnode,fs_close)(vnode->mount->cookie, vnode->private_node, cookie);
	FS_CALL(vnode,fs_free_cookie)(vnode->mount->cookie, vnode->private_node, cookie);
	put_vnode(vnode);

	FS_CALL(directory,fs_unlink)(directory->mount->cookie, directory->private_node, name);
	
	return status;
}


/** Calls fs_open() on the given vnode and returns a new
 *	file descriptor for it
 */

static int
open_vnode(struct vnode *vnode, int omode, bool kernel)
{
	file_cookie cookie;
	int status;

	status = FS_CALL(vnode,fs_open)(vnode->mount->cookie, vnode->private_node, omode, &cookie);
	if (status < 0)
		return status;

	status = new_file_fd(vnode, cookie, omode, kernel);
	if (status < 0) {
		FS_CALL(vnode,fs_close)(vnode->mount->cookie, vnode->private_node, cookie);
		FS_CALL(vnode,fs_free_cookie)(vnode->mount->cookie, vnode->private_node, cookie);
	}
	return status;
}


/** Calls fs_open_dir() on the given vnode and returns a new
 *	file descriptor for it
 */

static int
open_dir_vnode(struct vnode *vnode, bool kernel)
{
	struct file_descriptor *descriptor;
	file_cookie cookie;
	int status, fd;
	
	status = FS_CALL(vnode,fs_open_dir)(vnode->mount->cookie, vnode->private_node, &cookie);
	if (status < 0)
		return status;

	// file is opened, create a fd
	descriptor = alloc_fd();
	if (!descriptor) {
		status = ENOMEM;
		goto err;
	}
	descriptor->vnode = vnode;
	descriptor->cookie = cookie;
	descriptor->ops = &dir_ops;
	descriptor->type = FDTYPE_DIR;

	fd = new_fd(get_current_io_context(kernel), descriptor);
	if (fd >= 0)
		return fd;

err1:
	status = B_NO_MORE_FDS;
	kfree(descriptor);

err:
	FS_CALL(vnode,fs_close_dir)(vnode->mount->cookie, vnode->private_node, cookie);
	FS_CALL(vnode,fs_free_dir_cookie)(vnode->mount->cookie, vnode->private_node, cookie);

	return status;
}


static int
file_create_entry_ref(fs_id fsID, vnode_id directoryID, const char *name, int omode, int perms, bool kernel)
{
	struct vnode *directory,*vnode;
	int status;

	FUNCTION(("file_create_entry_ref: name = '%s', omode %x, perms %d, kernel %d\n", name, omode, perms, kernel));

	// get directory to put the new file in	
	status = get_vnode(fsID,directoryID,&directory,false);
	if (status < B_OK)
		return status;

	status = create_vnode(directory, name, omode, perms, kernel);
	put_vnode(directory);

	return status;
}


static int
file_create(char *path, int omode, int perms, bool kernel)
{
	char filename[SYS_MAX_NAME_LEN];
	struct vnode *directory,*vnode;
	file_cookie cookie;
	vnode_id newID;
	int status;

	FUNCTION(("file_create: path '%s', omode %x, perms %d, kernel %d\n", path, omode, perms, kernel));

	// get directory to put the new file in	
	status = path_to_dir_vnode(path, &directory, filename, kernel);
	if (status < 0)
		return status;

	status = create_vnode(directory,filename,omode,perms,kernel);
	put_vnode(directory);

	return status;
}


static int
file_open_entry_ref(fs_id fsID, vnode_id directoryID, const char *name, int omode, bool kernel)
{
	struct vnode *vnode;
	int status;

	FUNCTION(("file_open_entry_ref()\n"));

	// get the vnode matching the entry_ref
	status = entry_ref_to_vnode(fsID, directoryID, name, &vnode);
	if (status < B_OK)
		return status;

	status = open_vnode(vnode, omode, kernel);
	if (status < B_OK)
		put_vnode(vnode);

	return status;
}


static int
file_open(char *path, int omode, bool kernel)
{
	struct file_descriptor *descriptor;
	struct vnode *vnode = NULL;
	int status;
	int fd;

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


static int
file_close(struct file_descriptor *descriptor)
{
	struct vnode *vnode = descriptor->vnode;

	FUNCTION(("file_close(descriptor = %p)\n",descriptor));
	
	if (FS_CALL(vnode,fs_close))
		return FS_CALL(vnode,fs_close)(vnode->mount->cookie,vnode->private_node,descriptor->cookie);

	return B_OK;
}


static ssize_t
file_read(struct file_descriptor *descriptor, off_t pos, void *buffer, size_t *length)
{
	struct vnode *vnode = descriptor->vnode;

	FUNCTION(("file_read: buf %p, pos %Ld, len %p = %ld\n", buffer, pos, length, *length));
	return FS_CALL(vnode,fs_read)(vnode->mount->cookie, vnode->private_node, descriptor->cookie, pos, buffer, length);
}


static ssize_t
file_write(struct file_descriptor *descriptor, off_t pos, const void *buffer, size_t *length)
{
	struct vnode *vnode = descriptor->vnode;

	FUNCTION(("file_write: buf %p, pos %Ld, len %p\n", buffer, pos, length));
	return FS_CALL(vnode,fs_write)(vnode->mount->cookie, vnode->private_node, descriptor->cookie, pos, buffer, length);
}


static off_t
file_seek(struct file_descriptor *descriptor, off_t pos, int seekType)
{
	struct vnode *vnode = descriptor->vnode;

	FUNCTION(("file_seek: pos 0x%Ld, seek_type %d\n", pos, seekType));
	return FS_CALL(vnode,fs_seek)(vnode->mount->cookie, vnode->private_node, descriptor->cookie, pos, seekType);
}


static int
dir_create_entry_ref(fs_id fsID, vnode_id parentID, const char *name, int perms, bool kernel)
{
	struct vnode *vnode;
	vnode_id newID;
	int status;

	FUNCTION(("dir_create_entry_ref(dev = %d, ino = %Ld, name = '%s', perms = %d)\n", fsID, parentID, name, perms));
	
	status = get_vnode(fsID, parentID, &vnode, kernel);
	if (status < B_OK)
		return status;

	if (FS_CALL(vnode, fs_create_dir))
		status = FS_CALL(vnode, fs_create_dir)(vnode->mount->cookie, vnode->private_node, name, perms, &newID);
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

	if (FS_CALL(vnode,fs_create_dir))
		status = FS_CALL(vnode,fs_create_dir)(vnode->mount->cookie, vnode->private_node, filename, perms, &newID);
	else
		status = EROFS;

	put_vnode(vnode);
	return status;
}


static int
dir_open_node_ref(fs_id fsID, vnode_id directoryID, bool kernel)
{
	struct vnode *vnode;
	int status;

	FUNCTION(("dir_open_entry_ref()\n"));

	// get the vnode matching the node_ref
	status = get_vnode(fsID, directoryID, &vnode, false);
	if (status < B_OK)
		return status;

	status = open_dir_vnode(vnode, kernel);
	if (status < B_OK)
		put_vnode(vnode);

	return status;
}


static int
dir_open_entry_ref(fs_id fsID, vnode_id parentID, const char *name, bool kernel)
{
	struct vnode *vnode;
	int status;

	FUNCTION(("dir_open_entry_ref()\n"));

	// get the vnode matching the entry_ref
	status = entry_ref_to_vnode(fsID, parentID, name, &vnode);
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
	if (status < 0)
		return status;

	status = open_dir_vnode(vnode,kernel);
	if (status < 0)
		put_vnode(vnode);

	return status;
}


static status_t 
dir_read(struct file_descriptor *descriptor, struct dirent *buffer, size_t bufferSize, uint32 *_count)
{
	struct vnode *vnode = descriptor->vnode;

	if (FS_CALL(vnode,fs_read_dir))
		return FS_CALL(vnode,fs_read_dir)(vnode->mount->cookie,vnode->private_node,descriptor->cookie,buffer,bufferSize,_count);
	
	return EOPNOTSUPP;
}


static status_t 
dir_rewind(struct file_descriptor *descriptor)
{
	struct vnode *vnode = descriptor->vnode;

	if (FS_CALL(vnode,fs_rewind_dir))
		return FS_CALL(vnode,fs_rewind_dir)(vnode->mount->cookie,vnode->private_node,descriptor->cookie);

	return EOPNOTSUPP;
}


static int
dir_close(struct file_descriptor *descriptor)
{
	struct vnode *vnode = descriptor->vnode;

	FUNCTION(("dir_close(descriptor = %p)\n",descriptor));
	
	if (FS_CALL(vnode,fs_close_dir))
		return FS_CALL(vnode,fs_close_dir)(vnode->mount->cookie,vnode->private_node,descriptor->cookie);

	return B_OK;
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

	if (FS_CALL(directory, fs_remove_dir))
		status = FS_CALL(directory, fs_remove_dir)(directory->mount->cookie, directory->private_node, name);
	else
		status = EROFS;

	put_vnode(directory);
	return status;
}


static int
common_read_stat(struct file_descriptor *descriptor, struct stat *stat)
{
	struct vnode *vnode = descriptor->vnode;

	FUNCTION(("common_read_stat: stat 0x%p\n", stat));
	return FS_CALL(vnode,fs_read_stat)(vnode->mount->cookie, vnode->private_node, stat);
}


static int
common_ioctl(struct file_descriptor *descriptor, ulong op, void *buffer, size_t length)
{
	struct vnode *vnode = descriptor->vnode;

	if (FS_CALL(vnode,fs_ioctl))
		return FS_CALL(vnode,fs_ioctl)(vnode->mount->cookie,vnode->private_node,descriptor->cookie,op,buffer,length);

	return EOPNOTSUPP;
}


static int
common_sync(int fd, bool kernel)
{
	struct file_descriptor *descriptor;
	struct vnode *vnode;
	int status;

	FUNCTION(("vfs_fsync: entry. fd %d kernel %d\n", fd, kernel));

	descriptor = get_fd_and_vnode(fd, &vnode, kernel);
	if (descriptor == NULL)
		return ERR_INVALID_HANDLE;

	if (FS_CALL(vnode,fs_fsync) != NULL)
		status = FS_CALL(vnode,fs_fsync)(vnode->mount->cookie, vnode->private_node);
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

	if (FS_CALL(vnode,fs_read_link) != NULL)
		status = FS_CALL(vnode,fs_read_link)(vnode->mount->cookie, vnode->private_node, buffer, bufferSize);
	else
		status = B_BAD_VALUE;

	put_vnode(vnode);

	return status;
}


static int
common_write_link(char *path, char *toPath, bool kernel)
{
	struct vnode *vnode;
	int status;

	status = path_to_vnode(path, false, &vnode, kernel);
	if (status < B_OK)
		return status;

	if (FS_CALL(vnode,fs_write_link) != NULL)
		status = FS_CALL(vnode,fs_write_link)(vnode->mount->cookie, vnode->private_node, toPath);
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

	if (FS_CALL(vnode,fs_create_symlink) != NULL)
		status = FS_CALL(vnode,fs_create_symlink)(vnode->mount->cookie, vnode->private_node, name, toPath, mode);
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

	if (FS_CALL(vnode, fs_link) != NULL)
		status = FS_CALL(vnode,fs_link)(directory->mount->cookie, directory->private_node, name, vnode->private_node);
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

	if (FS_CALL(vnode,fs_unlink) != NULL)
		status = FS_CALL(vnode,fs_unlink)(vnode->mount->cookie, vnode->private_node, filename);
	else
		status = EROFS;

	put_vnode(vnode);

	return status;
}


static int
common_access(char *path, int mode, bool kernel)
{
	struct vnode *vnode;
	int status;

	status = path_to_vnode(path, true, &vnode, kernel);
	if (status < B_OK)
		return status;

	if (FS_CALL(vnode,fs_access) != NULL)
		status = FS_CALL(vnode,fs_access)(vnode->mount->cookie, vnode->private_node, mode);
	else
		status = EOPNOTSUPP;

	put_vnode(vnode);

	return status;
}


static int
common_rename(char *path, char *newPath, bool kernel)
{
	struct vnode *vnode1, *vnode2;
	char filename1[SYS_MAX_NAME_LEN];
	char filename2[SYS_MAX_NAME_LEN];
	int status;

	FUNCTION(("common_rename(path = %s, newPath = %s, kernel = %d)\n", path, newPath, kernel));

	status = path_to_dir_vnode(path, &vnode1, filename1, kernel);
	if (status < 0)
		goto err;

	status = path_to_dir_vnode(newPath, &vnode2, filename2, kernel);
	if (status < 0)
		goto err1;

	if (vnode1->fs_id != vnode2->fs_id) {
		status = ERR_VFS_CROSS_FS_RENAME;
		goto err2;
	}

	if (FS_CALL(vnode1,fs_rename) != NULL)
		status = FS_CALL(vnode1,fs_rename)(vnode1->mount->cookie, vnode1->private_node, filename1, vnode2->private_node, filename2);
	else
		status = EROFS;

err2:
	put_vnode(vnode2);
err1:
	put_vnode(vnode1);
err:
	return status;
}


static int
common_write_stat(int fd, char *path, bool traverseLeafLink, const struct stat *stat, int statMask, bool kernel)
{
	struct vnode *vnode;
	int status;

	FUNCTION(("common_write_stat: path '%s', stat 0x%p, stat_mask %d, kernel %d\n", path, stat, statMask, kernel));

	status = fd_and_path_to_vnode(fd, path, traverseLeafLink, &vnode, kernel);
	if (status < 0)
		return status;

	if (FS_CALL(vnode,fs_write_stat))
		status = FS_CALL(vnode,fs_write_stat)(vnode->mount->cookie, vnode->private_node, stat, statMask);
	else
		status = EROFS;

	dec_vnode_ref_count(vnode, false);

	return status;
}


//	#pragma mark -
//	General File System functions


static int
fs_mount(char *path, const char *device, const char *fs_name, void *args, bool kernel)
{
	struct fs_mount *mount;
	int err = 0;
	struct vnode *covered_vnode = NULL;
	vnode_id root_id;

	FUNCTION(("vfs_mount: entry. path = '%s', fs_name = '%s'\n", path, fs_name));

	mutex_lock(&gMountOpMutex);

	mount = (struct fs_mount *)kmalloc(sizeof(struct fs_mount));
	if (mount == NULL)  {
		err = ENOMEM;
		goto err;
	}

	mount->mount_point = kstrdup(path);
	if (mount->mount_point == NULL) {
		err = ENOMEM;
		goto err1;
	}

	mount->fs = find_fs(fs_name);
	if (mount->fs == NULL) {
		err = ERR_VFS_INVALID_FS;
		goto err2;
	}

	recursive_lock_create(&mount->rlock);
	mount->id = next_fsid++;
	mount->unmounting = false;

	if (!root_vnode) {
		// we haven't mounted anything yet
		if (strcmp(path, "/") != 0) {
			err = ERR_VFS_GENERAL;
			goto err3;
		}

		err = mount->fs->calls->fs_mount(mount->id, device, NULL, &mount->cookie, &root_id);
		if (err < 0) {
			err = ERR_VFS_GENERAL;
			goto err3;
		}

		mount->covers_vnode = NULL; // this is the root mount
	} else {
		err = path_to_vnode(path, true, &covered_vnode, kernel);
		if (err < 0)
			goto err2;

		if (!covered_vnode) {
			err = ERR_VFS_GENERAL;
			goto err2;
		}

		// XXX insert check to make sure covered_vnode is a DIR, or maybe it's okay for it not to be

		if ((covered_vnode != root_vnode) && (covered_vnode->mount->root_vnode == covered_vnode)){
			err = ERR_VFS_ALREADY_MOUNTPOINT;
			goto err2;
		}

		mount->covers_vnode = covered_vnode;

		// mount it
		err = mount->fs->calls->fs_mount(mount->id, device, NULL, &mount->cookie, &root_id);
		if (err < 0)
			goto err4;
	}

	mutex_lock(&gMountMutex);

	// insert mount struct into list
	hash_insert(mounts_table, mount);

	mutex_unlock(&gMountMutex);

	err = get_vnode(mount->id, root_id, &mount->root_vnode, 0);
	if (err < 0)
		goto err5;

	// XXX may be a race here
	if (mount->covers_vnode)
		mount->covers_vnode->covered_by = mount->root_vnode;

	if (!root_vnode)
		root_vnode = mount->root_vnode;

	mutex_unlock(&gMountOpMutex);

	return 0;

err5:
	mount->fs->calls->fs_unmount(mount->cookie);
err4:
	if (mount->covers_vnode)
		dec_vnode_ref_count(mount->covers_vnode, false);
err3:
	recursive_lock_destroy(&mount->rlock);
err2:
	kfree(mount->mount_point);
err1:
	kfree(mount);
err:
	mutex_unlock(&gMountOpMutex);

	return err;
}


static int
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

	mount = find_mount(vnode->fs_id);
	if (!mount)
		panic("vfs_unmount: fsid_to_mount failed on root vnode @%p of mount\n", vnode);

	if (mount->root_vnode != vnode) {
		// not mountpoint
		dec_vnode_ref_count(vnode, false);
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
	for (vnode = mount->vnodes_head; vnode != NULL; vnode = vnode->mount_next) {
		if (vnode->busy || vnode->ref_count != 0) {
			mount->root_vnode->ref_count += 2;
			mutex_unlock(&gVnodeMutex);
			dec_vnode_ref_count(mount->root_vnode, false);

			err = EBUSY;
			goto err;
		}
	}

	/* we can safely continue, mark all of the vnodes busy and this mount
	structure in unmounting state */
	for (vnode = mount->vnodes_head; vnode; vnode = vnode->mount_next)
		if (vnode != mount->root_vnode)
			vnode->busy = true;
	mount->unmounting = true;

	mutex_unlock(&gVnodeMutex);

	mount->covers_vnode->covered_by = NULL;
	dec_vnode_ref_count(mount->covers_vnode, false);

	/* release the ref on the root vnode twice */
	dec_vnode_ref_count(mount->root_vnode, false);
	dec_vnode_ref_count(mount->root_vnode, false);

	// ToDo: when full vnode cache in place, will need to force
	// a putvnode/removevnode here

	/* remove the mount structure from the hash table */
	mutex_lock(&gMountMutex);
	hash_remove(mounts_table, mount);
	mutex_unlock(&gMountMutex);

	mutex_unlock(&gMountOpMutex);

	mount->fs->calls->fs_unmount(mount->cookie);

	kfree(mount->mount_point);
	kfree(mount);

	return 0;

err:
	mutex_unlock(&gMountOpMutex);
	return err;
}


static int
fs_sync(void)
{
	struct hash_iterator iter;
	struct fs_mount *mount;

	FUNCTION(("vfs_sync: entry.\n"));

	/* cycle through and call sync on each mounted fs */
	mutex_lock(&gMountOpMutex);
	mutex_lock(&gMountMutex);

	hash_open(mounts_table, &iter);
	while ((mount = hash_next(mounts_table, &iter))) {
		mount->fs->calls->fs_sync(mount->cookie);
	}
	hash_close(mounts_table, &iter, false);

	mutex_unlock(&gMountMutex);
	mutex_unlock(&gMountOpMutex);

	return 0;
}


static int
fs_read_info(dev_t device, struct fs_info *info)
{
	struct fs_mount *mount;
	int status;

	mutex_lock(&gMountMutex);	

	mount = find_mount(device);
	if (mount == NULL) {
		status = EINVAL;
		goto error;
	}

	if (mount->fs->calls->fs_read_fs_info)
		status = mount->fs->calls->fs_read_fs_info(mount->cookie, info);
	else
		status = EOPNOTSUPP;

	// fill in other info the file system doesn't (have to) know about
	info->dev = mount->id;
	info->root = mount->root_vnode->id;

error:
	mutex_unlock(&gMountMutex);
	return status;
}


static int
fs_write_info(dev_t device, const struct fs_info *info, int mask)
{
	struct fs_mount *mount;
	int status;

	mutex_lock(&gMountMutex);	

	mount = find_mount(device);
	if (mount == NULL) {
		status = EINVAL;
		goto error;
	}

	if (mount->fs->calls->fs_write_fs_info)
		status = mount->fs->calls->fs_write_fs_info(mount->cookie, info, mask);
	else
		status = EROFS;

error:
	mutex_unlock(&gMountMutex);
	return status;
}


static int
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


static int
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

	rc = FS_CALL(vnode,fs_read_stat)(vnode->mount->cookie, vnode->private_node, &stat);
	if (rc < 0)
		goto err;

	if (!S_ISDIR(stat.st_mode)) {
		// nope, can't cwd to here
		rc = ERR_VFS_WRONG_STREAM_TYPE;
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
sys_read_stat(const char *path, bool traverseLeafLink, struct stat *stat)
{
	char pathBuffer[SYS_MAX_PATH_LEN + 1];
	struct vnode *vnode;
	int status;

	strlcpy(pathBuffer, path, SYS_MAX_PATH_LEN - 1);

	FUNCTION(("sys_read_stat: path '%s', stat %p,\n", path, stat));

	status = path_to_vnode(pathBuffer, traverseLeafLink, &vnode, true);
	if (status < 0)
		return status;

	status = FS_CALL(vnode,fs_read_stat)(vnode->mount->cookie, vnode->private_node, stat);

	put_vnode(vnode);
	return status;
}


int
sys_write_stat(int fd, const char *path, bool traverseLeafLink, struct stat *stat, int statMask)
{
	char pathBuffer[SYS_MAX_PATH_LEN + 1];

	if (fd == -1)
		strlcpy(pathBuffer, path, SYS_MAX_PATH_LEN - 1);

	return common_write_stat(fd, pathBuffer, traverseLeafLink, stat, statMask, true);
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

	rc = user_strncpy(path, upath, SYS_MAX_PATH_LEN - 1);
	if (rc < 0)
		return rc;
	path[SYS_MAX_PATH_LEN - 1] = '\0';

	rc = user_strncpy(fs_name, ufs_name, SYS_MAX_OS_NAME_LEN - 1);
	if (rc < 0)
		return rc;
	fs_name[SYS_MAX_OS_NAME_LEN - 1] = '\0';

	if (udevice) {
		rc = user_strncpy(device, udevice, SYS_MAX_PATH_LEN - 1);
		if (rc < 0)
			return rc;
		device[SYS_MAX_PATH_LEN - 1] = '\0';
	} else
		device[0] = '\0';

	return fs_mount(path, device, fs_name, args, false);
}


int
user_unmount(const char *upath)
{
	char path[SYS_MAX_PATH_LEN + 1];
	int status;

	status = user_strncpy(path, upath, SYS_MAX_PATH_LEN - 1);
	if (status < 0)
		return status;
	path[SYS_MAX_PATH_LEN - 1] = '\0';

	return fs_unmount(path, false);
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
		return ERR_VM_BAD_USER_MEMORY;

	status = user_strncpy(name, userName, sizeof(name) - 1);
	if (status < B_OK)
		return status;
	name[sizeof(name) - 1] = '\0';

	return file_open_entry_ref(device, inode, name, omode, false);
}


int
user_open(const char *userPath, int omode)
{
	char path[SYS_MAX_PATH_LEN + 1];
	int status;

	if (!CHECK_USER_ADDRESS(userPath))
		return ERR_VM_BAD_USER_MEMORY;

	status = user_strncpy(path, userPath, SYS_MAX_PATH_LEN - 1);
	if (status < 0)
		return status;
	path[SYS_MAX_PATH_LEN - 1] = '\0';

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
		return ERR_VM_BAD_USER_MEMORY;

	status = user_strncpy(name, uname, sizeof(name) - 1);
	if (status < B_OK)
		return status;
	name[sizeof(name) - 1] = '\0';

	return dir_open_entry_ref(device, inode, name, false);
}


int
user_open_dir(const char *userPath)
{
	char path[SYS_MAX_PATH_LEN + 1];
	int status;

	if (!CHECK_USER_ADDRESS(userPath))
		return ERR_VM_BAD_USER_MEMORY;

	status = user_strncpy(path, userPath, SYS_MAX_PATH_LEN - 1);
	if (status < 0)
		return status;
	path[SYS_MAX_PATH_LEN - 1] = 0;

	return dir_open(path, false);
}


int
user_fsync(int fd)
{
	return common_sync(fd, false);
}


int
user_create_entry_ref(dev_t device, ino_t inode, const char *uname, int omode, int perms)
{
	char name[B_FILE_NAME_LENGTH];
	int status;

	if (!CHECK_USER_ADDRESS(uname))
		return ERR_VM_BAD_USER_MEMORY;

	status = user_strncpy(name, uname, sizeof(name) - 1);
	if (status < 0)
		return status;
	name[sizeof(name) - 1] = '\0';

	return file_create_entry_ref(device, inode, name, omode, perms, false);
}


int
user_create(const char *userPath, int omode, int perms)
{
	char path[SYS_MAX_PATH_LEN + 1];
	int status;

	if (!CHECK_USER_ADDRESS(userPath))
		return B_BAD_ADDRESS;

	status = user_strncpy(path, userPath, SYS_MAX_PATH_LEN - 1);
	if (status < 0)
		return status;
	path[SYS_MAX_PATH_LEN - 1] = '\0';

	return file_create(path, omode, perms, false);
}


int
user_create_dir_entry_ref(dev_t device, ino_t inode, const char *uname, int perms)
{
	char name[B_FILE_NAME_LENGTH];
	int status;

	if (!CHECK_USER_ADDRESS(uname))
		return B_BAD_ADDRESS;

	status = user_strncpy(name, uname, sizeof(name) - 1);
	if (status < 0)
		return status;
	name[sizeof(name) - 1] = '\0';

	return dir_create_entry_ref(device, inode, name, perms, false);
}


int
user_create_dir(const char *userPath, int perms)
{
	char path[SYS_MAX_PATH_LEN + 1];
	int status;

	if (!CHECK_USER_ADDRESS(userPath))
		return B_BAD_ADDRESS;

	status = user_strncpy(path, userPath, SYS_MAX_PATH_LEN - 1);
	if (status < 0)
		return status;
	path[SYS_MAX_PATH_LEN - 1] = '\0';

	return dir_create(path, perms, false);
}


int
user_remove_dir(const char *userPath)
{
	char path[SYS_MAX_PATH_LEN + 1];
	int status;

	if (!CHECK_USER_ADDRESS(userPath))
		return B_BAD_ADDRESS;

	status = user_strncpy(path, userPath, SYS_MAX_PATH_LEN - 1);
	if (status < 0)
		return status;
	path[SYS_MAX_PATH_LEN - 1] = '\0';

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

	status = user_strncpy(path, userPath, SYS_MAX_PATH_LEN - 1);
	if (status < 0)
		return status;
	path[SYS_MAX_PATH_LEN - 1] = '\0';

	if (bufferSize > SYS_MAX_PATH_LEN)
		bufferSize = SYS_MAX_PATH_LEN;

	status = common_read_link(path, buffer, bufferSize, false);
	if (status < B_OK)
		return status;

	return user_strncpy(userBuffer, buffer, bufferSize);
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

	status = user_strncpy(path, userPath, SYS_MAX_PATH_LEN - 1);
	if (status < 0)
		return status;
	path[SYS_MAX_PATH_LEN - 1] = '\0';

	status = user_strncpy(toPath, userToPath, SYS_MAX_PATH_LEN - 1);
	if (status < 0)
		return status;
	toPath[SYS_MAX_PATH_LEN - 1] = '\0';

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

	status = user_strncpy(path, userPath, SYS_MAX_PATH_LEN - 1);
	if (status < 0)
		return status;
	path[SYS_MAX_PATH_LEN - 1] = '\0';

	status = user_strncpy(toPath, userToPath, SYS_MAX_PATH_LEN - 1);
	if (status < 0)
		return status;
	toPath[SYS_MAX_PATH_LEN - 1] = '\0';

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

	status = user_strncpy(path, userPath, SYS_MAX_PATH_LEN - 1);
	if (status < 0)
		return status;
	path[SYS_MAX_PATH_LEN - 1] = '\0';

	status = user_strncpy(toPath, userToPath, SYS_MAX_PATH_LEN - 1);
	if (status < 0)
		return status;
	toPath[SYS_MAX_PATH_LEN - 1] = '\0';

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

	status = user_strncpy(path, userPath, SYS_MAX_PATH_LEN - 1);
	if (status < 0)
		return status;
	path[SYS_MAX_PATH_LEN - 1] = '\0';

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

	status = user_strncpy(oldPath, userOldPath, SYS_MAX_PATH_LEN - 1);
	if (status < 0)
		return status;
	oldPath[SYS_MAX_PATH_LEN - 1] = '\0';

	status = user_strncpy(newPath, userNewPath, SYS_MAX_PATH_LEN - 1);
	if (status < 0)
		return status;
	newPath[SYS_MAX_PATH_LEN - 1] = '\0';

	return common_rename(oldPath, newPath, false);
}


int
user_access(const char *userPath, int mode)
{
	char path[SYS_MAX_PATH_LEN + 1];
	int status;

	if (!CHECK_USER_ADDRESS(userPath))
		return B_BAD_ADDRESS;

	status = user_strncpy(path, userPath, SYS_MAX_PATH_LEN - 1);
	if (status < 0)
		return status;
	path[SYS_MAX_PATH_LEN - 1] = '\0';

	return common_access(path, mode, false);
}


int
user_read_stat(const char *userPath, bool traverseLink, struct stat *userStat)
{
	char path[SYS_MAX_PATH_LEN + 1];
	struct vnode *vnode = NULL;
	struct stat stat;
	int rc;

	if (!CHECK_USER_ADDRESS(userPath)
		|| !CHECK_USER_ADDRESS(userStat))
		return B_BAD_ADDRESS;

	rc = user_strncpy(path, userPath, SYS_MAX_PATH_LEN - 1);
	if (rc < 0)
		return rc;
	path[SYS_MAX_PATH_LEN - 1] = '\0';

	FUNCTION(("user_read_stat(path = %s, traverseLeafLink = %d)\n", path, traverseLink));

	rc = path_to_vnode(path, traverseLink, &vnode, false);
	if (rc < 0)
		return rc;

	if (FS_CALL(vnode,fs_read_stat))
		rc = FS_CALL(vnode,fs_read_stat)(vnode->mount->cookie, vnode->private_node, &stat);

	put_vnode(vnode);

	if (rc < 0)
		return rc;

	return user_memcpy(userStat, &stat, sizeof(struct stat));
}


int
user_write_stat(int fd, const char *userPath, bool traverseLeafLink, struct stat *userStat, int statMask)
{
	char path[SYS_MAX_PATH_LEN + 1];
	struct stat stat;
	int rc;

	if ((fd == -1 && !CHECK_USER_ADDRESS(userPath))
		|| !CHECK_USER_ADDRESS(userStat))
		return B_BAD_ADDRESS;

	if (fd == -1) {
		rc = user_strncpy(path, userPath, SYS_MAX_PATH_LEN - 1);
		if (rc < 0)
			return rc;
		path[SYS_MAX_PATH_LEN - 1] = '\0';
	}

	rc = user_memcpy(&stat, userStat, sizeof(struct stat));
	if (rc < 0)
		return rc;

	return common_write_stat(fd, path, traverseLeafLink, &stat, statMask, false);
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

	// Call vfs to get current working directory
	status = get_cwd(buffer, size, false);
	if (status < 0)
		return status;

	// Copy back the result
	if (user_strncpy(userBuffer, buffer, size) < 0)
		return ERR_VM_BAD_USER_MEMORY;

	return status;
}


int
user_setcwd(int fd, const char *userPath)
{
	char path[SYS_MAX_PATH_LEN];
	int rc;

	PRINT(("user_setcwd: path = %p\n", userPath));

	if (fd == -1) {
		if (!CHECK_USER_ADDRESS(userPath))
			return B_BAD_ADDRESS;

		rc = user_strncpy(path, userPath, SYS_MAX_PATH_LEN - 1);
		if (rc < 0)
			return rc;
		path[SYS_MAX_PATH_LEN - 1] = '\0';
	} else
		path[0] = '\0';

	// Call vfs to set new working directory
	return set_cwd(fd, path, false);
}

