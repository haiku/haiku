/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <KernelExport.h>
#include <NodeMonitor.h>

#include <vfs.h>
#include <debug.h>
#include <khash.h>
#include <lock.h>
#include <vm.h>

#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

#include "builtin_fs.h"


// ToDo: handles file names suboptimally - it has all file names
//	in a single linked list, no hash lookups or whatever.

#define PIPEFS_TRACE 0

#if PIPEFS_TRACE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x)
#endif

struct pipefs_vnode {
	pipefs_vnode 	*next;
	pipefs_vnode	*hash_next;
	vnode_id		id;
	int32			type;
	char			*name;
};

struct dir_cookie {
	dir_cookie		*next;
	dir_cookie		*prev;
	pipefs_vnode	*current;
	int				open_mode;
};

struct pipefs {
	mount_id		id;
	mutex			lock;
	vnode_id		next_vnode_id;
	hash_table		*vnode_list_hash;
	pipefs_vnode 	*root_vnode;

	// root directory contents - we don't support other directories
	pipefs_vnode	*first_entry;
	dir_cookie		*first_dir_cookie;
};


#define PIPEFS_HASH_SIZE 16


static uint32
pipefs_vnode_hash_func(void *_vnode, const void *_key, uint32 range)
{
	pipefs_vnode *vnode = (pipefs_vnode *)_vnode;
	const vnode_id *key = (const vnode_id *)_key;

	if (vnode != NULL)
		return vnode->id % range;

	return (*key) % range;
}


static int
pipefs_vnode_compare_func(void *_vnode, const void *_key)
{
	pipefs_vnode *vnode = (pipefs_vnode *)_vnode;
	const vnode_id *key = (const vnode_id *)_key;

	if (vnode->id == *key)
		return 0;

	return -1;
}


static pipefs_vnode *
pipefs_create_vnode(pipefs *fs, const char *name, int32 type)
{
	pipefs_vnode *vnode = (pipefs_vnode *)malloc(sizeof(pipefs_vnode));
	if (vnode == NULL)
		return NULL;

	vnode->name = strdup(name);
	if (vnode->name == NULL) {
		free(vnode);
		return NULL;
	}

	vnode->next = NULL;
	vnode->id = fs->next_vnode_id++;
	vnode->type = type;

	return vnode;
}


static status_t
pipefs_delete_vnode(pipefs *fs, pipefs_vnode *vnode, bool forceDelete)
{
	// remove it from the global hash table
	hash_remove(fs->vnode_list_hash, vnode);

	free(vnode->name);
	free(vnode);

	return 0;
}


static void
insert_cookie_in_jar(pipefs *fs, dir_cookie *cookie)
{
	cookie->next = fs->first_dir_cookie;
	fs->first_dir_cookie = cookie;
	cookie->prev = NULL;
}


static void
remove_cookie_from_jar(pipefs *fs, dir_cookie *cookie)
{
	if (cookie->next)
		cookie->next->prev = cookie->prev;

	if (cookie->prev)
		cookie->prev->next = cookie->next;

	if (fs->first_dir_cookie == cookie)
		fs->first_dir_cookie = cookie->next;

	cookie->prev = cookie->next = NULL;
}


/* makes sure none of the dircookies point to the vnode passed in */

static void
update_dircookies(pipefs *fs, pipefs_vnode *vnode)
{
	dir_cookie *cookie;

	for (cookie = fs->first_dir_cookie; cookie; cookie = cookie->next) {
		if (cookie->current == vnode)
			cookie->current = vnode->next;
	}
}


static pipefs_vnode *
pipefs_find(pipefs *fs, const char *name)
{
	if (!strcmp(name, ".")
		|| !strcmp(name, ".."))
		return fs->root_vnode;

	for (pipefs_vnode *vnode = fs->first_entry; vnode; vnode = vnode->next) {
		if (strcmp(vnode->name, name) == 0)
			return vnode;
	}
	return NULL;
}


static status_t
pipefs_insert(pipefs *fs, pipefs_vnode *vnode)
{
	vnode->next = fs->first_entry;
	fs->first_entry = vnode;

	return B_OK;
}


static status_t
pipefs_remove_from_fs(pipefs *fs, pipefs_vnode *findNode)
{
	pipefs_vnode *vnode, *last;

	for (vnode = fs->first_entry, last = NULL; vnode; last = vnode, vnode = vnode->next) {
		if (vnode == findNode) {
			/* make sure all dircookies dont point to this vnode */
			update_dircookies(fs, vnode);

			if (last)
				last->next = vnode->next;
			else
				fs->first_entry = vnode->next;

			vnode->next = NULL;
			return B_OK;
		}
	}
	return B_ENTRY_NOT_FOUND;
}


static status_t
pipefs_remove(pipefs *fs, pipefs_vnode *directory, const char *name)
{
	struct pipefs_vnode *vnode;
	status_t status = B_OK;

	mutex_lock(&fs->lock);

	vnode = pipefs_find(fs, name);
	if (!vnode)
		status = B_ENTRY_NOT_FOUND;
	else if (vnode->type == S_IFDIR)
		status = B_NOT_ALLOWED;

	if (status < B_OK)
		goto err;

	pipefs_remove_from_fs(fs, vnode);
	notify_listener(B_ENTRY_REMOVED, fs->id, directory->id, 0, vnode->id, name);

	// schedule this vnode to be removed when it's ref goes to zero
	vfs_remove_vnode(fs->id, vnode->id);

err:
	mutex_unlock(&fs->lock);

	return status;
}


//	#pragma mark -


static status_t
pipefs_mount(mount_id id, const char *device, void *args, fs_volume *_fs, vnode_id *_rootVnodeID)
{
	pipefs *fs;
	pipefs_vnode *vnode;
	status_t err;

	TRACE(("pipefs_mount: entry\n"));

	fs = (pipefs *)malloc(sizeof(pipefs));
	if (fs == NULL)
		return ENOMEM;

	fs->id = id;
	fs->next_vnode_id = 0;

	err = mutex_init(&fs->lock, "pipefs_mutex");
	if (err < 0)
		goto err1;

	fs->vnode_list_hash = hash_init(PIPEFS_HASH_SIZE, (addr)&vnode->hash_next - (addr)vnode,
		&pipefs_vnode_compare_func, &pipefs_vnode_hash_func);
	if (fs->vnode_list_hash == NULL) {
		err = B_NO_MEMORY;
		goto err2;
	}

	// create the root vnode
	vnode = pipefs_create_vnode(fs, "", S_IFDIR);
	if (vnode == NULL) {
		err = B_NO_MEMORY;
		goto err3;
	}

	fs->root_vnode = vnode;
	fs->first_entry = NULL;
	fs->first_dir_cookie = NULL;

	hash_insert(fs->vnode_list_hash, vnode);

	*_rootVnodeID = vnode->id;
	*_fs = fs;

	return B_OK;

err3:
	hash_uninit(fs->vnode_list_hash);
err2:
	mutex_destroy(&fs->lock);
err1:
	free(fs);

	return err;
}


static status_t
pipefs_unmount(fs_volume _fs)
{
	struct pipefs *fs = (struct pipefs *)_fs;
	struct pipefs_vnode *vnode;
	struct hash_iterator i;

	TRACE(("pipefs_unmount: entry fs = %p\n", fs));

	// put_vnode on the root to release the ref to it
	vfs_put_vnode(fs->id, fs->root_vnode->id);

	// delete all of the vnodes
	hash_open(fs->vnode_list_hash, &i);
	while ((vnode = (struct pipefs_vnode *)hash_next(fs->vnode_list_hash, &i)) != NULL) {
		pipefs_delete_vnode(fs, vnode, true);
	}
	hash_close(fs->vnode_list_hash, &i, false);

	hash_uninit(fs->vnode_list_hash);
	mutex_destroy(&fs->lock);
	free(fs);

	return 0;
}


static status_t
pipefs_sync(fs_volume fs)
{
	TRACE(("pipefs_sync: entry\n"));

	return 0;
}


static status_t
pipefs_lookup(fs_volume _fs, fs_vnode _dir, const char *name, vnode_id *_id, int *_type)
{
	pipefs *fs = (pipefs *)_fs;
	status_t status;

	TRACE(("pipefs_lookup: entry dir %p, name '%s'\n", _dir, name));

	pipefs_vnode *directory = (pipefs_vnode *)_dir;
	if (directory->type != S_IFDIR)
		return B_NOT_A_DIRECTORY;

	mutex_lock(&fs->lock);

	// look it up
	pipefs_vnode *vnode = pipefs_find(fs, name);
	if (!vnode) {
		status = B_ENTRY_NOT_FOUND;
		goto err;
	}

	pipefs_vnode *vnodeDummy;
	status = vfs_get_vnode(fs->id, vnode->id, (fs_vnode *)&vnodeDummy);
	if (status < B_OK)
		goto err;

	*_id = vnode->id;
	*_type = vnode->type;

err:
	mutex_unlock(&fs->lock);

	return status;
}


static status_t
pipefs_get_vnode_name(fs_volume _fs, fs_vnode _vnode, char *buffer, size_t bufferSize)
{
	pipefs_vnode *vnode = (pipefs_vnode *)_vnode;

	TRACE(("devfs_get_vnode_name: vnode = %p\n", vnode));

	strlcpy(buffer, vnode->name, bufferSize);
	return B_OK;
}


static status_t
pipefs_get_vnode(fs_volume _fs, vnode_id id, fs_vnode *_vnode, bool reenter)
{
	pipefs *fs = (pipefs *)_fs;

	TRACE(("pipefs_getvnode: asking for vnode 0x%Lx, r %d\n", id, reenter));

	if (!reenter)
		mutex_lock(&fs->lock);

	*_vnode = hash_lookup(fs->vnode_list_hash, &id);

	if (!reenter)
		mutex_unlock(&fs->lock);

	TRACE(("pipefs_getnvnode: looked it up at %p\n", *_vnode));

	if (*_vnode)
		return B_OK;

	return B_ENTRY_NOT_FOUND;
}


static status_t
pipefs_put_vnode(fs_volume _fs, fs_vnode _vnode, bool reenter)
{
#if PIPEFS_TRACE
	pipefs_vnode *vnode = (pipefs_vnode *)_vnode;

	TRACE(("pipefs_putvnode: entry on vnode 0x%Lx, r %d\n", vnode->id, reenter));
#endif
	return B_OK;
}


static status_t
pipefs_remove_vnode(fs_volume _fs, fs_vnode _vnode, bool reenter)
{
	pipefs *fs = (pipefs *)_fs;

	TRACE(("pipefs_removevnode: remove %p (0x%Lx), r %d\n", vnode, vnode->id, reenter));

	if (!reenter)
		mutex_lock(&fs->lock);

	pipefs_vnode *vnode = (pipefs_vnode *)_vnode;
	if (vnode->next) {
		// can't remove node if it's linked to the dir
		panic("pipefs_remove_vnode(): vnode %p asked to be removed is present in dir\n", vnode);
	}

	pipefs_delete_vnode(fs, vnode, false);

	if (!reenter)
		mutex_unlock(&fs->lock);

	return 0;
}


static status_t
pipefs_create(fs_volume _fs, fs_vnode _dir, const char *name, int omode, int perms, fs_cookie *_cookie, vnode_id *new_vnid)
{
	pipefs *fs = (pipefs *)_fs;

	TRACE(("pipefs_create_dir: dir %p, name = '%s', perms = %d, id = 0x%Lx pointer id = %p\n", dir, name, perms,*new_vnid, new_vnid));

	mutex_lock(&fs->lock);

	pipefs_vnode *directory = (pipefs_vnode *)_dir;
	status_t status = B_OK;

	pipefs_vnode *vnode = pipefs_find(fs, name);
	if (vnode != NULL) {
		status = B_FILE_EXISTS;
		goto err;
	}

	vnode = pipefs_create_vnode(fs, name, S_IFIFO);
	if (vnode == NULL) {
		status = B_NO_MEMORY;
		goto err;
	}

	pipefs_insert(fs, vnode);
	hash_insert(fs->vnode_list_hash, vnode);

	mutex_unlock(&fs->lock);	

	notify_listener(B_ENTRY_CREATED, fs->id, directory->id, 0, vnode->id, name);
	return B_OK;

err1:
	pipefs_delete_vnode(fs, vnode, false);
err:
	mutex_unlock(&fs->lock);

	return status;
}


static status_t
pipefs_open(fs_volume _fs, fs_vnode _v, int openMode, fs_cookie *_cookie)
{
	// allow to open the file, but it can't be done anything with it

	*_cookie = NULL;
		// initialize the cookie, because pipefs_free_cookie() relies on it

	return B_OK;
}


static status_t
pipefs_close(fs_volume _fs, fs_vnode _vnode, fs_cookie _cookie)
{
	TRACE(("pipefs_close: entry vnode %p, cookie %p\n", _vnode, _cookie));

	return 0;
}


static status_t
pipefs_free_cookie(fs_volume _fs, fs_vnode _vnode, fs_cookie _cookie)
{
//	pipefs_cookie *cookie = _cookie;

	TRACE(("pipefs_freecookie: entry vnode %p, cookie %p\n", _vnode, _cookie));

//	if (cookie)
//		free(cookie);

	return 0;
}


static status_t
pipefs_fsync(fs_volume _fs, fs_vnode _v)
{
	return B_OK;
}


static ssize_t
pipefs_read(fs_volume _fs, fs_vnode _vnode, fs_cookie _cookie, off_t pos,
	void *buffer, size_t *_length)
{
	TRACE(("pipefs_read: vnode %p, cookie %p, pos 0x%Lx , len 0x%lx\n", vnode, cookie, pos, *len));

	return EINVAL;
}


static ssize_t
pipefs_write(fs_volume fs, fs_vnode vnode, fs_cookie cookie, off_t pos,
	const void *buffer, size_t *_length)
{
	TRACE(("pipefs_write: vnode %p, cookie %p, pos 0x%Lx , len 0x%lx\n", vnode, cookie, pos, *len));

	return EINVAL;
}


static off_t
pipefs_seek(fs_volume _fs, fs_vnode _vnode, fs_cookie _cookie, off_t pos, int seekType)
{
	return ESPIPE;
}


static status_t
pipefs_create_dir(fs_volume _fs, fs_vnode _dir, const char *name, int perms, vnode_id *_newID)
{
	return ENOSYS;
}


static status_t
pipefs_remove_dir(fs_volume _fs, fs_vnode _dir, const char *name)
{
	return EPERM;
}


static status_t
pipefs_open_dir(fs_volume _fs, fs_vnode _vnode, fs_cookie *_cookie)
{
	pipefs *fs = (pipefs *)_fs;

	TRACE(("pipefs_open_dir(): vnode = %p\n", _vnode));

	pipefs_vnode *vnode = (pipefs_vnode *)_vnode;
	if (vnode->type != S_IFDIR)
		return B_BAD_VALUE;

	if (vnode != fs->root_vnode)
		panic("pipefs: found directory that's not the root!");

	dir_cookie *cookie = (dir_cookie *)malloc(sizeof(dir_cookie));
	if (cookie == NULL)
		return B_NO_MEMORY;

	mutex_lock(&fs->lock);

	cookie->current = fs->first_entry;

	insert_cookie_in_jar(fs, cookie);
	*_cookie = cookie;

	mutex_unlock(&fs->lock);

	return B_OK;
}


static status_t
pipefs_read_dir(fs_volume _fs, fs_vnode _vnode, fs_cookie _cookie,
	struct dirent *dirent, size_t bufferSize, uint32 *_num)
{
	pipefs *fs = (pipefs *)_fs;
	status_t status = 0;

	TRACE(("pipefs_read_dir: vnode %p, cookie %p, buffer = %p, bufferSize = %ld, num = %p\n", _vnode, cookie, dirent, bufferSize,_num));

	mutex_lock(&fs->lock);

	dir_cookie *cookie = (dir_cookie *)_cookie;
	if (cookie->current == NULL) {
		// we're at the end of the directory
		*_num = 0;
		status = B_OK;
		goto err;
	}

	dirent->d_dev = fs->id;
	dirent->d_ino = cookie->current->id;
	dirent->d_reclen = strlen(cookie->current->name) + sizeof(struct dirent);

	if (dirent->d_reclen > bufferSize) {
		status = ENOBUFS;
		goto err;
	}

	status = user_strlcpy(dirent->d_name, cookie->current->name, bufferSize);
	if (status < 0)
		goto err;

	cookie->current = cookie->current->next;

err:
	mutex_unlock(&fs->lock);

	return status;
}


static status_t
pipefs_rewind_dir(fs_volume _fs, fs_vnode _vnode, fs_cookie _cookie)
{
	pipefs *fs = (pipefs *)_fs;

	mutex_lock(&fs->lock);

	dir_cookie *cookie = (dir_cookie *)_cookie;
	cookie->current = fs->first_entry;

	mutex_unlock(&fs->lock);
	return B_OK;
}


static status_t
pipefs_close_dir(fs_volume _fs, fs_vnode _v, fs_cookie _cookie)
{
	TRACE(("pipefs_close: entry vnode %p, cookie %p\n", _vnode, _cookie));

	return 0;
}


static status_t
pipefs_free_dir_cookie(fs_volume _fs, fs_vnode _vnode, fs_cookie _cookie)
{
	dir_cookie *cookie = (dir_cookie *)_cookie;

	TRACE(("pipefs_freecookie: entry vnode %p, cookie %p\n", _vnode, cookie));

	free(cookie);
	return 0;
}


static status_t
pipefs_ioctl(fs_volume _fs, fs_vnode _vnode, fs_cookie _cookie, ulong op,
	void *buffer, size_t length)
{
	TRACE(("pipefs_ioctl: vnode %p, cookie %p, op %ld, buf %p, len %ld\n",
		_vnode, _cookie, op, buffer, length));

	return EINVAL;
}


static status_t
pipefs_can_page(fs_volume _fs, fs_vnode _v)
{
	return -1;
}


static ssize_t
pipefs_read_pages(fs_volume _fs, fs_vnode _v, iovecs *vecs, off_t pos)
{
	return EPERM;
}


static ssize_t
pipefs_write_pages(fs_volume _fs, fs_vnode _v, iovecs *vecs, off_t pos)
{
	return EPERM;
}


static status_t
pipefs_unlink(fs_volume _fs, fs_vnode _dir, const char *name)
{
	pipefs *fs = (pipefs *)_fs;
	pipefs_vnode *directory = (pipefs_vnode *)_dir;

	TRACE(("pipefs_unlink: dir %p (0x%Lx), name '%s'\n", _dir, directory->id, name));

	return pipefs_remove(fs, directory, name);
}


static status_t
pipefs_read_stat(fs_volume _fs, fs_vnode _vnode, struct stat *stat)
{
	pipefs *fs = (pipefs *)_fs;
	pipefs_vnode *vnode = (pipefs_vnode *)_vnode;

	TRACE(("pipefs_rstat: vnode %p (0x%Lx), stat %p\n", vnode, vnode->id, stat));

	stat->st_dev = fs->id;
	stat->st_ino = vnode->id;
	stat->st_size = 0;	// ToDo: oughta get me!
	stat->st_mode = vnode->type | DEFFILEMODE;

	return 0;
}


//	#pragma mark -


static struct fs_ops pipefs_ops = {
	&pipefs_mount,
	&pipefs_unmount,
	NULL,
	NULL,
	&pipefs_sync,

	&pipefs_lookup,
	&pipefs_get_vnode_name,

	&pipefs_get_vnode,
	&pipefs_put_vnode,
	&pipefs_remove_vnode,

	&pipefs_can_page,
	&pipefs_read_pages,
	&pipefs_write_pages,

	/* common */
	&pipefs_ioctl,
	&pipefs_fsync,

	NULL,	// fs_read_link()
	NULL,	// fs_write_link()
	NULL,	// fs_symlink()
	NULL,	// fs_link()
	&pipefs_unlink,
	NULL,	// fs_rename()

	NULL,	// fs_access()
	&pipefs_read_stat,
	NULL,	// fs_write_stat()

	/* file */
	&pipefs_create,
	&pipefs_open,
	&pipefs_close,
	&pipefs_free_cookie,
	&pipefs_read,
	&pipefs_write,

	/* directory */
	&pipefs_create_dir,
	&pipefs_remove_dir,
	&pipefs_open_dir,
	&pipefs_close_dir,
	&pipefs_free_dir_cookie,
	&pipefs_read_dir,
	&pipefs_rewind_dir,

	// the other operations are not supported (attributes, indices, queries)
	NULL,
};


extern "C" status_t
bootstrap_pipefs(void)
{
	dprintf("bootstrap_pipefs: entry\n");

	return vfs_register_filesystem("pipefs", &pipefs_ops);
}
