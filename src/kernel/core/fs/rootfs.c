/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <kernel.h>
#include <vfs.h>
#include <debug.h>
#include <khash.h>
#include <memheap.h>
#include <lock.h>
#include <vm.h>
#include <Errors.h>
#include <kerrors.h>
#include <sys/stat.h>

#include <rootfs.h>

#include <string.h>
#include <stdio.h>

#define ROOTFS_TRACE 0

#if ROOTFS_TRACE
#define TRACE(x) dprintf x
#else
#define TRACE(x)
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
		int length;
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
	fs_id id;
	mutex lock;
	vnode_id next_vnode_id;
	void *vnode_list_hash;
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


static unsigned int
rootfs_vnode_hash_func(void *_v, const void *_key, unsigned int range)
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
rootfs_create_vnode(struct rootfs *fs)
{
	struct rootfs_vnode *v;

	v = kmalloc(sizeof(struct rootfs_vnode));
	if (v == NULL)
		return NULL;

	memset(v, 0, sizeof(struct rootfs_vnode));
	v->id = fs->next_vnode_id++;

	return v;
}


static int
rootfs_delete_vnode(struct rootfs *fs, struct rootfs_vnode *v, bool force_delete)
{
	// cant delete it if it's in a directory or is a directory
	// and has children
	if (!force_delete && (v->stream.dir.dir_head != NULL || v->dir_next != NULL))
		return EPERM;

	// remove it from the global hash table
	hash_remove(fs->vnode_list_hash, v);

	if (v->name != NULL)
		kfree(v->name);
	kfree(v);

	return 0;
}


static void
insert_cookie_in_jar(struct rootfs_vnode *dir, struct rootfs_cookie *cookie)
{
	cookie->next = dir->stream.dir.jar_head;
	dir->stream.dir.jar_head = cookie;
	cookie->prev = NULL;
}


static void
remove_cookie_from_jar(struct rootfs_vnode *dir, struct rootfs_cookie *cookie)
{
	if(cookie->next)
		cookie->next->prev = cookie->prev;
	if(cookie->prev)
		cookie->prev->next = cookie->next;
	if(dir->stream.dir.jar_head == cookie)
		dir->stream.dir.jar_head = cookie->next;

	cookie->prev = cookie->next = NULL;
}

/* makes sure none of the dircookies point to the vnode passed in */
static void update_dircookies(struct rootfs_vnode *dir, struct rootfs_vnode *v)
{
	struct rootfs_cookie *cookie;

	for (cookie = dir->stream.dir.jar_head; cookie; cookie = cookie->next) {
		if(cookie->ptr == v)
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


static int
rootfs_insert_in_dir(struct rootfs_vnode *dir, struct rootfs_vnode *v)
{
	v->dir_next = dir->stream.dir.dir_head;
	dir->stream.dir.dir_head = v;
	return 0;
}


static int
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


static int
rootfs_is_dir_empty(struct rootfs_vnode *dir)
{
	return !dir->stream.dir.dir_head;
}


static int
rootfs_remove(struct rootfs *fs, struct rootfs_vnode *dir, const char *name, bool isDirectory)
{
	struct rootfs_vnode *vnode;
	int status = B_OK;

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

	// schedule this vnode to be removed when it's ref goes to zero
	vfs_remove_vnode(fs->id, vnode->id);

err:
	mutex_unlock(&fs->lock);

	return status;
}


//	#pragma mark -


static int
rootfs_mount(fs_id id, const char *device, void *args, fs_cookie *_fs, vnode_id *root_vnid)
{
	struct rootfs *fs;
	struct rootfs_vnode *vnode;
	int err;

	TRACE(("rootfs_mount: entry\n"));

	fs = kmalloc(sizeof(struct rootfs));
	if (fs == NULL)
		return ENOMEM;

	fs->id = id;
	fs->next_vnode_id = 0;

	err = mutex_init(&fs->lock, "rootfs_mutex");
	if (err < 0)
		goto err1;

	fs->vnode_list_hash = hash_init(ROOTFS_HASH_SIZE, (addr)&vnode->all_next - (addr)vnode,
		&rootfs_vnode_compare_func, &rootfs_vnode_hash_func);
	if (fs->vnode_list_hash == NULL) {
		err = ENOMEM;
		goto err2;
	}

	// create the root vnode
	vnode = rootfs_create_vnode(fs);
	if (vnode == NULL) {
		err = ENOMEM;
		goto err3;
	}

	// set it up
	vnode->parent = vnode;
	vnode->name = kstrdup("");
	if (vnode->name == NULL) {
		err = ENOMEM;
		goto err4;
	}

	vnode->stream.type = STREAM_TYPE_DIR;
	vnode->stream.dir.dir_head = NULL;
	fs->root_vnode = vnode;
	hash_insert(fs->vnode_list_hash, vnode);

	*root_vnid = vnode->id;
	*_fs = fs;

	return 0;

err4:
	rootfs_delete_vnode(fs, vnode, true);
err3:
	hash_uninit(fs->vnode_list_hash);
err2:
	mutex_destroy(&fs->lock);
err1:
	kfree(fs);

	return err;
}


static int
rootfs_unmount(fs_cookie _fs)
{
	struct rootfs *fs = (struct rootfs *)_fs;
	struct rootfs_vnode *v;
	struct hash_iterator i;

	TRACE(("rootfs_unmount: entry fs = 0x%x\n", fs));

	// put_vnode on the root to release the ref to it
	vfs_put_vnode(fs->id, fs->root_vnode->id);

	// delete all of the vnodes
	hash_open(fs->vnode_list_hash, &i);
	while((v = (struct rootfs_vnode *)hash_next(fs->vnode_list_hash, &i)) != NULL) {
		rootfs_delete_vnode(fs, v, true);
	}
	hash_close(fs->vnode_list_hash, &i, false);

	hash_uninit(fs->vnode_list_hash);
	mutex_destroy(&fs->lock);
	kfree(fs);

	return 0;
}


static int
rootfs_sync(fs_cookie fs)
{
	TRACE(("rootfs_sync: entry\n"));

	return 0;
}


static int
rootfs_lookup(fs_cookie _fs, fs_vnode _dir, const char *name, vnode_id *_id, int *_type)
{
	struct rootfs *fs = (struct rootfs *)_fs;
	struct rootfs_vnode *dir = (struct rootfs_vnode *)_dir;
	struct rootfs_vnode *vnode,*vdummy;
	struct rootfs_vnode *v1;
	int status;

	TRACE(("rootfs_lookup: entry dir 0x%x, name '%s'\n", dir, name));
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


static int
rootfs_get_vnode_name(fs_cookie _fs, fs_vnode _vnode, char *buffer, size_t bufferSize)
{
	struct rootfs_vnode *vnode = (struct rootfs_vnode *)_vnode;

	TRACE(("devfs_get_vnode_name: vnode = %p\n",vnode));
	
	strlcpy(buffer,vnode->name,bufferSize);
	return B_OK;
}


static int
rootfs_get_vnode(fs_cookie _fs, vnode_id id, fs_vnode *_vnode, bool reenter)
{
	struct rootfs *fs = (struct rootfs *)_fs;

	TRACE(("rootfs_getvnode: asking for vnode 0x%x 0x%x, r %d\n", id, reenter));

	if (!reenter)
		mutex_lock(&fs->lock);

	*_vnode = hash_lookup(fs->vnode_list_hash, &id);

	if (!reenter)
		mutex_unlock(&fs->lock);

	TRACE(("rootfs_getnvnode: looked it up at 0x%x\n", *_vnode));

	if (*_vnode)
		return B_NO_ERROR;

	return ENOENT;
}


static int
rootfs_put_vnode(fs_cookie _fs, fs_vnode _vnode, bool reenter)
{
#if ROOTFS_TRACE
	struct rootfs_vnode *vnode = (struct rootfs_vnode *)_vnode;

	TRACE(("rootfs_putvnode: entry on vnode 0x%x 0x%x, r %d\n", vnode->id, reenter));
#endif
	return 0; // whatever
}


static int
rootfs_remove_vnode(fs_cookie _fs, fs_vnode _vnode, bool reenter)
{
	struct rootfs *fs = (struct rootfs *)_fs;
	struct rootfs_vnode *vnode = (struct rootfs_vnode *)_vnode;

	TRACE(("rootfs_removevnode: remove 0x%x (0x%x 0x%x), r %d\n", vnode, vnode->id, reenter));

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


static int
rootfs_create(fs_cookie _fs, fs_vnode _dir, const char *name, int omode, int perms, file_cookie *_cookie, vnode_id *new_vnid)
{
	return EINVAL;
}


static int
rootfs_open(fs_cookie _fs, fs_vnode _v, int oflags, file_cookie *_cookie)
{
	// allow to open the file, but it can't be done anything with it
	return B_OK;
}


static int
rootfs_close(fs_cookie _fs, fs_vnode _v, file_cookie _cookie)
{
#if ROOTFS_TRACE
	struct rootfs_vnode *v = _v;
	struct rootfs_cookie *cookie = _cookie;

	TRACE(("rootfs_close: entry vnode 0x%x, cookie 0x%x\n", v, cookie));
#endif
	return 0;
}


static int
rootfs_free_cookie(fs_cookie _fs, fs_vnode _v, file_cookie _cookie)
{
	struct rootfs_cookie *cookie = _cookie;
#if ROOTFS_TRACE
	struct rootfs_vnode *v = _v;

	TRACE(("rootfs_freecookie: entry vnode 0x%x, cookie 0x%x\n", v, cookie));
#endif
	if (cookie)
		kfree(cookie);

	return 0;
}


static int
rootfs_fsync(fs_cookie _fs, fs_vnode _v)
{
	return 0;
}


static ssize_t
rootfs_read(fs_cookie _fs, fs_vnode _v, file_cookie _cookie, off_t pos, void *buf, size_t *len)
{
	return EINVAL;
}


static ssize_t
rootfs_write(fs_cookie fs, fs_vnode v, file_cookie cookie, off_t pos, const void *buf, size_t *len)
{
	TRACE(("rootfs_write: vnode 0x%x, cookie 0x%x, pos 0x%x 0x%x, len 0x%x\n", v, cookie, pos, *len));

	return EPERM;
}


static off_t
rootfs_seek(fs_cookie _fs, fs_vnode _v, file_cookie _cookie, off_t pos, int st)
{
	return ESPIPE;
}


static int
rootfs_create_dir(fs_cookie _fs, fs_vnode _dir, const char *name, int perms, vnode_id *new_vnid)
{
	struct rootfs *fs = _fs;
	struct rootfs_vnode *dir = _dir;
	struct rootfs_vnode *vnode;
	bool created_vnode = false;
	int status = 0;

	TRACE(("rootfs_create_dir: dir 0x%x, name = '%s', stream_type = %d\n", dir, name, st));

	mutex_lock(&fs->lock);

	vnode = rootfs_find_in_dir(dir, name);
	if (vnode != NULL) {
		status = B_FILE_EXISTS;
		goto err;
	}

	dprintf("rootfs_create: creating new vnode\n");
	vnode = rootfs_create_vnode(fs);
	if (vnode == NULL) {
		status = B_NO_MEMORY;
		goto err;
	}
	created_vnode = true;
	vnode->name = kstrdup(name);
	if (vnode->name == NULL) {
		status = B_NO_MEMORY;
		goto err1;
	}
	vnode->stream.type = STREAM_TYPE_DIR;
	vnode->parent = dir;
	rootfs_insert_in_dir(dir, vnode);

	hash_insert(fs->vnode_list_hash, vnode);

	vnode->stream.dir.dir_head = NULL;
	vnode->stream.dir.jar_head = NULL;

	mutex_unlock(&fs->lock);
	return 0;

err1:
	if (created_vnode)
		rootfs_delete_vnode(fs, vnode, false);
err:
	mutex_unlock(&fs->lock);

	return status;
}


static int
rootfs_remove_dir(fs_cookie _fs, fs_vnode _dir, const char *name)
{
	struct rootfs *fs = _fs;
	struct rootfs_vnode *dir = _dir;

	TRACE(("rootfs_remove_dir: dir 0x%x (0x%x 0x%x), name '%s'\n", dir, dir->id, name));

	return rootfs_remove(fs, dir, name, true);
}


static int
rootfs_open_dir(fs_cookie _fs, fs_vnode _v, file_cookie *_cookie)
{
	struct rootfs *fs = (struct rootfs *)_fs;
	struct rootfs_vnode *vnode = (struct rootfs_vnode *)_v;
	struct rootfs_cookie *cookie;

	TRACE(("rootfs_open: vnode 0x%x\n", vnode));

	if (vnode->stream.type != STREAM_TYPE_DIR)
		return B_BAD_VALUE;

	cookie = kmalloc(sizeof(struct rootfs_cookie));
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


static int
rootfs_read_dir(fs_cookie _fs, fs_vnode _vnode, file_cookie _cookie, struct dirent *dirent, size_t bufferSize, uint32 *_num)
{
	struct rootfs_cookie *cookie = _cookie;
	struct rootfs *fs = _fs;
	status_t status = 0;

	TRACE(("rootfs_read_dir: vnode 0x%x, cookie 0x%x, buffer = 0x%x, bufferSize = 0x%x\n", v, cookie, dirent, bufferSize));

	mutex_lock(&fs->lock);

	if (cookie->ptr == NULL) {
		// we're at the end of the directory
		*_num = 0;
		status = B_OK;
		goto err;
	}

	dirent->d_dev = fs->id;
	dirent->d_ino = cookie->ptr->id;
	dirent->d_reclen = strlen(cookie->ptr->name);

	if (sizeof(struct dirent) + dirent->d_reclen + 1 > bufferSize) {
		status = ERR_VFS_INSUFFICIENT_BUF;
		goto err;
	}

	status = user_strcpy(dirent->d_name, cookie->ptr->name);
	if (status < 0)
		goto err;

	cookie->ptr = cookie->ptr->dir_next;

err:
	mutex_unlock(&fs->lock);

	return status;
}


static int
rootfs_rewind_dir(fs_cookie _fs, fs_vnode _vnode, file_cookie _cookie)
{
	struct rootfs *fs = _fs;
	struct rootfs_vnode *vnode = _vnode;
	struct rootfs_cookie *cookie = _cookie;

	mutex_lock(&fs->lock);

	cookie->ptr = vnode->stream.dir.dir_head;

	mutex_unlock(&fs->lock);
	return B_OK;
}


static int
rootfs_ioctl(fs_cookie _fs, fs_vnode _v, file_cookie _cookie, ulong op, void *buf, size_t len)
{
	TRACE(("rootfs_ioctl: vnode 0x%x, cookie 0x%x, op %d, buf 0x%x, len 0x%x\n", _v, _cookie, op, buf, len));

	return EINVAL;
}


static int
rootfs_can_page(fs_cookie _fs, fs_vnode _v)
{
	return -1;
}


static ssize_t
rootfs_read_page(fs_cookie _fs, fs_vnode _v, iovecs *vecs, off_t pos)
{
	return EPERM;
}


static ssize_t
rootfs_write_page(fs_cookie _fs, fs_vnode _v, iovecs *vecs, off_t pos)
{
	return EPERM;
}


static int
rootfs_read_link(fs_cookie _fs, fs_vnode _link, char *buffer, size_t bufferSize)
{
	struct rootfs *fs = _fs;
	struct rootfs_vnode *link = _link;
	
	if (link->stream.type != STREAM_TYPE_SYMLINK)
		return B_BAD_VALUE;

	if (bufferSize < link->stream.symlink.length)
		return B_NAME_TOO_LONG;

	memcpy(buffer, link->stream.symlink.path, link->stream.symlink.length + 1);
	return link->stream.symlink.length;
}


static int
rootfs_symlink(fs_cookie _fs, fs_vnode _dir, const char *name, const char *path, int mode)
{
	struct rootfs *fs = _fs;
	struct rootfs_vnode *dir = _dir;
	struct rootfs_vnode *vnode;
	bool created_vnode = false;
	int status = 0;

	TRACE(("rootfs_symlink: dir 0x%x, name = '%s', path = %s\n", dir, name, path));

	mutex_lock(&fs->lock);

	vnode = rootfs_find_in_dir(dir, name);
	if (vnode != NULL) {
		status = B_FILE_EXISTS;
		goto err;
	}

	dprintf("rootfs_create: creating new symlink\n");
	vnode = rootfs_create_vnode(fs);
	if (vnode == NULL) {
		status = B_NO_MEMORY;
		goto err;
	}
	created_vnode = true;
	vnode->name = kstrdup(name);
	if (vnode->name == NULL) {
		status = B_NO_MEMORY;
		goto err1;
	}
	vnode->stream.type = STREAM_TYPE_SYMLINK;
	vnode->parent = dir;
	rootfs_insert_in_dir(dir, vnode);

	hash_insert(fs->vnode_list_hash, vnode);

	vnode->stream.symlink.path = kstrdup(path);
	if (vnode->stream.symlink.path == NULL) {
		status = ENOMEM;
		goto err1;
	}
	vnode->stream.symlink.length = strlen(path);

	mutex_unlock(&fs->lock);
	return 0;

err1:
	if (created_vnode)
		rootfs_delete_vnode(fs, vnode, false);
err:
	mutex_unlock(&fs->lock);
	return status;
}


static int
rootfs_unlink(fs_cookie _fs, fs_vnode _dir, const char *name)
{
	struct rootfs *fs = _fs;
	struct rootfs_vnode *dir = _dir;

	TRACE(("rootfs_unlink: dir 0x%x (0x%x 0x%x), name '%s'\n", dir, dir->id, name));

	return rootfs_remove(fs, dir, name, false);
}


static int
rootfs_rename(fs_cookie _fs, fs_vnode _olddir, const char *oldname, fs_vnode _newdir, const char *newname)
{
	struct rootfs *fs = _fs;
	struct rootfs_vnode *olddir = _olddir;
	struct rootfs_vnode *newdir = _newdir;
	struct rootfs_vnode *v1, *v2;
	int err;

	TRACE(("rootfs_rename: olddir 0x%x (0x%x 0x%x), oldname '%s', newdir 0x%x (0x%x 0x%x), newname '%s'\n",
		olddir, olddir->id, oldname, newdir, newdir->id, newname));

	mutex_lock(&fs->lock);

	v1 = rootfs_find_in_dir(olddir, oldname);
	if (!v1) {
		err = ERR_VFS_PATH_NOT_FOUND;
		goto err;
	}

	v2 = rootfs_find_in_dir(newdir, newname);

	if (olddir == newdir) {
		// rename to a different name in the same dir
		if (v2) {
			// target node exists
			err = ERR_VFS_ALREADY_EXISTS;
			goto err;
		}

		// change the name on this node
		if (strlen(oldname) >= strlen(newname)) {
			// reuse the old name buffer
			strcpy(v1->name, newname);
		} else {
			char *ptr = v1->name;

			v1->name = kstrdup(newname);
			if(!v1->name) {
				// bad place to be, at least restore
				v1->name = ptr;
				err = ENOMEM;
				goto err;
			}
			kfree(ptr);
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


static int
rootfs_read_stat(fs_cookie _fs, fs_vnode _v, struct stat *stat)
{
	struct rootfs_vnode *vnode = _v;

	TRACE(("rootfs_rstat: vnode 0x%x (0x%x 0x%x), stat 0x%x\n", vnode, vnode->id, stat));

	// stream exists, but we know to return size 0, since we can only hold directories
	stat->st_ino = vnode->id;
	stat->st_size = 0;
	stat->st_mode = vnode->stream.type | DEFFILEMODE;

	return 0;
}


static int
rootfs_write_stat(fs_cookie _fs, fs_vnode _v, const struct stat *stat, int stat_mask)
{
#if ROOTFS_TRACE
	struct rootfs *fs = _fs;
	struct rootfs_vnode *v = _v;

	TRACE(("rootfs_wstat: vnode 0x%x (0x%x 0x%x), stat 0x%x\n", v, v->id, stat));
#endif
	// cannot change anything
	return EINVAL;
}


//	#pragma mark -


static struct fs_calls rootfs_calls = {
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
	&rootfs_read_page,
	&rootfs_write_page,

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
	&rootfs_seek,

	/* directory */
	&rootfs_create_dir,
	&rootfs_remove_dir,
	&rootfs_open_dir,
	&rootfs_close,			// we are using the same operations for directories
	&rootfs_free_cookie,	// and files here - that's intended, not by accident
	&rootfs_read_dir,
	&rootfs_rewind_dir,
};


int
bootstrap_rootfs(void)
{
	dprintf("bootstrap_rootfs: entry\n");

	return vfs_register_filesystem("rootfs", &rootfs_calls);
}
