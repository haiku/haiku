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
#include <arch/cpu.h>
#include <sys/stat.h>

#include <bootfs.h>

#include <bootdir.h>

#include <string.h>
#include <stdio.h>

#define BOOTFS_TRACE 0

#if BOOTFS_TRACE
#define TRACE(x) dprintf x
#else
#define TRACE(x)
#endif

static char *bootdir = NULL;
static off_t bootdir_len = 0;
static region_id bootdir_region = -1;

struct bootfs_stream {
	stream_type type;
	union {
		struct stream_dir {
			struct bootfs_vnode *dir_head;
			struct bootfs_cookie *jar_head;
		} dir;
		struct stream_file {
			void *start;
			off_t len;
		} file;
	} u;
};

struct bootfs_vnode {
	struct bootfs_vnode *all_next;
	vnode_id id;
	char *name;
	void *redir_vnode;
	struct bootfs_vnode *parent;
	struct bootfs_vnode *dir_next;
	struct bootfs_stream stream;
};

struct bootfs {
	fs_id id;
	mutex lock;
	int next_vnode_id;
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
static unsigned int bootfs_vnode_hash_func(void *_v, const void *_key, unsigned int range)
{
	struct bootfs_vnode *v = _v;
	const vnode_id *key = _key;

	if(v != NULL)
		return v->id % range;
	else
		return (*key) % range;
}

static int bootfs_vnode_compare_func(void *_v, const void *_key)
{
	struct bootfs_vnode *v = _v;
	const vnode_id *key = _key;

	if(v->id == *key)
		return 0;
	else
		return -1;
}

static struct bootfs_vnode *bootfs_create_vnode(struct bootfs *fs, const char *name)
{
	struct bootfs_vnode *v;

	v = kmalloc(sizeof(struct bootfs_vnode));
	if(v == NULL)
		return NULL;

	memset(v, 0, sizeof(struct bootfs_vnode));
	v->id = fs->next_vnode_id++;

	v->name = kstrdup(name);
	if(v->name == NULL) {
		kfree(v);
		return NULL;
	}

	return v;
}

static int bootfs_delete_vnode(struct bootfs *fs, struct bootfs_vnode *v, bool force_delete)
{
	// cant delete it if it's in a directory or is a directory
	// and has children
	if(!force_delete && ((v->stream.type == STREAM_TYPE_DIR && v->stream.u.dir.dir_head != NULL) || v->dir_next != NULL)) {
		return ERR_NOT_ALLOWED;
	}

	// remove it from the global hash table
	hash_remove(fs->vnode_list_hash, v);

	if(v->name != NULL)
		kfree(v->name);
	kfree(v);

	return 0;
}

static void insert_cookie_in_jar(struct bootfs_vnode *dir, struct bootfs_cookie *cookie)
{
	cookie->u.dir.next = dir->stream.u.dir.jar_head;
	dir->stream.u.dir.jar_head = cookie;
	cookie->u.dir.prev = NULL;
}

static void remove_cookie_from_jar(struct bootfs_vnode *dir, struct bootfs_cookie *cookie)
{
	if(cookie->u.dir.next)
		cookie->u.dir.next->u.dir.prev = cookie->u.dir.prev;
	if(cookie->u.dir.prev)
		cookie->u.dir.prev->u.dir.next = cookie->u.dir.next;
	if(dir->stream.u.dir.jar_head == cookie)
		dir->stream.u.dir.jar_head = cookie->u.dir.next;

	cookie->u.dir.prev = cookie->u.dir.next = NULL;
}

/* makes sure none of the dircookies point to the vnode passed in */
static void update_dircookies(struct bootfs_vnode *dir, struct bootfs_vnode *v)
{
	struct bootfs_cookie *cookie;

	for(cookie = dir->stream.u.dir.jar_head; cookie; cookie = cookie->u.dir.next) {
		if(cookie->u.dir.ptr == v) {
			cookie->u.dir.ptr = v->dir_next;
		}
	}
}


static struct bootfs_vnode *bootfs_find_in_dir(struct bootfs_vnode *dir, const char *path)
{
	struct bootfs_vnode *v;

	if(dir->stream.type != STREAM_TYPE_DIR)
		return NULL;

	if(!strcmp(path, "."))
		return dir;
	if(!strcmp(path, ".."))
		return dir->parent;

	for(v = dir->stream.u.dir.dir_head; v; v = v->dir_next) {
//		dprintf("bootfs_find_in_dir: looking at entry '%s'\n", v->name);
		if(strcmp(v->name, path) == 0) {
//			dprintf("bootfs_find_in_dir: found it at 0x%x\n", v);
			return v;
		}
	}
	return NULL;
}

static int bootfs_insert_in_dir(struct bootfs_vnode *dir, struct bootfs_vnode *v)
{
	if(dir->stream.type != STREAM_TYPE_DIR)
		return ERR_INVALID_ARGS;

	v->dir_next = dir->stream.u.dir.dir_head;
	dir->stream.u.dir.dir_head = v;

	v->parent = dir;
	return 0;
}

static int bootfs_remove_from_dir(struct bootfs_vnode *dir, struct bootfs_vnode *findit)
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

static int bootfs_is_dir_empty(struct bootfs_vnode *dir)
{
	if(dir->stream.type != STREAM_TYPE_DIR)
		return false;
	return !dir->stream.u.dir.dir_head;
}

// creates a path of vnodes up to the last part of the passed in path.
// returns the vnode the last segment should be a part of and
// a pointer to the leaf of the path.
// clobbers the path string passed in
static struct bootfs_vnode *bootfs_create_path(struct bootfs *fs, char *path, struct bootfs_vnode *base, char **path_leaf)
{
	struct bootfs_vnode *v;
	char *temp;
	bool done;

	// strip off any leading '/' or spaces
	for(; *path == '/' || *path == ' '; path++)
		;

	// first, find the leaf
	*path_leaf = strrchr(path, '/');
	if(*path_leaf == NULL) {
		// didn't find it, so this path only is a leaf
		*path_leaf = path;
		return base;
	}

	// this is a multipart path, seperate the leaf off
	**path_leaf = '\0';
	(*path_leaf)++;
	if(**path_leaf == '\0') {
		// the path ended with '/'. That's invalid
		return NULL;
	}

	// now, lets walk down the path, building vnodes as we need em
	done = false;
	for(; !done; path = temp+1) {
		// find the next seperator and knock it out
		temp = strchr(path, '/');
		if(temp) {
			*temp = '\0';
		} else {
			done = true;
		}

		if(*path == '\0') {
			// zero length path segment, continue
			continue;
		}

		v = bootfs_find_in_dir(base, path);
		if(!v) {
			v = bootfs_create_vnode(fs, path);
			if(!v)
				return NULL;

			v->stream.type = STREAM_TYPE_DIR;
			v->stream.u.dir.dir_head = NULL;
			v->stream.u.dir.jar_head = NULL;

			bootfs_insert_in_dir(base, v);
			hash_insert(fs->vnode_list_hash, v);
		} else {
			// we found one
			if(v->stream.type != STREAM_TYPE_DIR)
				return NULL;
		}
		base = v;
	}
	return base;
}

static int bootfs_create_vnode_tree(struct bootfs *fs, struct bootfs_vnode *root)
{
	int i;
	boot_entry *entry;
	int err;
	struct bootfs_vnode *new_vnode;
	struct bootfs_vnode *dir;
	char path[SYS_MAX_PATH_LEN];
	char *leaf;

	entry = (boot_entry *)bootdir;
	for(i=0; i<BOOTDIR_MAX_ENTRIES; i++) {
		if(entry[i].be_type != BE_TYPE_NONE && entry[i].be_type != BE_TYPE_DIRECTORY) {
			strncpy(path, entry[i].be_name, SYS_MAX_PATH_LEN-1);
			path[SYS_MAX_PATH_LEN-1] = '\0';

			dir = bootfs_create_path(fs, path, root, &leaf);
			if(!dir)
				continue;

			new_vnode = bootfs_create_vnode(fs, leaf);
			if(new_vnode == NULL)
				return ERR_NO_MEMORY;

			// set up the new node
			new_vnode->stream.type = STREAM_TYPE_FILE;
			new_vnode->stream.u.file.start = bootdir + entry[i].be_offset * PAGE_SIZE;
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

static int bootfs_mount(fs_cookie *_fs, fs_id id, const char *device, void *args, vnode_id *root_vnid)
{
	struct bootfs *fs;
	struct bootfs_vnode *v;
	int err;

	TRACE(("bootfs_mount: entry\n"));

	fs = kmalloc(sizeof(struct bootfs));
	if(fs == NULL) {
		err = ERR_NO_MEMORY;
		goto err;
	}

	fs->id = id;
	fs->next_vnode_id = 0;

	err = mutex_init(&fs->lock, "bootfs_mutex");
	if(err < 0) {
		goto err1;
	}

	fs->vnode_list_hash = hash_init(BOOTFS_HASH_SIZE, (addr)&v->all_next - (addr)v,
		&bootfs_vnode_compare_func, &bootfs_vnode_hash_func);
	if(fs->vnode_list_hash == NULL) {
		err = ERR_NO_MEMORY;
		goto err2;
	}

	// create a vnode
	v = bootfs_create_vnode(fs, "");
	if(v == NULL) {
		err = ERR_NO_MEMORY;
		goto err3;
	}

	// set it up
	v->parent = v;

	// create a dir stream for it to hold
	v->stream.type = STREAM_TYPE_DIR;
	v->stream.u.dir.dir_head = NULL;
	fs->root_vnode = v;

	hash_insert(fs->vnode_list_hash, v);

	err = bootfs_create_vnode_tree(fs, fs->root_vnode);
	if(err < 0) {
		goto err4;
	}

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
	kfree(fs);
err:
	return err;
}

static int bootfs_unmount(fs_cookie _fs)
{
	struct bootfs *fs = _fs;
	struct bootfs_vnode *v;
	struct hash_iterator i;

	TRACE(("bootfs_unmount: entry fs = 0x%x\n", fs));

	// delete all of the vnodes
	hash_open(fs->vnode_list_hash, &i);
	while((v = (struct bootfs_vnode *)hash_next(fs->vnode_list_hash, &i)) != NULL) {
		bootfs_delete_vnode(fs, v, true);
	}
	hash_close(fs->vnode_list_hash, &i, false);

	hash_uninit(fs->vnode_list_hash);
	mutex_destroy(&fs->lock);
	kfree(fs);

	return 0;
}

static int bootfs_sync(fs_cookie fs)
{
	TRACE(("bootfs_sync: entry\n"));

	return 0;
}

static int bootfs_lookup(fs_cookie _fs, fs_vnode _dir, const char *name, vnode_id *id)
{
	struct bootfs *fs = (struct bootfs *)_fs;
	struct bootfs_vnode *dir = (struct bootfs_vnode *)_dir;
	struct bootfs_vnode *v;
	struct bootfs_vnode *v1;
	int err;

	TRACE(("bootfs_lookup: entry dir 0x%x, name '%s'\n", dir, name));

	if(dir->stream.type != STREAM_TYPE_DIR)
		return ERR_VFS_NOT_DIR;

	mutex_lock(&fs->lock);

	// look it up
	v = bootfs_find_in_dir(dir, name);
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

static int bootfs_getvnode(fs_cookie _fs, vnode_id id, fs_vnode *v, bool r)
{
	struct bootfs *fs = (struct bootfs *)_fs;
	int err;

	TRACE(("bootfs_getvnode: asking for vnode 0x%x 0x%x, r %d\n", id, r));

	if(!r)
		mutex_lock(&fs->lock);

	*v = hash_lookup(fs->vnode_list_hash, &id);

	if(!r)
		mutex_unlock(&fs->lock);

	TRACE(("bootfs_getnvnode: looked it up at 0x%x\n", *v));

	if(*v)
		return 0;
	else
		return ENOENT;
}

static int bootfs_putvnode(fs_cookie _fs, fs_vnode _v, bool r)
{
	struct bootfs_vnode *v = (struct bootfs_vnode *)_v;

	TRACE(("bootfs_putvnode: entry on vnode 0x%x 0x%x, r %d\n", v->id, r));

	return 0; // whatever
}

static int bootfs_removevnode(fs_cookie _fs, fs_vnode _v, bool r)
{
	struct bootfs *fs = (struct bootfs *)_fs;
	struct bootfs_vnode *v = (struct bootfs_vnode *)_v;
	struct bootfs_vnode dummy;
	int err;

	TRACE(("bootfs_removevnode: remove 0x%x (0x%x 0x%x), r %d\n", v, v->id, r));

	if(!r)
		mutex_lock(&fs->lock);

	if(v->dir_next) {
		// can't remove node if it's linked to the dir
		panic("bootfs_removevnode: vnode %p asked to be removed is present in dir\n", v);
	}

	bootfs_delete_vnode(fs, v, false);

	err = 0;

err:
	if(!r)
		mutex_unlock(&fs->lock);

	return err;
}

static int bootfs_open(fs_cookie _fs, fs_vnode _v, file_cookie *_cookie, stream_type st, int oflags)
{
	struct bootfs *fs = _fs;
	struct bootfs_vnode *v = _v;
	struct bootfs_cookie *cookie;
	int err = 0;
	int start = 0;
dprintf("bootfs_open: \n");
	TRACE(("bootfs_open: vnode 0x%x, stream_type %d, oflags 0x%x\n", v, st, oflags));

	if(st != STREAM_TYPE_ANY && st != v->stream.type) {
		err = ERR_VFS_WRONG_STREAM_TYPE;
		goto err;
	}

	cookie = kmalloc(sizeof(struct bootfs_cookie));
	if(cookie == NULL) {
		err = ERR_NO_MEMORY;
		goto err;
	}

	mutex_lock(&fs->lock);

	cookie->s = &v->stream;
	switch(v->stream.type) {
		case STREAM_TYPE_DIR:
			cookie->u.dir.ptr = v->stream.u.dir.dir_head;
			break;
		case STREAM_TYPE_FILE:
			cookie->u.file.pos = 0;
			break;
		default:
			err = ERR_VFS_WRONG_STREAM_TYPE;
			kfree(cookie);
	}

	*_cookie = cookie;

err1:
	mutex_unlock(&fs->lock);
err:
	return err;
}

static int bootfs_close(fs_cookie _fs, fs_vnode _v, file_cookie _cookie)
{
	struct bootfs *fs = _fs;
	struct bootfs_vnode *v = _v;
	struct bootfs_cookie *cookie = _cookie;

	TRACE(("bootfs_close: entry vnode 0x%x, cookie 0x%x\n", v, cookie));

	return 0;
}

static int bootfs_freecookie(fs_cookie _fs, fs_vnode _v, file_cookie _cookie)
{
	struct bootfs *fs = _fs;
	struct bootfs_vnode *v = _v;
	struct bootfs_cookie *cookie = _cookie;

	TRACE(("bootfs_freecookie: entry vnode 0x%x, cookie 0x%x\n", v, cookie));

	if(cookie)
		kfree(cookie);

	return 0;
}

static int bootfs_fsync(fs_cookie _fs, fs_vnode _v)
{
	return 0;
}

static ssize_t bootfs_read(fs_cookie _fs, fs_vnode _v, file_cookie _cookie, void *buf, off_t pos, size_t *len)
{
	struct bootfs *fs = _fs;
	struct bootfs_vnode *v = _v;
	struct bootfs_cookie *cookie = _cookie;
	ssize_t err = 0;

	TRACE(("bootfs_read: vnode 0x%x, cookie 0x%x, pos 0x%x 0x%x, len 0x%x\n", v, cookie, pos, len));

	mutex_lock(&fs->lock);

	switch(cookie->s->type) {
		case STREAM_TYPE_DIR: {
			if(cookie->u.dir.ptr == NULL) {
				*len = 0;
				err = ENOENT;
				break;
			}

			if((ssize_t)strlen(cookie->u.dir.ptr->name) + 1 > *len) {
				err = ERR_VFS_INSUFFICIENT_BUF;
				goto err;
			}

			err = user_strcpy(buf, cookie->u.dir.ptr->name);
			if(err < 0)
				goto err;

			err = strlen(cookie->u.dir.ptr->name) + 1;

			cookie->u.dir.ptr = cookie->u.dir.ptr->dir_next;
			break;
		}
		case STREAM_TYPE_FILE:
			if(*len <= 0) {
				err = 0;
				break;
			}
			if(pos < 0) {
				// we'll read where the cookie is at
				pos = cookie->u.file.pos;
			}
			if(pos >= cookie->s->u.file.len) {
				*len = 0;
				err = ESPIPE;
				break;
			}
			if(pos + *len > cookie->s->u.file.len) {
				// trim the read
				*len = cookie->s->u.file.len - pos;
			}
			err = user_memcpy(buf, cookie->s->u.file.start + pos, *len);
			if(err < 0) {
				goto err;
			}
			cookie->u.file.pos = pos + *len;
			err = 0;
			break;
		default:
			*len = 0;
			err = EINVAL;
	}
err:
	mutex_unlock(&fs->lock);

	return err;
}

static ssize_t bootfs_write(fs_cookie fs, fs_vnode v, file_cookie cookie, const void *buf, off_t pos, size_t *len)
{
	TRACE(("bootfs_write: vnode 0x%x, cookie 0x%x, pos 0x%x 0x%x, len 0x%x\n", v, cookie, pos, *len));

	return EROFS;
}

static int bootfs_seek(fs_cookie _fs, fs_vnode _v, file_cookie _cookie, off_t pos, int st)
{
	struct bootfs *fs = _fs;
	struct bootfs_vnode *v = _v;
	struct bootfs_cookie *cookie = _cookie;
	int err = 0;

	TRACE(("bootfs_seek: vnode 0x%x, cookie 0x%x, pos 0x%x 0x%x, seek_type %d\n", v, cookie, pos, st));

	mutex_lock(&fs->lock);

	switch(cookie->s->type) {
		case STREAM_TYPE_DIR:
			switch(st) {
				// only valid args are seek_type SEEK_SET, pos 0.
				// this rewinds to beginning of directory
				case SEEK_SET:
					if(pos == 0) {
						cookie->u.dir.ptr = cookie->s->u.dir.dir_head;
					} else {
						err = ESPIPE;
					}
					break;
				case SEEK_CUR:
				case SEEK_END:
				default:
					err = ESPIPE;
			}
			break;
		case STREAM_TYPE_FILE:
			switch(st) {
				case SEEK_SET:
					if(pos < 0)
						pos = 0;
					if(pos > cookie->s->u.file.len)
						pos = cookie->s->u.file.len;
					cookie->u.file.pos = pos;
					break;
				case SEEK_CUR:
					if(pos + cookie->u.file.pos > cookie->s->u.file.len)
						cookie->u.file.pos = cookie->s->u.file.len;
					else if(pos + cookie->u.file.pos < 0)
						cookie->u.file.pos = 0;
					else
						cookie->u.file.pos += pos;
					break;
				case SEEK_END:
					if(pos > 0)
						cookie->u.file.pos = cookie->s->u.file.len;
					else if(pos + cookie->s->u.file.len < 0)
						cookie->u.file.pos = 0;
					else
						cookie->u.file.pos = pos + cookie->s->u.file.len;
					break;
				default:
					err = ERR_INVALID_ARGS;

			}
			break;
		default:
			err = ERR_INVALID_ARGS;
	}

	mutex_unlock(&fs->lock);

	return err;
}


static int
bootfs_read_dir(fs_cookie _fs, fs_vnode _vnode, file_cookie _cookie, struct dirent *buffer, size_t bufferSize, uint32 *_num)
{
	// ToDo: implement me!
	return B_OK;
}


static int
bootfs_rewind_dir(fs_cookie _fs, fs_vnode _vnode, file_cookie _cookie)
{
	// ToDo: me too!
	return B_OK;
}


static int
bootfs_ioctl(fs_cookie _fs, fs_vnode _v, file_cookie _cookie, ulong op, void *buf, size_t len)
{
	TRACE(("bootfs_ioctl: vnode 0x%x, cookie 0x%x, op %d, buf 0x%x, len 0x%x\n", _v, _cookie, op, buf, len));
	return EINVAL;
}


static int
bootfs_canpage(fs_cookie _fs, fs_vnode _v)
{
	struct bootfs_vnode *v = _v;

	TRACE(("bootfs_canpage: vnode 0x%x\n", v));

	if(v->stream.type == STREAM_TYPE_FILE)
		return 1;
	else
		return 0;
}

static ssize_t bootfs_readpage(fs_cookie _fs, fs_vnode _v, iovecs *vecs, off_t pos)
{
	struct bootfs *fs = _fs;
	struct bootfs_vnode *v = _v;
	unsigned int i;

	TRACE(("bootfs_readpage: vnode 0x%x, vecs 0x%x, pos 0x%x 0x%x\n", v, vecs, pos));

	for(i=0; i<vecs->num; i++) {
		if(pos >= v->stream.u.file.len) {
			memset(vecs->vec[i].iov_base, 0, vecs->vec[i].iov_len);
			pos += vecs->vec[i].iov_len;
		} else {
			unsigned int copy_len;

			copy_len = min(vecs->vec[i].iov_len, v->stream.u.file.len - pos);

			memcpy(vecs->vec[i].iov_base, v->stream.u.file.start + pos, copy_len);

			if(copy_len < vecs->vec[i].iov_len)
				memset((char *)vecs->vec[i].iov_base + copy_len, 0, vecs->vec[i].iov_len - copy_len);

			pos += vecs->vec[i].iov_len;
		}
	}

	return EPERM;
}

static ssize_t bootfs_writepage(fs_cookie _fs, fs_vnode _v, iovecs *vecs, off_t pos)
{
	struct bootfs *fs = _fs;
	struct bootfs_vnode *v = _v;

	TRACE(("bootfs_writepage: vnode 0x%x, vecs 0x%x, pos 0x%x 0x%x\n", v, vecs, pos));

	return ERR_NOT_ALLOWED;
}

static int bootfs_create(fs_cookie _fs, fs_vnode _dir, const char *name, stream_type st, void *create_args, vnode_id *new_vnid)
{
	return ERR_VFS_READONLY_FS;
}

static int bootfs_unlink(fs_cookie _fs, fs_vnode _dir, const char *name)
{
	return ERR_VFS_READONLY_FS;
}

static int bootfs_rename(fs_cookie _fs, fs_vnode _olddir, const char *oldname, fs_vnode _newdir, const char *newname)
{
	return ERR_VFS_READONLY_FS;
}

static int bootfs_rstat(fs_cookie _fs, fs_vnode _v, struct stat *stat)
{
	struct bootfs *fs = _fs;
	struct bootfs_vnode *v = _v;
	int err = 0;
//dprintf("bootfs_rstat:\n");
	TRACE(("bootfs_rstat: vnode 0x%x (0x%x 0x%x), stat 0x%x\n", v, v->id, stat));

	mutex_lock(&fs->lock);

    /* Read Only File System */
    /** XXX - no pretense at access control, just tell people
     *        they can read it :)
     */
	stat->st_mode = (S_IRUSR | S_IRGRP | S_IROTH);
	stat->st_size = 0;
	stat->st_ino = v->id;

	switch(v->stream.type) {
		case STREAM_TYPE_DIR:
			stat->st_mode |= S_IFDIR;
			break;
		case STREAM_TYPE_FILE:
			stat->st_size = v->stream.u.file.len;
			stat->st_mode |= S_IFREG;
			break;
		default:
			err = EINVAL;
			break;
	}

err:
	mutex_unlock(&fs->lock);

	return err;
}


static int bootfs_wstat(fs_cookie _fs, fs_vnode _v, struct stat *stat, int stat_mask)
{
	struct bootfs *fs = _fs;
	struct bootfs_vnode *v = _v;

	TRACE(("bootfs_wstat: vnode 0x%x (0x%x 0x%x), stat 0x%x\n", v, v->id, stat));

	// cannot change anything
	return ERR_VFS_READONLY_FS;
}

static struct fs_calls bootfs_calls = {
	&bootfs_mount,
	&bootfs_unmount,
	&bootfs_sync,

	&bootfs_lookup,

	&bootfs_getvnode,
	&bootfs_putvnode,
	&bootfs_removevnode,

	&bootfs_open,
	&bootfs_close,
	&bootfs_freecookie,
	&bootfs_fsync,

	&bootfs_read,
	&bootfs_write,
	&bootfs_seek,

	&bootfs_read_dir,
	&bootfs_rewind_dir,

	&bootfs_ioctl,

	&bootfs_canpage,
	&bootfs_readpage,
	&bootfs_writepage,

	&bootfs_create,
	&bootfs_unlink,
	&bootfs_rename,

	&bootfs_rstat,
	&bootfs_wstat,
};


int
bootstrap_bootfs(void)
{
	region_id rid;
	vm_region_info rinfo;

	dprintf("bootstrap_bootfs: entry\n");

	// find the bootdir and set it up
	rid = vm_find_region_by_name(vm_get_kernel_aspace_id(), "bootdir");
	if(rid < 0)
		panic("bootstrap_bootfs: no bootdir area found!\n");
	vm_get_region_info(rid, &rinfo);
	bootdir = (char *)rinfo.base;
	bootdir_len = rinfo.size;
	bootdir_region = rinfo.id;

	dprintf("bootstrap_bootfs: found bootdir at %p\n", bootdir);

	return vfs_register_filesystem("bootfs", &bootfs_calls);
}

