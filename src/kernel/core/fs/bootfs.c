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

#include <boot/bootdir.h>

#include <sys/stat.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>

#include "builtin_fs.h"


#define BOOTFS_TRACE 0
#if BOOTFS_TRACE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x)
#endif

static char *bootdir = NULL;
static off_t bootdir_len = 0;
static region_id bootdir_region = -1;

struct bootfs_stream {
	int type;
	union {
		struct stream_dir {
			struct bootfs_vnode *dir_head;
			struct bootfs_cookie *jar_head;
		} dir;
		struct stream_file {
			uint8 *start;
			off_t len;
		} file;
	} u;
};

struct bootfs_vnode {
	struct bootfs_vnode *all_next;
	vnode_id id;
	char *name;
	struct bootfs_vnode *parent;
	struct bootfs_vnode *dir_next;
	struct bootfs_stream stream;
};

struct bootfs {
	mount_id id;
	mutex lock;
	int32 next_vnode_id;
	void *vnode_list_hash;
	struct bootfs_vnode *root_vnode;
};

struct bootfs_cookie {
	struct bootfs_stream *s;
	int oflags;
	union {
		struct cookie_dir {
			struct bootfs_cookie *next;
			struct bootfs_cookie *prev;
			struct bootfs_vnode *ptr;
		} dir;
		struct cookie_file {
			off_t pos;
		} file;
	} u;
};


#define BOOTFS_HASH_SIZE 16


static uint32
bootfs_vnode_hash_func(void *_v, const void *_key, uint32 range)
{
	struct bootfs_vnode *v = _v;
	const vnode_id *key = _key;

	if (v != NULL)
		return v->id % range;

	return (*key) % range;
}


static int
bootfs_vnode_compare_func(void *_v, const void *_key)
{
	struct bootfs_vnode *v = _v;
	const vnode_id *key = _key;

	if (v->id == *key)
		return 0;

	return -1;
}


static struct bootfs_vnode *
bootfs_create_vnode(struct bootfs *fs, const char *name)
{
	struct bootfs_vnode *v;

	v = malloc(sizeof(struct bootfs_vnode));
	if (v == NULL)
		return NULL;

	memset(v, 0, sizeof(struct bootfs_vnode));
	v->id = atomic_add(&fs->next_vnode_id, 1);

	v->name = strdup(name);
	if (v->name == NULL) {
		free(v);
		return NULL;
	}

	return v;
}


static status_t
bootfs_delete_vnode(struct bootfs *fs, struct bootfs_vnode *v, bool force_delete)
{
	// cant delete it if it's in a directory or is a directory
	// and has children
	if (!force_delete
		&& ((v->stream.type == S_IFDIR && v->stream.u.dir.dir_head != NULL)
			|| v->dir_next != NULL))
		return EPERM;

	// remove it from the global hash table
	hash_remove(fs->vnode_list_hash, v);

	if (v->name != NULL)
		free(v->name);
	free(v);

	return 0;
}

#if 0
static void
insert_cookie_in_jar(struct bootfs_vnode *dir, struct bootfs_cookie *cookie)
{
	cookie->u.dir.next = dir->stream.u.dir.jar_head;
	dir->stream.u.dir.jar_head = cookie;
	cookie->u.dir.prev = NULL;
}


static void
remove_cookie_from_jar(struct bootfs_vnode *dir, struct bootfs_cookie *cookie)
{
	if(cookie->u.dir.next)
		cookie->u.dir.next->u.dir.prev = cookie->u.dir.prev;
	if(cookie->u.dir.prev)
		cookie->u.dir.prev->u.dir.next = cookie->u.dir.next;
	if(dir->stream.u.dir.jar_head == cookie)
		dir->stream.u.dir.jar_head = cookie->u.dir.next;

	cookie->u.dir.prev = cookie->u.dir.next = NULL;
}


/** Makes sure none of the dircookies point to the vnode passed in */

static void
update_dircookies(struct bootfs_vnode *dir, struct bootfs_vnode *v)
{
	struct bootfs_cookie *cookie;

	for(cookie = dir->stream.u.dir.jar_head; cookie; cookie = cookie->u.dir.next) {
		if(cookie->u.dir.ptr == v) {
			cookie->u.dir.ptr = v->dir_next;
		}
	}
}
#endif


static struct bootfs_vnode *
bootfs_find_in_dir(struct bootfs_vnode *dir, const char *path)
{
	struct bootfs_vnode *v;

	if (dir->stream.type != S_IFDIR)
		return NULL;

	if (!strcmp(path, "."))
		return dir;
	if (!strcmp(path, ".."))
		return dir->parent;

	for (v = dir->stream.u.dir.dir_head; v; v = v->dir_next) {
//		dprintf("bootfs_find_in_dir: looking at entry '%s'\n", v->name);
		if (strcmp(v->name, path) == 0) {
//			dprintf("bootfs_find_in_dir: found it at 0x%x\n", v);
			return v;
		}
	}
	return NULL;
}


static status_t
bootfs_insert_in_dir(struct bootfs_vnode *dir, struct bootfs_vnode *v)
{
	if (dir->stream.type != S_IFDIR)
		return EINVAL;

	v->dir_next = dir->stream.u.dir.dir_head;
	dir->stream.u.dir.dir_head = v;

	v->parent = dir;
	return 0;
}

#if 0
static status_t
bootfs_remove_from_dir(struct bootfs_vnode *dir, struct bootfs_vnode *findit)
{
	struct bootfs_vnode *v;
	struct bootfs_vnode *last_v;

	for(v = dir->stream.u.dir.dir_head, last_v = NULL; v; last_v = v, v = v->dir_next) {
		if(v == findit) {
			/* make sure all dircookies dont point to this vnode */
			update_dircookies(dir, v);

			if(last_v)
				last_v->dir_next = v->dir_next;
			else
				dir->stream.u.dir.dir_head = v->dir_next;
			v->dir_next = NULL;
			return 0;
		}
	}
	return -1;
}


static bool
bootfs_is_dir_empty(struct bootfs_vnode *dir)
{
	if(dir->stream.type != S_IFDIR)
		return false;
	return !dir->stream.u.dir.dir_head;
}
#endif


/**	Creates a path of vnodes up to the last part of the passed in path.
 *	returns the vnode the last segment should be a part of and
 *	a pointer to the leaf of the path.
 *	clobbers the path string passed in
 */

static struct bootfs_vnode *
bootfs_create_path(struct bootfs *fs, char *path, struct bootfs_vnode *base, char **path_leaf)
{
	struct bootfs_vnode *v;
	char *temp;
	bool done;

	// strip off any leading '/' or spaces
	for (; *path == '/' || *path == ' '; path++)
		;

	// first, find the leaf
	*path_leaf = strrchr(path, '/');
	if (*path_leaf == NULL) {
		// didn't find it, so this path only is a leaf
		*path_leaf = path;
		return base;
	}

	// this is a multipart path, seperate the leaf off
	**path_leaf = '\0';
	(*path_leaf)++;
	if (**path_leaf == '\0') {
		// the path ended with '/'. That's invalid
		return NULL;
	}

	// now, lets walk down the path, building vnodes as we need em
	done = false;
	for (; !done; path = temp+1) {
		// find the next seperator and knock it out
		temp = strchr(path, '/');
		if (temp) {
			*temp = '\0';
		} else {
			done = true;
		}

		if (*path == '\0') {
			// zero length path segment, continue
			continue;
		}

		v = bootfs_find_in_dir(base, path);
		if (!v) {
			v = bootfs_create_vnode(fs, path);
			if (!v)
				return NULL;

			v->stream.type = S_IFDIR;
			v->stream.u.dir.dir_head = NULL;
			v->stream.u.dir.jar_head = NULL;

			bootfs_insert_in_dir(base, v);
			hash_insert(fs->vnode_list_hash, v);
		} else {
			// we found one
			if (v->stream.type != S_IFDIR)
				return NULL;
		}
		base = v;
	}
	return base;
}


static status_t
bootfs_create_vnode_tree(struct bootfs *fs, struct bootfs_vnode *root)
{
	int i;
	boot_entry *entry;
	struct bootfs_vnode *new_vnode;
	struct bootfs_vnode *dir;
	char path[SYS_MAX_PATH_LEN];
	char *leaf;

	entry = (boot_entry *)bootdir;
	for (i = 0; i < BOOTDIR_MAX_ENTRIES; i++) {
		if (entry[i].be_type != BE_TYPE_NONE && entry[i].be_type != BE_TYPE_DIRECTORY) {
			strlcpy(path, entry[i].be_name, SYS_MAX_PATH_LEN);

			dir = bootfs_create_path(fs, path, root, &leaf);
			if (!dir)
				continue;

			new_vnode = bootfs_create_vnode(fs, leaf);
			if (new_vnode == NULL)
				return ENOMEM;

			// set up the new node
			new_vnode->stream.type = S_IFREG;
			new_vnode->stream.u.file.start = bootdir + entry[i].be_offset * B_PAGE_SIZE;
			new_vnode->stream.u.file.len = entry[i].be_vsize;

			dprintf("bootfs_create_vnode_tree: added entry '%s', start %p, len 0x%Lx\n", new_vnode->name,
				new_vnode->stream.u.file.start, new_vnode->stream.u.file.len);

			// insert it into the parent dir
			bootfs_insert_in_dir(dir, new_vnode);

			hash_insert(fs->vnode_list_hash, new_vnode);
		}
	}

	return 0;
}


//	#pragma mark -


static status_t
bootfs_mount(mount_id id, const char *device, void *args, fs_volume *_fs, vnode_id *root_vnid)
{
	struct bootfs *fs;
	struct bootfs_vnode *v;
	status_t err;

	TRACE(("bootfs_mount: entry\n"));

	fs = malloc(sizeof(struct bootfs));
	if (fs == NULL)
		return B_NO_MEMORY;

	fs->id = id;
	fs->next_vnode_id = 0;

	err = mutex_init(&fs->lock, "bootfs_mutex");
	if (err < 0)
		goto err1;

	fs->vnode_list_hash = hash_init(BOOTFS_HASH_SIZE, (addr)&v->all_next - (addr)v,
		&bootfs_vnode_compare_func, &bootfs_vnode_hash_func);
	if (fs->vnode_list_hash == NULL) {
		err = ENOMEM;
		goto err2;
	}

	// create a vnode
	v = bootfs_create_vnode(fs, "");
	if (v == NULL) {
		err = ENOMEM;
		goto err3;
	}

	// set it up
	v->parent = v;

	// create a dir stream for it to hold
	v->stream.type = S_IFDIR;
	v->stream.u.dir.dir_head = NULL;
	fs->root_vnode = v;

	hash_insert(fs->vnode_list_hash, v);

	err = bootfs_create_vnode_tree(fs, fs->root_vnode);
	if (err < 0)
		goto err4;

	*root_vnid = v->id;
	*_fs = fs;

	return 0;

err4:
	bootfs_delete_vnode(fs, v, true);
err3:
	hash_uninit(fs->vnode_list_hash);
err2:
	mutex_destroy(&fs->lock);
err1:
	free(fs);

	return err;
}


static status_t
bootfs_unmount(fs_volume _fs)
{
	struct bootfs *fs = _fs;
	struct bootfs_vnode *v;
	struct hash_iterator i;

	TRACE(("bootfs_unmount: entry fs = %p\n", fs));

	// delete all of the vnodes
	hash_open(fs->vnode_list_hash, &i);
	while((v = (struct bootfs_vnode *)hash_next(fs->vnode_list_hash, &i)) != NULL) {
		bootfs_delete_vnode(fs, v, true);
	}
	hash_close(fs->vnode_list_hash, &i, false);

	hash_uninit(fs->vnode_list_hash);
	mutex_destroy(&fs->lock);
	free(fs);

	return 0;
}


static status_t
bootfs_sync(fs_volume fs)
{
	TRACE(("bootfs_sync: entry\n"));

	return 0;
}


static status_t
bootfs_lookup(fs_volume _fs, fs_vnode _dir, const char *name, vnode_id *_id, int *_type)
{
	struct bootfs *fs = (struct bootfs *)_fs;
	struct bootfs_vnode *dir = (struct bootfs_vnode *)_dir;
	struct bootfs_vnode *vnode, *vdummy;
	status_t status;

	TRACE(("bootfs_lookup: entry dir %p, name '%s'\n", dir, name));

	if (dir->stream.type != S_IFDIR)
		return B_NOT_A_DIRECTORY;

	mutex_lock(&fs->lock);

	// look it up
	vnode = bootfs_find_in_dir(dir, name);
	if (!vnode) {
		status = B_ENTRY_NOT_FOUND;
		goto err;
	}

	status = vfs_get_vnode(fs->id, vnode->id, (fs_vnode *)&vdummy);
	if (status < 0)
		goto err;

	*_id = vnode->id;
	*_type = dir->stream.type;

err:
	mutex_unlock(&fs->lock);

	return status;
}


static status_t
bootfs_get_vnode_name(fs_volume _fs, fs_vnode _vnode, char *buffer, size_t bufferSize)
{
	struct bootfs_vnode *vnode = (struct bootfs_vnode *)_vnode;

	TRACE(("devfs_get_vnode_name: vnode = %p\n",vnode));
	
	strlcpy(buffer,vnode->name,bufferSize);
	return B_OK;
}


static status_t
bootfs_get_vnode(fs_volume _fs, vnode_id id, fs_vnode *v, bool r)
{
	struct bootfs *fs = (struct bootfs *)_fs;

	TRACE(("bootfs_get_vnode: asking for vnode 0x%Lx, r %d\n", id, r));

	if (!r)
		mutex_lock(&fs->lock);

	*v = hash_lookup(fs->vnode_list_hash, &id);

	if (!r)
		mutex_unlock(&fs->lock);

	TRACE(("bootfs_get_vnode: looked it up at %p\n", *v));

	if (*v)
		return B_OK;

	return B_ENTRY_NOT_FOUND;
}


static status_t
bootfs_put_vnode(fs_volume _fs, fs_vnode _v, bool r)
{
	TRACE(("bootfs_put_vnode: entry on vnode 0x%Lx r %d\n", ((struct bootfs_vnode *)_v)->id, r));

	return 0; // whatever
}


static status_t
bootfs_remove_vnode(fs_volume _fs, fs_vnode _v, bool reenter)
{
	struct bootfs *fs = (struct bootfs *)_fs;
	struct bootfs_vnode *v = (struct bootfs_vnode *)_v;

	TRACE(("bootfs_remove_vnode: remove %p (0x%Lx) r %d\n", v, v->id, reenter));

	if (!reenter)
		mutex_lock(&fs->lock);

	if (v->dir_next) {
		// can't remove node if it's linked to the dir
		panic("bootfs_remove_vnode: vnode %p asked to be removed is present in dir\n", v);
	}

	bootfs_delete_vnode(fs, v, false);

	if (!reenter)
		mutex_unlock(&fs->lock);

	return B_OK;
}


static status_t
bootfs_create(fs_volume _fs, fs_vnode _dir, const char *name, int omode, int perms, fs_cookie *_cookie, vnode_id *new_vnid)
{
	return EROFS;
}


static status_t
bootfs_open(fs_volume _fs, fs_vnode _v, int oflags, fs_cookie *_cookie)
{
	struct bootfs *fs = _fs;
	struct bootfs_vnode *vnode = _v;
	struct bootfs_cookie *cookie;

	TRACE(("bootfs_open: vnode %p, oflags 0x%x\n", vnode, oflags));

	cookie = malloc(sizeof(struct bootfs_cookie));
	if (cookie == NULL)
		return ENOMEM;

	mutex_lock(&fs->lock);

	cookie->s = &vnode->stream;
	cookie->u.file.pos = 0;
	*_cookie = cookie;

	mutex_unlock(&fs->lock);

	return B_OK;
}


static status_t
bootfs_close(fs_volume _fs, fs_vnode _v, fs_cookie _cookie)
{
	TRACE(("bootfs_close: entry vnode %p, cookie %p\n", _v, _cookie));

	return B_OK;
}


static status_t
bootfs_free_cookie(fs_volume _fs, fs_vnode _v, fs_cookie _cookie)
{
	struct bootfs_cookie *cookie = _cookie;

	TRACE(("bootfs_freecookie: entry vnode %p, cookie %p\n", _v, cookie));

	free(cookie);
	return B_OK;
}


static status_t
bootfs_fsync(fs_volume _fs, fs_vnode _v)
{
	return B_OK;
}


static ssize_t
bootfs_read(fs_volume _fs, fs_vnode _v, fs_cookie _cookie, off_t pos, void *buffer, size_t *_length)
{
	struct bootfs *fs = _fs;
	struct bootfs_cookie *cookie = _cookie;
	ssize_t err = 0;

	TRACE(("bootfs_read: vnode %p, cookie %p, pos 0x%Lx, len 0x%lx\n", _v, cookie, pos, *_length));

	mutex_lock(&fs->lock);

	switch (cookie->s->type) {
		case S_IFREG:
			if (*_length <= 0) {
				err = 0;
				break;
			}
			if (pos < 0) {
				// we'll read where the cookie is at
				pos = cookie->u.file.pos;
			}
			if (pos >= cookie->s->u.file.len) {
				*_length = 0;
				err = ESPIPE;
				break;
			}
			if (pos + *_length > cookie->s->u.file.len) {
				// trim the read
				*_length = cookie->s->u.file.len - pos;
			}
			err = user_memcpy(buffer, cookie->s->u.file.start + pos, *_length);
			if (err < 0)
				goto err;

			cookie->u.file.pos = pos + *_length;
			err = 0;
			break;

		default:
			*_length = 0;
			err = EINVAL;
	}
err:
	mutex_unlock(&fs->lock);

	return err;
}


static ssize_t
bootfs_write(fs_volume fs, fs_vnode v, fs_cookie cookie, off_t pos, const void *buf, size_t *len)
{
	TRACE(("bootfs_write: vnode %p, cookie %p, pos 0x%Lx , len 0x%lx\n", v, cookie, pos, *len));

	return EROFS;
}


static status_t
bootfs_create_dir(fs_volume _fs, fs_vnode _dir, const char *name, int perms, vnode_id *new_vnid)
{
	return EROFS;
}


static status_t
bootfs_open_dir(fs_volume _fs, fs_vnode _v, fs_cookie *_cookie)
{
	struct bootfs *fs = _fs;
	struct bootfs_vnode *vnode = _v;
	struct bootfs_cookie *cookie;
	status_t status = 0;

	TRACE(("bootfs_open_dir: vnode %p\n", vnode));

	if (vnode->stream.type != S_IFDIR)
		return EINVAL;

	cookie = malloc(sizeof(struct bootfs_cookie));
	if (cookie == NULL)
		return ENOMEM;

	mutex_lock(&fs->lock);

	cookie->s = &vnode->stream;
	cookie->u.dir.ptr = vnode->stream.u.dir.dir_head;
	*_cookie = cookie;

	mutex_unlock(&fs->lock);

	return status;
}


static status_t
bootfs_read_dir(fs_volume _fs, fs_vnode _vnode, fs_cookie _cookie, struct dirent *dirent, size_t bufferSize, uint32 *_num)
{
	struct bootfs_cookie *cookie = _cookie;
	struct bootfs *fs = _fs;
	status_t status = B_OK;

	TRACE(("bootfs_read_dir(fs_volume = %p vnode = %p, fs_cookie = %p, buffer = %p, bufferSize = %ld, num = %ld)\n",_fs, _vnode, cookie, dirent, bufferSize, *_num));

	mutex_lock(&fs->lock);

	if (cookie->u.dir.ptr == NULL) {
		*_num = 0;
		goto err;
	}

	dirent->d_dev = fs->id;
	dirent->d_ino = cookie->u.dir.ptr->id;
	dirent->d_reclen = strlen(cookie->u.dir.ptr->name) + sizeof(struct dirent);

	if (dirent->d_reclen > bufferSize) {
		status = ENOBUFS;
		goto err;
	}

	status = user_strlcpy(dirent->d_name, cookie->u.dir.ptr->name, bufferSize - sizeof(struct dirent));
	if (status < B_OK)
		goto err;

	cookie->u.dir.ptr = cookie->u.dir.ptr->dir_next;
	status = B_OK;

err:
	mutex_unlock(&fs->lock);
	return status;
}


static status_t
bootfs_rewind_dir(fs_volume _fs, fs_vnode _vnode, fs_cookie _cookie)
{
	struct bootfs *fs = _fs;
	struct bootfs_vnode *vnode = _vnode;
	struct bootfs_cookie *cookie = _cookie;

	mutex_lock(&fs->lock);

	cookie->u.dir.ptr = vnode->stream.u.dir.dir_head;

	mutex_unlock(&fs->lock);
	return B_OK;
}


static status_t
bootfs_ioctl(fs_volume _fs, fs_vnode _v, fs_cookie _cookie, ulong op, void *buf, size_t len)
{
	TRACE(("bootfs_ioctl: fs_volume %p vnode %p, fs_cookie %p, op %lu, buf %p, len %ld\n", _fs, _v, _cookie, op, buf, len));
	return EINVAL;
}


static status_t
bootfs_can_page(fs_volume _fs, fs_vnode _v)
{
	struct bootfs_vnode *v = _v;

	TRACE(("bootfs_canpage: vnode %p\n", v));

	if (v->stream.type == S_IFREG)
		return 1;
	else
		return 0;
}


static ssize_t
bootfs_read_pages(fs_volume _fs, fs_vnode _v, iovecs *vecs, off_t pos)
{
	struct bootfs_vnode *v = _v;
	unsigned int i;

	TRACE(("bootfs_readpage: fs_cookie %p vnode %p, vecs %p, pos 0x%Ld\n", _fs, v, vecs, pos));

	for (i = 0; i < vecs->num; i++) {
		if (pos >= v->stream.u.file.len) {
			memset(vecs->vec[i].iov_base, 0, vecs->vec[i].iov_len);
			pos += vecs->vec[i].iov_len;
		} else {
			unsigned int copy_len;

			copy_len = min(vecs->vec[i].iov_len, v->stream.u.file.len - pos);

			memcpy(vecs->vec[i].iov_base, v->stream.u.file.start + pos, copy_len);

			if (copy_len < vecs->vec[i].iov_len)
				memset((char *)vecs->vec[i].iov_base + copy_len, 0, vecs->vec[i].iov_len - copy_len);

			pos += vecs->vec[i].iov_len;
		}
	}

	return EPERM;
}


static ssize_t
bootfs_write_pages(fs_volume _fs, fs_vnode _v, iovecs *vecs, off_t pos)
{
	TRACE(("bootfs_writepage: fs_cookie %p vnode %p, vecs %p, pos %Ld \n", _fs, _v, vecs, pos));

	return EPERM;
}


static status_t
bootfs_unlink(fs_volume _fs, fs_vnode _dir, const char *name)
{
	return EROFS;
}


static status_t
bootfs_rename(fs_volume _fs, fs_vnode _olddir, const char *oldname, fs_vnode _newdir, const char *newname)
{
	return EROFS;
}


static status_t
bootfs_read_stat(fs_volume _fs, fs_vnode _v, struct stat *stat)
{
	struct bootfs *fs = _fs;
	struct bootfs_vnode *v = _v;
	status_t err = 0;

	TRACE(("bootfs_rstat: fs_cookie %p vnode %p v->id 0x%Lx , stat %p\n", fs, v, v->id, stat));

	mutex_lock(&fs->lock);

    /* Read Only File System */
    /** XXX - no pretense at access control, just tell people
     *        they can read it :)
     */
	stat->st_ino = v->id;
	stat->st_mode = v->stream.type | S_IRUSR | S_IRGRP | S_IROTH;

	if (v->stream.type == S_IFREG)
		stat->st_size = v->stream.u.file.len;
	else
		stat->st_size = 0;

	mutex_unlock(&fs->lock);

	return err;
}


static status_t
bootfs_write_stat(fs_volume _fs, fs_vnode _v, const struct stat *stat, int statMask)
{
	// cannot change anything
	return EROFS;
}


//	#pragma mark -


static struct fs_ops bootfs_ops = {
	&bootfs_mount,
	&bootfs_unmount,
	NULL,
	NULL,
	&bootfs_sync,

	&bootfs_lookup,
	&bootfs_get_vnode_name,

	&bootfs_get_vnode,
	&bootfs_put_vnode,
	&bootfs_remove_vnode,

	&bootfs_can_page,
	&bootfs_read_pages,
	&bootfs_write_pages,

	/* common */
	&bootfs_ioctl,
	&bootfs_fsync,

	NULL,	// read_link
	NULL,	// write_link
	NULL,	// symlink
	NULL,	// link
	&bootfs_unlink,
	&bootfs_rename,

	NULL,	// access
	&bootfs_read_stat,
	&bootfs_write_stat,

	/* file */
	&bootfs_create,
	&bootfs_open,
	&bootfs_close,
	&bootfs_free_cookie,
	&bootfs_read,
	&bootfs_write,

	/* dir */
	&bootfs_create_dir,
	NULL,	// remove_dir
	&bootfs_open_dir,
	&bootfs_close,			// we are using the same operations for directories
	&bootfs_free_cookie,	// and files here - that's intended, not by accident
	&bootfs_read_dir,
	&bootfs_rewind_dir,

	// the other operations are not supported (attributes, indices, queries)
	NULL,
};


status_t
bootstrap_bootfs(void)
{
	area_id area;
	area_info areaInfo;

	TRACE(("bootstrap_bootfs: entry\n"));

	// find the bootdir and set it up
	area = find_area("bootdir");
	if (area < 0)
		panic("bootstrap_bootfs: no bootdir area found!\n");

	get_area_info(area, &areaInfo);
	bootdir = (char *)areaInfo.address;
	bootdir_len = areaInfo.size;
	bootdir_region = areaInfo.area;

	TRACE(("bootstrap_bootfs: found bootdir at %p\n", bootdir));

	return vfs_register_filesystem("bootfs", &bootfs_ops);
}

