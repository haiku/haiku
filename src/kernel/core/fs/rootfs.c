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

struct rootfs_stream {
	// only type of stream supported by rootfs
	struct stream_dir {
		struct rootfs_vnode *dir_head;
		struct rootfs_cookie *jar_head;
	} dir;
};

struct rootfs_vnode {
	struct rootfs_vnode *all_next;
	vnode_id id;
	char *name;
	void *redir_vnode;
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
static unsigned int rootfs_vnode_hash_func(void *_v, const void *_key, unsigned int range)
{
	struct rootfs_vnode *v = _v;
	const vnode_id *key = _key;

	if(v != NULL)
		return v->id % range;
	else
		return (*key) % range;
}

static int rootfs_vnode_compare_func(void *_v, const void *_key)
{
	struct rootfs_vnode *v = _v;
	const vnode_id *key = _key;

	if(v->id == *key)
		return 0;
	else
		return -1;
}

static struct rootfs_vnode *rootfs_create_vnode(struct rootfs *fs)
{
	struct rootfs_vnode *v;

	v = kmalloc(sizeof(struct rootfs_vnode));
	if(v == NULL)
		return NULL;

	memset(v, 0, sizeof(struct rootfs_vnode));
	v->id = fs->next_vnode_id++;

	return v;
}

static int rootfs_delete_vnode(struct rootfs *fs, struct rootfs_vnode *v, bool force_delete)
{
	// cant delete it if it's in a directory or is a directory
	// and has children
	if(!force_delete && (v->stream.dir.dir_head != NULL || v->dir_next != NULL)) {
		return ERR_NOT_ALLOWED;
	}

	// remove it from the global hash table
	hash_remove(fs->vnode_list_hash, v);

	if(v->name != NULL)
		kfree(v->name);
	kfree(v);

	return 0;
}

static void insert_cookie_in_jar(struct rootfs_vnode *dir, struct rootfs_cookie *cookie)
{
	cookie->next = dir->stream.dir.jar_head;
	dir->stream.dir.jar_head = cookie;
	cookie->prev = NULL;
}

static void remove_cookie_from_jar(struct rootfs_vnode *dir, struct rootfs_cookie *cookie)
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

	for(cookie = dir->stream.dir.jar_head; cookie; cookie = cookie->next) {
		if(cookie->ptr == v) {
			cookie->ptr = v->dir_next;
		}
	}
}

static struct rootfs_vnode *rootfs_find_in_dir(struct rootfs_vnode *dir, const char *path)
{
	struct rootfs_vnode *v;

	if(!strcmp(path, "."))
		return dir;
	if(!strcmp(path, ".."))
		return dir->parent;

	for(v = dir->stream.dir.dir_head; v; v = v->dir_next) {
		if(strcmp(v->name, path) == 0) {
			return v;
		}
	}
	return NULL;
}

static int rootfs_insert_in_dir(struct rootfs_vnode *dir, struct rootfs_vnode *v)
{
	v->dir_next = dir->stream.dir.dir_head;
	dir->stream.dir.dir_head = v;
	return 0;
}

static int rootfs_remove_from_dir(struct rootfs_vnode *dir, struct rootfs_vnode *findit)
{
	struct rootfs_vnode *v;
	struct rootfs_vnode *last_v;

	for(v = dir->stream.dir.dir_head, last_v = NULL; v; last_v = v, v = v->dir_next) {
		if(v == findit) {
			/* make sure all dircookies dont point to this vnode */
			update_dircookies(dir, v);

			if(last_v)
				last_v->dir_next = v->dir_next;
			else
				dir->stream.dir.dir_head = v->dir_next;
			v->dir_next = NULL;
			return 0;
		}
	}
	return -1;
}

static int rootfs_is_dir_empty(struct rootfs_vnode *dir)
{
	return !dir->stream.dir.dir_head;
}

static int rootfs_mount(fs_cookie *_fs, fs_id id, const char *device, void *args, vnode_id *root_vnid)
{
	struct rootfs *fs;
	struct rootfs_vnode *v;
	int err;

	TRACE(("rootfs_mount: entry\n"));

	fs = kmalloc(sizeof(struct rootfs));
	if(fs == NULL) {
		err = ERR_NO_MEMORY;
		goto err;
	}

	fs->id = id;
	fs->next_vnode_id = 0;

	err = mutex_init(&fs->lock, "rootfs_mutex");
	if(err < 0) {
		goto err1;
	}

	fs->vnode_list_hash = hash_init(ROOTFS_HASH_SIZE, (addr)&v->all_next - (addr)v,
		&rootfs_vnode_compare_func, &rootfs_vnode_hash_func);
	if(fs->vnode_list_hash == NULL) {
		err = ERR_NO_MEMORY;
		goto err2;
	}

	// create a vnode
	v = rootfs_create_vnode(fs);
	if(v == NULL) {
		err = ERR_NO_MEMORY;
		goto err3;
	}

	// set it up
	v->parent = v;
	v->name = kstrdup("");
	if(v->name == NULL) {
		err = ERR_NO_MEMORY;
		goto err4;
	}

	v->stream.dir.dir_head = NULL;
	fs->root_vnode = v;
	hash_insert(fs->vnode_list_hash, v);

	*root_vnid = v->id;
	*_fs = fs;

	return 0;

err4:
	rootfs_delete_vnode(fs, v, true);
err3:
	hash_uninit(fs->vnode_list_hash);
err2:
	mutex_destroy(&fs->lock);
err1:
	kfree(fs);
err:
	return err;
}

static int rootfs_unmount(fs_cookie _fs)
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

static int rootfs_sync(fs_cookie fs)
{
	TRACE(("rootfs_sync: entry\n"));

	return 0;
}

static int rootfs_lookup(fs_cookie _fs, fs_vnode _dir, const char *name, vnode_id *id)
{
	struct rootfs *fs = (struct rootfs *)_fs;
	struct rootfs_vnode *dir = (struct rootfs_vnode *)_dir;
	struct rootfs_vnode *v;
	struct rootfs_vnode *v1;
	int err;

	TRACE(("rootfs_lookup: entry dir 0x%x, name '%s'\n", dir, name));

	mutex_lock(&fs->lock);

	// look it up
	v = rootfs_find_in_dir(dir, name);
	if(!v) {
		err = ENOENT;
		goto err;
	}

	err = vfs_get_vnode(fs->id, v->id, (fs_vnode *)&v1);
	if(err < 0) {
		goto err;
	}

	*id = v->id;

	err = B_NO_ERROR;

err:
	mutex_unlock(&fs->lock);

	return err;
}

static int rootfs_getvnode(fs_cookie _fs, vnode_id id, fs_vnode *v, bool r)
{
	struct rootfs *fs = (struct rootfs *)_fs;

	TRACE(("rootfs_getvnode: asking for vnode 0x%x 0x%x, r %d\n", id, r));

	if(!r)
		mutex_lock(&fs->lock);

	*v = hash_lookup(fs->vnode_list_hash, &id);

	if(!r)
		mutex_unlock(&fs->lock);

	TRACE(("rootfs_getnvnode: looked it up at 0x%x\n", *v));

	if(*v)
		return 0;
	else
		return ERR_NOT_FOUND;
}

static int rootfs_putvnode(fs_cookie _fs, fs_vnode _v, bool r)
{
#if ROOTFS_TRACE
	struct rootfs_vnode *v = (struct rootfs_vnode *)_v;

	TRACE(("rootfs_putvnode: entry on vnode 0x%x 0x%x, r %d\n", v->id, r));
#endif
	return 0; // whatever
}

static int rootfs_removevnode(fs_cookie _fs, fs_vnode _v, bool r)
{
	struct rootfs *fs = (struct rootfs *)_fs;
	struct rootfs_vnode *v = (struct rootfs_vnode *)_v;
	int err;

	TRACE(("rootfs_removevnode: remove 0x%x (0x%x 0x%x), r %d\n", v, v->id, r));

	if(!r)
		mutex_lock(&fs->lock);

	if(v->dir_next) {
		// can't remove node if it's linked to the dir
		panic("rootfs_removevnode: vnode %p asked to be removed is present in dir\n", v);
	}

	rootfs_delete_vnode(fs, v, false);

	err = 0;

	if(!r)
		mutex_unlock(&fs->lock);

	return err;
}

static int rootfs_open(fs_cookie _fs, fs_vnode _v, file_cookie *_cookie, stream_type st, int oflags)
{
	struct rootfs *fs = (struct rootfs *)_fs;
	struct rootfs_vnode *v = (struct rootfs_vnode *)_v;
	struct rootfs_cookie *cookie;
	int err = 0;

	TRACE(("rootfs_open: vnode 0x%x, stream_type %d, oflags 0x%x\n", v, st, oflags));

	if(st != STREAM_TYPE_ANY && st != STREAM_TYPE_DIR) {
		err = ERR_VFS_WRONG_STREAM_TYPE;
		goto err;
	}

	cookie = kmalloc(sizeof(struct rootfs_cookie));
	if(cookie == NULL) {
		err = ERR_NO_MEMORY;
		goto err;
	}

	mutex_lock(&fs->lock);

	cookie->ptr = v->stream.dir.dir_head;
	cookie->oflags = oflags;

	insert_cookie_in_jar(v, cookie);

	*_cookie = cookie;

	mutex_unlock(&fs->lock);
err:
	return err;
}

static int rootfs_close(fs_cookie _fs, fs_vnode _v, file_cookie _cookie)
{
#if ROOTFS_TRACE
	struct rootfs_vnode *v = _v;
	struct rootfs_cookie *cookie = _cookie;

	TRACE(("rootfs_close: entry vnode 0x%x, cookie 0x%x\n", v, cookie));
#endif
	return 0;
}

static int rootfs_freecookie(fs_cookie _fs, fs_vnode _v, file_cookie _cookie)
{
	struct rootfs_cookie *cookie = _cookie;
#if ROOTFS_TRACE
	struct rootfs_vnode *v = _v;

	TRACE(("rootfs_freecookie: entry vnode 0x%x, cookie 0x%x\n", v, cookie));
#endif
	if(cookie)
		kfree(cookie);

	return 0;
}

static int rootfs_fsync(fs_cookie _fs, fs_vnode _v)
{
	return 0;
}

static ssize_t rootfs_read(fs_cookie _fs, fs_vnode _v, file_cookie _cookie, void *buf, off_t pos, size_t *len)
{
	struct rootfs *fs = _fs;
#if ROOTFS_TRACE
	struct rootfs_vnode *v = _v;
#endif
	struct rootfs_cookie *cookie = _cookie;
	int err = 0;

	TRACE(("rootfs_read: vnode 0x%x, cookie 0x%x, pos 0x%x 0x%x, len 0x%x\n", v, cookie, pos, *len));

	mutex_lock(&fs->lock);

	if(cookie->ptr == NULL) {
		*len = 0;
		err = 0;
		goto err;
	}

	if((ssize_t)strlen(cookie->ptr->name) + 1 > *len) {
		err = ERR_VFS_INSUFFICIENT_BUF;
		goto err;
	}

	err = user_strcpy(buf, cookie->ptr->name);
	if(err < 0)
		goto err;

	*len = strlen(cookie->ptr->name) + 1;

	cookie->ptr = cookie->ptr->dir_next;

err:
	mutex_unlock(&fs->lock);

	return err;
}

static ssize_t rootfs_write(fs_cookie fs, fs_vnode v, file_cookie cookie, const void *buf, off_t pos, size_t *len)
{
	TRACE(("rootfs_write: vnode 0x%x, cookie 0x%x, pos 0x%x 0x%x, len 0x%x\n", v, cookie, pos, *len));

	return ERR_NOT_ALLOWED;
}

static int rootfs_seek(fs_cookie _fs, fs_vnode _v, file_cookie _cookie, off_t pos, int st)
{
	struct rootfs *fs = _fs;
	struct rootfs_vnode *v = _v;
	struct rootfs_cookie *cookie = _cookie;
	int err = 0;

	TRACE(("rootfs_seek: vnode 0x%x, cookie 0x%x, pos 0x%x 0x%x, seek_type %d\n", v, cookie, pos, st));

	mutex_lock(&fs->lock);

	switch(st) {
		// only valid args are seek_type SEEK_SET, pos 0.
		// this rewinds to beginning of directory
		case SEEK_SET:
			if(pos == 0) {
				cookie->ptr = v->stream.dir.dir_head;
			} else {
				err = ESPIPE;
			}
			break;
		case SEEK_CUR:
		case SEEK_END:
		default:
			err = EINVAL;
	}

	mutex_unlock(&fs->lock);

	return err;
}


static int
rootfs_read_dir(fs_cookie _fs, fs_vnode _vnode, file_cookie _cookie, struct dirent *buffer, size_t bufferSize, uint32 *_num)
{
	// ToDo: implement me!
	return B_OK;
}


static int
rootfs_rewind_dir(fs_cookie _fs, fs_vnode _vnode, file_cookie _cookie)
{
	// ToDo: me too!
	return B_OK;
}


static int
rootfs_ioctl(fs_cookie _fs, fs_vnode _v, file_cookie _cookie, ulong op, void *buf, size_t len)
{
	TRACE(("rootfs_ioctl: vnode 0x%x, cookie 0x%x, op %d, buf 0x%x, len 0x%x\n", _v, _cookie, op, buf, len));

	return EINVAL;
}


static int
rootfs_canpage(fs_cookie _fs, fs_vnode _v)
{
	return -1;
}


static ssize_t
rootfs_readpage(fs_cookie _fs, fs_vnode _v, iovecs *vecs, off_t pos)
{
	return EPERM;
}


static ssize_t
rootfs_writepage(fs_cookie _fs, fs_vnode _v, iovecs *vecs, off_t pos)
{
	return EPERM;
}


static int
rootfs_create(fs_cookie _fs, fs_vnode _dir, const char *name, stream_type st, void *create_args, vnode_id *new_vnid)
{
	struct rootfs *fs = _fs;
	struct rootfs_vnode *dir = _dir;
	struct rootfs_vnode *new_vnode;
	struct rootfs_stream *s;
	int err = 0;
	bool created_vnode = false;

	TRACE(("rootfs_create: dir 0x%x, name = '%s', stream_type = %d\n", dir, name, st));

	// we only support stream types of STREAM_TYPE_DIR
	if (st != STREAM_TYPE_DIR) {
		err = EPERM;
		goto err;
	}

	mutex_lock(&fs->lock);

	new_vnode = rootfs_find_in_dir(dir, name);
	if (new_vnode == NULL) {
		dprintf("rootfs_create: creating new vnode\n");
		new_vnode = rootfs_create_vnode(fs);
		if(new_vnode == NULL) {
			err = ENOMEM;
			goto err;
		}
		created_vnode = true;
		new_vnode->name = kstrdup(name);
		if(new_vnode->name == NULL) {
			err = ENOMEM;
			goto err1;
		}
		new_vnode->parent = dir;
		rootfs_insert_in_dir(dir, new_vnode);

		hash_insert(fs->vnode_list_hash, new_vnode);

		s = &new_vnode->stream;
	} else {
		// we found the vnode
		err = ERR_VFS_ALREADY_EXISTS;
		goto err;
	}

	new_vnode->stream.dir.dir_head = NULL;
	new_vnode->stream.dir.jar_head = NULL;

	mutex_unlock(&fs->lock);
	return 0;

err1:
	if(created_vnode)
		rootfs_delete_vnode(fs, new_vnode, false);
err:
	mutex_unlock(&fs->lock);

	return err;
}

static int rootfs_unlink(fs_cookie _fs, fs_vnode _dir, const char *name)
{
	struct rootfs *fs = _fs;
	struct rootfs_vnode *dir = _dir;
	struct rootfs_vnode *v;
	int err;

	TRACE(("rootfs_unlink: dir 0x%x (0x%x 0x%x), name '%s'\n", dir, dir->id, name));

	mutex_lock(&fs->lock);

	v = rootfs_find_in_dir(dir, name);
	if(!v) {
		err = ERR_VFS_PATH_NOT_FOUND;
		goto err;
	}

	// do some checking to see if we can delete it
	if(!rootfs_is_dir_empty(v)) {
		err = ERR_VFS_DIR_NOT_EMPTY;
		goto err;
	}

	rootfs_remove_from_dir(dir, v);

	// schedule this vnode to be removed when it's ref goes to zero
	vfs_remove_vnode(fs->id, v->id);

	err = 0;

err:
	mutex_unlock(&fs->lock);

	return err;
}

static int rootfs_rename(fs_cookie _fs, fs_vnode _olddir, const char *oldname, fs_vnode _newdir, const char *newname)
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
	if(!v1) {
		err = ERR_VFS_PATH_NOT_FOUND;
		goto err;
	}

	v2 = rootfs_find_in_dir(newdir, newname);

	if(olddir == newdir) {
		// rename to a different name in the same dir
		if(v2) {
			// target node exists
			err = ERR_VFS_ALREADY_EXISTS;
			goto err;
		}

		// change the name on this node
		if(strlen(oldname) >= strlen(newname)) {
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

static int rootfs_rstat(fs_cookie _fs, fs_vnode _v, struct stat *stat)
{
	struct rootfs_vnode *v = _v;

	TRACE(("rootfs_rstat: vnode 0x%x (0x%x 0x%x), stat 0x%x\n", v, v->id, stat));

//dprintf("rootfs_rstat\n");

	// stream exists, but we know to return size 0, since we can only hold directories
	stat->st_ino = v->id;
	stat->st_size = 0;
	stat->st_mode = (S_IFDIR | DEFFILEMODE);

	return 0;
}

static int rootfs_wstat(fs_cookie _fs, fs_vnode _v, struct stat *stat, int stat_mask)
{
#if ROOTFS_TRACE
	struct rootfs *fs = _fs;
	struct rootfs_vnode *v = _v;

	TRACE(("rootfs_wstat: vnode 0x%x (0x%x 0x%x), stat 0x%x\n", v, v->id, stat));
#endif
	// cannot change anything
	return EINVAL;
}

static struct fs_calls rootfs_calls = {
	&rootfs_mount,
	&rootfs_unmount,
	&rootfs_sync,

	&rootfs_lookup,

	&rootfs_getvnode,
	&rootfs_putvnode,
	&rootfs_removevnode,

	&rootfs_open,
	&rootfs_close,
	&rootfs_freecookie,
	&rootfs_fsync,

	&rootfs_read,
	&rootfs_write,
	&rootfs_seek,
	
	&rootfs_read_dir,
	&rootfs_rewind_dir,

	&rootfs_ioctl,

	&rootfs_canpage,
	&rootfs_readpage,
	&rootfs_writepage,

	&rootfs_create,
	&rootfs_unlink,
	&rootfs_rename,

	&rootfs_rstat,
	&rootfs_wstat,
};

int bootstrap_rootfs(void)
{
	dprintf("bootstrap_rootfs: entry\n");

	return vfs_register_filesystem("rootfs", &rootfs_calls);
}
