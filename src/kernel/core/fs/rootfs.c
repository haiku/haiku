/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <KernelExport.h>
#include <vfs.h>
#include <debug.h>
#include <khash.h>
#include <lock.h>
#include <vm.h>

#include <NodeMonitor.h>

#include <sys/stat.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>

#include "builtin_fs.h"


#define ROOTFS_TRACE 0

#if ROOTFS_TRACE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x)
#endif

typedef enum {
	STREAM_TYPE_DIR = S_IFDIR,
	STREAM_TYPE_SYMLINK = S_IFLNK,
} stream_type;

struct rootfs_stream {
	stream_type type;
	struct stream_dir {
		struct rootfs_vnode *dir_head;
		struct rootfs_cookie *jar_head;
	} dir;
	struct stream_symlink {
		char *path;
		size_t length;
	} symlink;
};

struct rootfs_vnode {
	struct rootfs_vnode *all_next;
	vnode_id id;
	char *name;
	struct rootfs_vnode *parent;
	struct rootfs_vnode *dir_next;
	struct rootfs_stream stream;
};

struct rootfs {
	mount_id id;
	mutex lock;
	vnode_id next_vnode_id;
	hash_table *vnode_list_hash;
	struct rootfs_vnode *root_vnode;
};

// dircookie, dirs are only types of streams supported by rootfs
struct rootfs_cookie {
	struct rootfs_cookie *next;
	struct rootfs_cookie *prev;
	struct rootfs_vnode *ptr;
	int oflags;
};

#define ROOTFS_HASH_SIZE 16


static uint32
rootfs_vnode_hash_func(void *_v, const void *_key, uint32 range)
{
	struct rootfs_vnode *vnode = _v;
	const vnode_id *key = _key;

	if (vnode != NULL)
		return vnode->id % range;

	return (*key) % range;
}


static int
rootfs_vnode_compare_func(void *_v, const void *_key)
{
	struct rootfs_vnode *v = _v;
	const vnode_id *key = _key;

	if (v->id == *key)
		return 0;

	return -1;
}


static struct rootfs_vnode *
rootfs_create_vnode(struct rootfs *fs, const char *name, int type)
{
	struct rootfs_vnode *vnode;

	vnode = malloc(sizeof(struct rootfs_vnode));
	if (vnode == NULL)
		return NULL;

	memset(vnode, 0, sizeof(struct rootfs_vnode));

	vnode->name = strdup(name);
	if (vnode->name == NULL) {
		free(vnode);
		return NULL;
	}

	vnode->id = fs->next_vnode_id++;
	vnode->stream.type = type;

	return vnode;
}


static status_t
rootfs_delete_vnode(struct rootfs *fs, struct rootfs_vnode *v, bool force_delete)
{
	// cant delete it if it's in a directory or is a directory
	// and has children
	if (!force_delete && (v->stream.dir.dir_head != NULL || v->dir_next != NULL))
		return EPERM;

	// remove it from the global hash table
	hash_remove(fs->vnode_list_hash, v);

	if (v->name != NULL)
		free(v->name);
	free(v);

	return 0;
}


static void
insert_cookie_in_jar(struct rootfs_vnode *dir, struct rootfs_cookie *cookie)
{
	cookie->next = dir->stream.dir.jar_head;
	dir->stream.dir.jar_head = cookie;
	cookie->prev = NULL;
}


#if 0
static void
remove_cookie_from_jar(struct rootfs_vnode *dir, struct rootfs_cookie *cookie)
{
	// ToDo: why is this function not called?

	if (cookie->next)
		cookie->next->prev = cookie->prev;
	if (cookie->prev)
		cookie->prev->next = cookie->next;
	if (dir->stream.dir.jar_head == cookie)
		dir->stream.dir.jar_head = cookie->next;

	cookie->prev = cookie->next = NULL;
}
#endif


/* makes sure none of the dircookies point to the vnode passed in */

static void
update_dircookies(struct rootfs_vnode *dir, struct rootfs_vnode *v)
{
	struct rootfs_cookie *cookie;

	for (cookie = dir->stream.dir.jar_head; cookie; cookie = cookie->next) {
		if (cookie->ptr == v)
			cookie->ptr = v->dir_next;
	}
}


static struct rootfs_vnode *
rootfs_find_in_dir(struct rootfs_vnode *dir, const char *path)
{
	struct rootfs_vnode *v;

	if (!strcmp(path, "."))
		return dir;
	if (!strcmp(path, ".."))
		return dir->parent;

	for (v = dir->stream.dir.dir_head; v; v = v->dir_next) {
		if (strcmp(v->name, path) == 0)
			return v;
	}
	return NULL;
}


static status_t
rootfs_insert_in_dir(struct rootfs_vnode *dir, struct rootfs_vnode *v)
{
	v->dir_next = dir->stream.dir.dir_head;
	dir->stream.dir.dir_head = v;
	return 0;
}


static status_t
rootfs_remove_from_dir(struct rootfs_vnode *dir, struct rootfs_vnode *findit)
{
	struct rootfs_vnode *v;
	struct rootfs_vnode *last_v;

	for (v = dir->stream.dir.dir_head, last_v = NULL; v; last_v = v, v = v->dir_next) {
		if (v == findit) {
			/* make sure all dircookies dont point to this vnode */
			update_dircookies(dir, v);

			if (last_v)
				last_v->dir_next = v->dir_next;
			else
				dir->stream.dir.dir_head = v->dir_next;
			v->dir_next = NULL;
			return 0;
		}
	}
	return -1;
}


static bool
rootfs_is_dir_empty(struct rootfs_vnode *dir)
{
	return !dir->stream.dir.dir_head;
}


static status_t
rootfs_remove(struct rootfs *fs, struct rootfs_vnode *dir, const char *name, bool isDirectory)
{
	struct rootfs_vnode *vnode;
	status_t status = B_OK;

	mutex_lock(&fs->lock);

	vnode = rootfs_find_in_dir(dir, name);
	if (!vnode)
		status = B_ENTRY_NOT_FOUND;
	else if (isDirectory && vnode->stream.type != STREAM_TYPE_DIR)
		status = B_NOT_A_DIRECTORY;
	else if (!isDirectory && vnode->stream.type == STREAM_TYPE_DIR)
		status = B_IS_A_DIRECTORY;
	else if (isDirectory && !rootfs_is_dir_empty(vnode))
		status = B_DIRECTORY_NOT_EMPTY;

	if (status < B_OK)
		goto err;

	rootfs_remove_from_dir(dir, vnode);
	notify_listener(B_ENTRY_REMOVED, fs->id, dir->id, 0, vnode->id, name);

	// schedule this vnode to be removed when it's ref goes to zero
	vfs_remove_vnode(fs->id, vnode->id);

err:
	mutex_unlock(&fs->lock);

	return status;
}


//	#pragma mark -


static status_t
rootfs_mount(mount_id id, const char *device, void *args, fs_volume *_fs, vnode_id *root_vnid)
{
	struct rootfs *fs;
	struct rootfs_vnode *vnode;
	status_t err;

	TRACE(("rootfs_mount: entry\n"));

	fs = malloc(sizeof(struct rootfs));
	if (fs == NULL)
		return B_NO_MEMORY;

	fs->id = id;
	fs->next_vnode_id = 0;

	err = mutex_init(&fs->lock, "rootfs_mutex");
	if (err < 0)
		goto err1;

	fs->vnode_list_hash = hash_init(ROOTFS_HASH_SIZE, (addr)&vnode->all_next - (addr)vnode,
		&rootfs_vnode_compare_func, &rootfs_vnode_hash_func);
	if (fs->vnode_list_hash == NULL) {
		err = B_NO_MEMORY;
		goto err2;
	}

	// create the root vnode
	vnode = rootfs_create_vnode(fs, "", STREAM_TYPE_DIR);
	if (vnode == NULL) {
		err = B_NO_MEMORY;
		goto err3;
	}
	vnode->parent = vnode;

	fs->root_vnode = vnode;
	hash_insert(fs->vnode_list_hash, vnode);

	*root_vnid = vnode->id;
	*_fs = fs;

	return 0;

err3:
	hash_uninit(fs->vnode_list_hash);
err2:
	mutex_destroy(&fs->lock);
err1:
	free(fs);

	return err;
}


static status_t
rootfs_unmount(fs_volume _fs)
{
	struct rootfs *fs = (struct rootfs *)_fs;
	struct rootfs_vnode *v;
	struct hash_iterator i;

	TRACE(("rootfs_unmount: entry fs = %p\n", fs));

	// put_vnode on the root to release the ref to it
	vfs_put_vnode(fs->id, fs->root_vnode->id);

	// delete all of the vnodes
	hash_open(fs->vnode_list_hash, &i);
	while ((v = (struct rootfs_vnode *)hash_next(fs->vnode_list_hash, &i)) != NULL) {
		rootfs_delete_vnode(fs, v, true);
	}
	hash_close(fs->vnode_list_hash, &i, false);

	hash_uninit(fs->vnode_list_hash);
	mutex_destroy(&fs->lock);
	free(fs);

	return 0;
}


static status_t
rootfs_sync(fs_volume fs)
{
	TRACE(("rootfs_sync: entry\n"));

	return 0;
}


static status_t
rootfs_lookup(fs_volume _fs, fs_vnode _dir, const char *name, vnode_id *_id, int *_type)
{
	struct rootfs *fs = (struct rootfs *)_fs;
	struct rootfs_vnode *dir = (struct rootfs_vnode *)_dir;
	struct rootfs_vnode *vnode,*vdummy;
	status_t status;

	TRACE(("rootfs_lookup: entry dir %p, name '%s'\n", dir, name));
	if (dir->stream.type != STREAM_TYPE_DIR)
		return B_NOT_A_DIRECTORY;

	mutex_lock(&fs->lock);

	// look it up
	vnode = rootfs_find_in_dir(dir, name);
	if (!vnode) {
		status = B_ENTRY_NOT_FOUND;
		goto err;
	}

	status = vfs_get_vnode(fs->id, vnode->id, (fs_vnode *)&vdummy);
	if (status < 0)
		goto err;

	*_id = vnode->id;
	*_type = vnode->stream.type;

err:
	mutex_unlock(&fs->lock);

	return status;
}


static status_t
rootfs_get_vnode_name(fs_volume _fs, fs_vnode _vnode, char *buffer, size_t bufferSize)
{
	struct rootfs_vnode *vnode = (struct rootfs_vnode *)_vnode;

	TRACE(("devfs_get_vnode_name: vnode = %p\n",vnode));
	
	strlcpy(buffer,vnode->name,bufferSize);
	return B_OK;
}


static status_t
rootfs_get_vnode(fs_volume _fs, vnode_id id, fs_vnode *_vnode, bool reenter)
{
	struct rootfs *fs = (struct rootfs *)_fs;

	TRACE(("rootfs_getvnode: asking for vnode 0x%Lx, r %d\n", id, reenter));

	if (!reenter)
		mutex_lock(&fs->lock);

	*_vnode = hash_lookup(fs->vnode_list_hash, &id);

	if (!reenter)
		mutex_unlock(&fs->lock);

	TRACE(("rootfs_getnvnode: looked it up at %p\n", *_vnode));

	if (*_vnode)
		return B_NO_ERROR;

	return ENOENT;
}


static status_t
rootfs_put_vnode(fs_volume _fs, fs_vnode _vnode, bool reenter)
{
#if ROOTFS_TRACE
	struct rootfs_vnode *vnode = (struct rootfs_vnode *)_vnode;

	TRACE(("rootfs_putvnode: entry on vnode 0x%Lx, r %d\n", vnode->id, reenter));
#endif
	return 0; // whatever
}


static status_t
rootfs_remove_vnode(fs_volume _fs, fs_vnode _vnode, bool reenter)
{
	struct rootfs *fs = (struct rootfs *)_fs;
	struct rootfs_vnode *vnode = (struct rootfs_vnode *)_vnode;

	TRACE(("rootfs_removevnode: remove %p (0x%Lx), r %d\n", vnode, vnode->id, reenter));

	if (!reenter)
		mutex_lock(&fs->lock);

	if (vnode->dir_next) {
		// can't remove node if it's linked to the dir
		panic("rootfs_removevnode: vnode %p asked to be removed is present in dir\n", vnode);
	}

	rootfs_delete_vnode(fs, vnode, false);

	if (!reenter)
		mutex_unlock(&fs->lock);

	return 0;
}


static status_t
rootfs_create(fs_volume _fs, fs_vnode _dir, const char *name, int omode, int perms, fs_cookie *_cookie, vnode_id *new_vnid)
{
	return EINVAL;
}


static status_t
rootfs_open(fs_volume _fs, fs_vnode _v, int oflags, fs_cookie *_cookie)
{
	// allow to open the file, but it can't be done anything with it

	*_cookie = NULL;
		// initialize the cookie, because rootfs_free_cookie() relies on it

	return B_OK;
}


static status_t
rootfs_close(fs_volume _fs, fs_vnode _v, fs_cookie _cookie)
{
#if ROOTFS_TRACE
	struct rootfs_vnode *v = _v;
	struct rootfs_cookie *cookie = _cookie;

	TRACE(("rootfs_close: entry vnode %p, cookie %p\n", v, cookie));
#endif
	return 0;
}


static status_t
rootfs_free_cookie(fs_volume _fs, fs_vnode _v, fs_cookie _cookie)
{
	struct rootfs_cookie *cookie = _cookie;
#if ROOTFS_TRACE
	struct rootfs_vnode *v = _v;

	TRACE(("rootfs_freecookie: entry vnode %p, cookie %p\n", v, cookie));
#endif
	if (cookie)
		free(cookie);

	return 0;
}


static status_t
rootfs_fsync(fs_volume _fs, fs_vnode _v)
{
	return 0;
}


static ssize_t
rootfs_read(fs_volume _fs, fs_vnode _v, fs_cookie _cookie, off_t pos, void *buf, size_t *len)
{
	return EINVAL;
}


static ssize_t
rootfs_write(fs_volume fs, fs_vnode v, fs_cookie cookie, off_t pos, const void *buf, size_t *len)
{
	TRACE(("rootfs_write: vnode %p, cookie %p, pos 0x%Lx , len 0x%lx\n", v, cookie, pos, *len));

	return EPERM;
}


static status_t
rootfs_create_dir(fs_volume _fs, fs_vnode _dir, const char *name, int perms, vnode_id *_newID)
{
	struct rootfs *fs = _fs;
	struct rootfs_vnode *dir = _dir;
	struct rootfs_vnode *vnode;
	status_t status = 0;

	TRACE(("rootfs_create_dir: dir %p, name = '%s', perms = %d, id = 0x%Lx pointer id = %p\n", dir, name, perms,*new_vnid, new_vnid));

	mutex_lock(&fs->lock);

	vnode = rootfs_find_in_dir(dir, name);
	if (vnode != NULL) {
		status = B_FILE_EXISTS;
		goto err;
	}

	dprintf("rootfs_create: creating new vnode\n");
	vnode = rootfs_create_vnode(fs, name, STREAM_TYPE_DIR);
	if (vnode == NULL) {
		status = B_NO_MEMORY;
		goto err;
	}
	vnode->parent = dir;
	rootfs_insert_in_dir(dir, vnode);

	hash_insert(fs->vnode_list_hash, vnode);

	vnode->stream.dir.dir_head = NULL;
	vnode->stream.dir.jar_head = NULL;

	notify_listener(B_ENTRY_CREATED, fs->id, dir->id, 0, vnode->id, name);

	mutex_unlock(&fs->lock);	
	return 0;

err:
	mutex_unlock(&fs->lock);

	return status;
}


static status_t
rootfs_remove_dir(fs_volume _fs, fs_vnode _dir, const char *name)
{
	struct rootfs *fs = _fs;
	struct rootfs_vnode *dir = _dir;

	TRACE(("rootfs_remove_dir: dir %p (0x%Lx), name '%s'\n", dir, dir->id, name));

	return rootfs_remove(fs, dir, name, true);
}


static status_t
rootfs_open_dir(fs_volume _fs, fs_vnode _v, fs_cookie *_cookie)
{
	struct rootfs *fs = (struct rootfs *)_fs;
	struct rootfs_vnode *vnode = (struct rootfs_vnode *)_v;
	struct rootfs_cookie *cookie;

	TRACE(("rootfs_open: vnode %p\n", vnode));

	if (vnode->stream.type != STREAM_TYPE_DIR)
		return B_BAD_VALUE;

	cookie = malloc(sizeof(struct rootfs_cookie));
	if (cookie == NULL)
		return B_NO_MEMORY;

	mutex_lock(&fs->lock);

	cookie->ptr = vnode->stream.dir.dir_head;
	//cookie->oflags = oflags;

	insert_cookie_in_jar(vnode, cookie);
	*_cookie = cookie;

	mutex_unlock(&fs->lock);

	return B_OK;
}


static status_t
rootfs_read_dir(fs_volume _fs, fs_vnode _vnode, fs_cookie _cookie, struct dirent *dirent, size_t bufferSize, uint32 *_num)
{
	struct rootfs_cookie *cookie = _cookie;
	struct rootfs *fs = _fs;
	status_t status = B_OK;

	TRACE(("rootfs_read_dir: vnode %p, cookie %p, buffer = %p, bufferSize = %ld, num = %p\n", _vnode, cookie, dirent, bufferSize,_num));

	mutex_lock(&fs->lock);

	if (cookie->ptr == NULL) {
		// we're at the end of the directory
		*_num = 0;
		goto err;
	}

	dirent->d_dev = fs->id;
	dirent->d_ino = cookie->ptr->id;
	dirent->d_reclen = strlen(cookie->ptr->name) + sizeof(struct dirent);

	if (dirent->d_reclen > bufferSize) {
		status = ENOBUFS;
		goto err;
	}

	status = user_strlcpy(dirent->d_name, cookie->ptr->name, bufferSize - sizeof(struct dirent));
	if (status < B_OK)
		goto err;

	cookie->ptr = cookie->ptr->dir_next;
	status = B_OK;

err:
	mutex_unlock(&fs->lock);

	return status;
}


static status_t
rootfs_rewind_dir(fs_volume _fs, fs_vnode _vnode, fs_cookie _cookie)
{
	struct rootfs *fs = _fs;
	struct rootfs_vnode *vnode = _vnode;
	struct rootfs_cookie *cookie = _cookie;

	mutex_lock(&fs->lock);

	cookie->ptr = vnode->stream.dir.dir_head;

	mutex_unlock(&fs->lock);
	return B_OK;
}


static status_t
rootfs_ioctl(fs_volume _fs, fs_vnode _v, fs_cookie _cookie, ulong op, void *buf, size_t len)
{
	TRACE(("rootfs_ioctl: vnode %p, cookie %p, op %ld, buf %p, len %ld\n", _v, _cookie, op, buf, len));

	return EINVAL;
}


static status_t
rootfs_can_page(fs_volume _fs, fs_vnode _v)
{
	return -1;
}


static ssize_t
rootfs_read_pages(fs_volume _fs, fs_vnode _v, iovecs *vecs, off_t pos)
{
	return EPERM;
}


static ssize_t
rootfs_write_pages(fs_volume _fs, fs_vnode _v, iovecs *vecs, off_t pos)
{
	return EPERM;
}


static status_t
rootfs_read_link(fs_volume _fs, fs_vnode _link, char *buffer, size_t bufferSize)
{
	struct rootfs_vnode *link = _link;
	
	if (link->stream.type != STREAM_TYPE_SYMLINK)
		return B_BAD_VALUE;

	if (bufferSize < link->stream.symlink.length)
		return B_NAME_TOO_LONG;

	memcpy(buffer, link->stream.symlink.path, link->stream.symlink.length + 1);
	return link->stream.symlink.length;
}


static status_t
rootfs_symlink(fs_volume _fs, fs_vnode _dir, const char *name, const char *path, int mode)
{
	struct rootfs *fs = _fs;
	struct rootfs_vnode *dir = _dir;
	struct rootfs_vnode *vnode;
	status_t status = 0;

	TRACE(("rootfs_symlink: dir %p, name = '%s', path = %s\n", dir, name, path));

	mutex_lock(&fs->lock);

	vnode = rootfs_find_in_dir(dir, name);
	if (vnode != NULL) {
		status = B_FILE_EXISTS;
		goto err;
	}

	dprintf("rootfs_create: creating new symlink\n");
	vnode = rootfs_create_vnode(fs, name, STREAM_TYPE_SYMLINK);
	if (vnode == NULL) {
		status = B_NO_MEMORY;
		goto err;
	}
	vnode->parent = dir;
	rootfs_insert_in_dir(dir, vnode);

	hash_insert(fs->vnode_list_hash, vnode);

	vnode->stream.symlink.path = strdup(path);
	if (vnode->stream.symlink.path == NULL) {
		status = B_NO_MEMORY;
		goto err1;
	}
	vnode->stream.symlink.length = strlen(path);

	notify_listener(B_ENTRY_CREATED, fs->id, dir->id, 0, vnode->id, name);

	mutex_unlock(&fs->lock);
	return 0;

err1:
	rootfs_delete_vnode(fs, vnode, false);
err:
	mutex_unlock(&fs->lock);
	return status;
}


static status_t
rootfs_unlink(fs_volume _fs, fs_vnode _dir, const char *name)
{
	struct rootfs *fs = _fs;
	struct rootfs_vnode *dir = _dir;

	TRACE(("rootfs_unlink: dir %p (0x%Lx), name '%s'\n", dir, dir->id, name));

	return rootfs_remove(fs, dir, name, false);
}


static status_t
rootfs_rename(fs_volume _fs, fs_vnode _olddir, const char *oldname, fs_vnode _newdir, const char *newname)
{
	struct rootfs *fs = _fs;
	struct rootfs_vnode *olddir = _olddir;
	struct rootfs_vnode *newdir = _newdir;
	struct rootfs_vnode *v1, *v2;
	status_t err;

	TRACE(("rootfs_rename: olddir %p (0x%Lx), oldname '%s', newdir %p (0x%Lx), newname '%s'\n",
		olddir, olddir->id, oldname, newdir, newdir->id, newname));

	mutex_lock(&fs->lock);

	v1 = rootfs_find_in_dir(olddir, oldname);
	if (!v1) {
		err = B_ENTRY_NOT_FOUND;
		goto err;
	}

	v2 = rootfs_find_in_dir(newdir, newname);

	if (olddir == newdir) {
		// rename to a different name in the same dir
		if (v2) {
			// target node exists
			err = B_NAME_IN_USE;
			goto err;
		}

		// change the name on this node
		if (strlen(oldname) >= strlen(newname)) {
			// reuse the old name buffer
			strcpy(v1->name, newname);
		} else {
			char *ptr = v1->name;

			v1->name = strdup(newname);
			if (!v1->name) {
				// bad place to be, at least restore
				v1->name = ptr;
				err = B_NO_MEMORY;
				goto err;
			}
			free(ptr);
		}

		/* no need to remove and add it unless the dir is sorting */
#if 0
		// remove it from the dir
		rootfs_remove_from_dir(olddir, v1);

		// add it back to the dir with the new name
		rootfs_insert_in_dir(newdir, v1);
#endif
	} else {
		// different target dir from source

		rootfs_remove_from_dir(olddir, v1);

		rootfs_insert_in_dir(newdir, v1);
	}

	err = 0;

err:
	mutex_unlock(&fs->lock);

	return err;
}


static status_t
rootfs_read_stat(fs_volume _fs, fs_vnode _v, struct stat *stat)
{
	struct rootfs *fs = _fs;
	struct rootfs_vnode *vnode = _v;

	TRACE(("rootfs_rstat: vnode %p (0x%Lx), stat %p\n", vnode, vnode->id, stat));

	// stream exists, but we know to return size 0, since we can only hold directories
	stat->st_dev = fs->id;
	stat->st_ino = vnode->id;
	stat->st_size = 0;
	stat->st_mode = vnode->stream.type | DEFFILEMODE;

	return 0;
}


static status_t
rootfs_write_stat(fs_volume _fs, fs_vnode _v, const struct stat *stat, int stat_mask)
{
#if ROOTFS_TRACE
	struct rootfs *fs = _fs;
	struct rootfs_vnode *v = _v;

	TRACE(("rootfs_wstat: vnode %p (0x%Lx), stat %p\n", v, v->id, stat));
#endif
	// cannot change anything
	return EINVAL;
}


//	#pragma mark -


static struct fs_ops rootfs_ops = {
	&rootfs_mount,
	&rootfs_unmount,
	NULL,
	NULL,
	&rootfs_sync,

	&rootfs_lookup,
	&rootfs_get_vnode_name,

	&rootfs_get_vnode,
	&rootfs_put_vnode,
	&rootfs_remove_vnode,

	&rootfs_can_page,
	&rootfs_read_pages,
	&rootfs_write_pages,

	/* common */
	&rootfs_ioctl,
	&rootfs_fsync,

	&rootfs_read_link,
	NULL,	// fs_write_link()
	&rootfs_symlink,
	NULL,	// fs_link()
	&rootfs_unlink,
	&rootfs_rename,

	NULL,	// fs_access()
	&rootfs_read_stat,
	&rootfs_write_stat,

	/* file */
	&rootfs_create,
	&rootfs_open,
	&rootfs_close,
	&rootfs_free_cookie,
	&rootfs_read,
	&rootfs_write,

	/* directory */
	&rootfs_create_dir,
	&rootfs_remove_dir,
	&rootfs_open_dir,
	&rootfs_close,			// we are using the same operations for directories
	&rootfs_free_cookie,	// and files here - that's intended, not by accident
	&rootfs_read_dir,
	&rootfs_rewind_dir,

	// the other operations are not supported (attributes, indices, queries)
	NULL,
};


status_t
bootstrap_rootfs(void)
{
	dprintf("bootstrap_rootfs: entry\n");

	return vfs_register_filesystem("rootfs", &rootfs_ops);
}
