/*
 * Copyright 2002-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
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


//#define TRACE_ROOTFS
#ifdef TRACE_ROOTFS
#	define TRACE(x) dprintf x
#else
#	define TRACE(x)
#endif


struct rootfs_stream {
	mode_t type;
	struct stream_dir {
		struct rootfs_vnode *dir_head;
		struct list cookies;
	} dir;
	struct stream_symlink {
		char *path;
		size_t length;
	} symlink;
};

struct rootfs_vnode {
	struct rootfs_vnode *all_next;
	vnode_id	id;
	char		*name;
	time_t		modification_time;
	time_t		creation_time;
	uid_t		uid;
	gid_t		gid;
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
struct rootfs_dir_cookie {
	struct list_link link;
	struct rootfs_vnode *current;
	int32 state;	// iteration state
};

// directory iteration states
enum {
	ITERATION_STATE_DOT		= 0,
	ITERATION_STATE_DOT_DOT	= 1,
	ITERATION_STATE_OTHERS	= 2,
	ITERATION_STATE_BEGIN	= ITERATION_STATE_DOT,
};

#define ROOTFS_HASH_SIZE 16


static uint32
rootfs_vnode_hash_func(void *_v, const void *_key, uint32 range)
{
	struct rootfs_vnode *vnode = _v;
	const vnode_id *key = _key;

	if (vnode != NULL)
		return vnode->id % range;

	return (uint64)*key % range;
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
rootfs_create_vnode(struct rootfs *fs, struct rootfs_vnode *parent,
	const char *name, int type)
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
	vnode->creation_time = vnode->modification_time = time(NULL);
	vnode->uid = geteuid();
	vnode->gid = parent ? parent->gid : getegid();
		// inherit group from parent if possible

	if (S_ISDIR(type))
		list_init(&vnode->stream.dir.cookies);

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

	free(v->name);
	free(v);

	return 0;
}


/* makes sure none of the dircookies point to the vnode passed in */

static void
update_dir_cookies(struct rootfs_vnode *dir, struct rootfs_vnode *vnode)
{
	struct rootfs_dir_cookie *cookie = NULL;

	while ((cookie = list_get_next_item(&dir->stream.dir.cookies, cookie)) != NULL) {
		if (cookie->current == vnode)
			cookie->current = vnode->dir_next;
	}
}


static struct rootfs_vnode *
rootfs_find_in_dir(struct rootfs_vnode *dir, const char *path)
{
	struct rootfs_vnode *vnode;

	if (!strcmp(path, "."))
		return dir;
	if (!strcmp(path, ".."))
		return dir->parent;

	for (vnode = dir->stream.dir.dir_head; vnode; vnode = vnode->dir_next) {
		if (strcmp(vnode->name, path) == 0)
			return vnode;
	}
	return NULL;
}


static status_t
rootfs_insert_in_dir(struct rootfs *fs, struct rootfs_vnode *dir,
	struct rootfs_vnode *vnode)
{
	// make sure the directory stays sorted alphabetically

	struct rootfs_vnode *node = dir->stream.dir.dir_head, *last = NULL;
	while (node && strcmp(node->name, vnode->name) < 0) {
		last = node;
		node = node->dir_next;
	}
	if (last == NULL) {
		// the new vnode is the first entry in the list
		vnode->dir_next = dir->stream.dir.dir_head;
		dir->stream.dir.dir_head = vnode;
	} else {
		// insert after that node
		vnode->dir_next = last->dir_next;
		last->dir_next = vnode;
	}

	vnode->parent = dir;
	dir->modification_time = time(NULL);

	notify_stat_changed(fs->id, dir->id, B_STAT_MODIFICATION_TIME);
	return B_OK;
}


static status_t
rootfs_remove_from_dir(struct rootfs *fs, struct rootfs_vnode *dir,
	struct rootfs_vnode *findit)
{
	struct rootfs_vnode *v;
	struct rootfs_vnode *last_v;

	for (v = dir->stream.dir.dir_head, last_v = NULL; v; last_v = v, v = v->dir_next) {
		if (v == findit) {
			/* make sure all dircookies dont point to this vnode */
			update_dir_cookies(dir, v);

			if (last_v)
				last_v->dir_next = v->dir_next;
			else
				dir->stream.dir.dir_head = v->dir_next;
			v->dir_next = NULL;

			dir->modification_time = time(NULL);
			notify_stat_changed(fs->id, dir->id, B_STAT_MODIFICATION_TIME);
			return B_OK;
		}
	}
	return B_ENTRY_NOT_FOUND;
}


static bool
rootfs_is_dir_empty(struct rootfs_vnode *dir)
{
	return !dir->stream.dir.dir_head;
}


/** You must hold the FS lock when calling this function */

static status_t
remove_node(struct rootfs *fs, struct rootfs_vnode *directory, struct rootfs_vnode *vnode)
{
	// schedule this vnode to be removed when it's ref goes to zero
	status_t status = remove_vnode(fs->id, vnode->id);
	if (status < B_OK)
		return status;

	rootfs_remove_from_dir(fs, directory, vnode);
	notify_entry_removed(fs->id, directory->id, vnode->name, vnode->id);

	return B_OK;
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
	else if (isDirectory && !S_ISDIR(vnode->stream.type))
		status = B_NOT_A_DIRECTORY;
	else if (!isDirectory && S_ISDIR(vnode->stream.type))
		status = B_IS_A_DIRECTORY;
	else if (isDirectory && !rootfs_is_dir_empty(vnode))
		status = B_DIRECTORY_NOT_EMPTY;

	if (status < B_OK)
		goto err;

	status = remove_node(fs, dir, vnode);

err:
	mutex_unlock(&fs->lock);

	return status;
}


//	#pragma mark -


static status_t
rootfs_mount(mount_id id, const char *device, uint32 flags, const char *args,
	fs_volume *_fs, vnode_id *root_vnid)
{
	struct rootfs *fs;
	struct rootfs_vnode *vnode;
	status_t err;

	TRACE(("rootfs_mount: entry\n"));

	fs = malloc(sizeof(struct rootfs));
	if (fs == NULL)
		return B_NO_MEMORY;

	fs->id = id;
	fs->next_vnode_id = 1;

	err = mutex_init(&fs->lock, "rootfs_mutex");
	if (err < B_OK)
		goto err1;

	fs->vnode_list_hash = hash_init(ROOTFS_HASH_SIZE, (addr_t)&vnode->all_next - (addr_t)vnode,
		&rootfs_vnode_compare_func, &rootfs_vnode_hash_func);
	if (fs->vnode_list_hash == NULL) {
		err = B_NO_MEMORY;
		goto err2;
	}

	// create the root vnode
	vnode = rootfs_create_vnode(fs, NULL, ".", S_IFDIR | 0777);
	if (vnode == NULL) {
		err = B_NO_MEMORY;
		goto err3;
	}
	vnode->parent = vnode;

	fs->root_vnode = vnode;
	hash_insert(fs->vnode_list_hash, vnode);
	publish_vnode(id, vnode->id, vnode);

	*root_vnid = vnode->id;
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
rootfs_unmount(fs_volume _fs)
{
	struct rootfs *fs = (struct rootfs *)_fs;
	struct rootfs_vnode *v;
	struct hash_iterator i;

	TRACE(("rootfs_unmount: entry fs = %p\n", fs));

	// release the reference to the root
	put_vnode(fs->id, fs->root_vnode->id);

	// delete all of the vnodes
	hash_open(fs->vnode_list_hash, &i);
	while ((v = (struct rootfs_vnode *)hash_next(fs->vnode_list_hash, &i)) != NULL) {
		rootfs_delete_vnode(fs, v, true);
	}
	hash_close(fs->vnode_list_hash, &i, false);

	hash_uninit(fs->vnode_list_hash);
	mutex_destroy(&fs->lock);
	free(fs);

	return B_OK;
}


static status_t
rootfs_sync(fs_volume fs)
{
	TRACE(("rootfs_sync: entry\n"));

	return B_OK;
}


static status_t
rootfs_lookup(fs_volume _fs, fs_vnode _dir, const char *name, vnode_id *_id, int *_type)
{
	struct rootfs *fs = (struct rootfs *)_fs;
	struct rootfs_vnode *dir = (struct rootfs_vnode *)_dir;
	struct rootfs_vnode *vnode,*vdummy;
	status_t status;

	TRACE(("rootfs_lookup: entry dir %p, name '%s'\n", dir, name));
	if (!S_ISDIR(dir->stream.type))
		return B_NOT_A_DIRECTORY;

	mutex_lock(&fs->lock);

	// look it up
	vnode = rootfs_find_in_dir(dir, name);
	if (!vnode) {
		status = B_ENTRY_NOT_FOUND;
		goto err;
	}

	status = get_vnode(fs->id, vnode->id, (fs_vnode *)&vdummy);
	if (status < B_OK)
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

	TRACE(("rootfs_get_vnode_name: vnode = %p (name = %s)\n", vnode, vnode->name));

	strlcpy(buffer, vnode->name, bufferSize);
	return B_OK;
}


static status_t
rootfs_get_vnode(fs_volume _fs, vnode_id id, fs_vnode *_vnode, bool reenter)
{
	struct rootfs *fs = (struct rootfs *)_fs;
	struct rootfs_vnode *vnode;

	TRACE(("rootfs_getvnode: asking for vnode %Ld, r %d\n", id, reenter));

	if (!reenter)
		mutex_lock(&fs->lock);

	vnode = hash_lookup(fs->vnode_list_hash, &id);

	if (!reenter)
		mutex_unlock(&fs->lock);

	TRACE(("rootfs_getnvnode: looked it up at %p\n", *_vnode));

	if (vnode == NULL)
		return B_ENTRY_NOT_FOUND;

	*_vnode = vnode;
	return B_OK;
}


static status_t
rootfs_put_vnode(fs_volume _fs, fs_vnode _vnode, bool reenter)
{
#ifdef TRACE_ROOTFS
	struct rootfs_vnode *vnode = (struct rootfs_vnode *)_vnode;

	TRACE(("rootfs_putvnode: entry on vnode 0x%Lx, r %d\n", vnode->id, reenter));
#endif
	return B_OK; // whatever
}


static status_t
rootfs_remove_vnode(fs_volume _fs, fs_vnode _vnode, bool reenter)
{
	struct rootfs *fs = (struct rootfs *)_fs;
	struct rootfs_vnode *vnode = (struct rootfs_vnode *)_vnode;

	TRACE(("rootfs_remove_vnode: remove %p (0x%Lx), r %d\n", vnode, vnode->id, reenter));

	if (!reenter)
		mutex_lock(&fs->lock);

	if (vnode->dir_next) {
		// can't remove node if it's linked to the dir
		panic("rootfs_remove_vnode: vnode %p asked to be removed is present in dir\n", vnode);
	}

	rootfs_delete_vnode(fs, vnode, false);

	if (!reenter)
		mutex_unlock(&fs->lock);

	return B_OK;
}


static status_t
rootfs_create(fs_volume _fs, fs_vnode _dir, const char *name, int omode, int perms, fs_cookie *_cookie, vnode_id *new_vnid)
{
	return B_BAD_VALUE;
}


static status_t
rootfs_open(fs_volume _fs, fs_vnode _v, int oflags, fs_cookie *_cookie)
{
	// allow to open the file, but it can't be done anything with it

	*_cookie = NULL;
	return B_OK;
}


static status_t
rootfs_close(fs_volume _fs, fs_vnode _v, fs_cookie _cookie)
{
#ifdef TRACE_ROOTFS
	struct rootfs_vnode *v = _v;
	struct rootfs_cookie *cookie = _cookie;

	TRACE(("rootfs_close: entry vnode %p, cookie %p\n", v, cookie));
#endif
	return B_OK;
}


static status_t
rootfs_free_cookie(fs_volume _fs, fs_vnode _v, fs_cookie _cookie)
{
	return B_OK;
}


static status_t
rootfs_fsync(fs_volume _fs, fs_vnode _v)
{
	return B_OK;
}


static status_t
rootfs_read(fs_volume _fs, fs_vnode _vnode, fs_cookie _cookie, 
	off_t pos, void *buffer, size_t *_length)
{
	return EINVAL;
}


static status_t
rootfs_write(fs_volume fs, fs_vnode vnode, fs_cookie cookie, 
	off_t pos, const void *buffer, size_t *_length)
{
	TRACE(("rootfs_write: vnode %p, cookie %p, pos 0x%Lx , len 0x%lx\n", 
		vnode, cookie, pos, *_length));

	return EPERM;
}


static status_t
rootfs_create_dir(fs_volume _fs, fs_vnode _dir, const char *name, int mode, vnode_id *_newID)
{
	struct rootfs *fs = _fs;
	struct rootfs_vnode *dir = _dir;
	struct rootfs_vnode *vnode;
	status_t status = 0;

	TRACE(("rootfs_create_dir: dir %p, name = '%s', perms = %d, id = 0x%Lx pointer id = %p\n",
		dir, name, mode,*_newID, _newID));

	mutex_lock(&fs->lock);

	vnode = rootfs_find_in_dir(dir, name);
	if (vnode != NULL) {
		status = B_FILE_EXISTS;
		goto err;
	}

	TRACE(("rootfs_create: creating new vnode\n"));
	vnode = rootfs_create_vnode(fs, dir, name, S_IFDIR | (mode & S_IUMSK));
	if (vnode == NULL) {
		status = B_NO_MEMORY;
		goto err;
	}

	rootfs_insert_in_dir(fs, dir, vnode);
	hash_insert(fs->vnode_list_hash, vnode);

	notify_entry_created(fs->id, dir->id, name, vnode->id);

	mutex_unlock(&fs->lock);	
	return B_OK;

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
	struct rootfs_dir_cookie *cookie;

	TRACE(("rootfs_open: vnode %p\n", vnode));

	if (!S_ISDIR(vnode->stream.type))
		return B_BAD_VALUE;

	cookie = malloc(sizeof(struct rootfs_dir_cookie));
	if (cookie == NULL)
		return B_NO_MEMORY;

	mutex_lock(&fs->lock);

	cookie->current = vnode->stream.dir.dir_head;
	cookie->state = ITERATION_STATE_BEGIN;

	list_add_item(&vnode->stream.dir.cookies, cookie);
	*_cookie = cookie;

	mutex_unlock(&fs->lock);

	return B_OK;
}


static status_t
rootfs_free_dir_cookie(fs_volume _fs, fs_vnode _vnode, fs_cookie _cookie)
{
	struct rootfs_dir_cookie *cookie = _cookie;
	struct rootfs_vnode *vnode = _vnode;
	struct rootfs *fs = _fs;

	mutex_lock(&fs->lock);
	list_remove_item(&vnode->stream.dir.cookies, cookie);
	mutex_unlock(&fs->lock);

	free(cookie);
	return B_OK;
}


static status_t
rootfs_read_dir(fs_volume _fs, fs_vnode _vnode, fs_cookie _cookie, struct dirent *dirent, size_t bufferSize, uint32 *_num)
{
	struct rootfs_vnode *vnode = (struct rootfs_vnode *)_vnode;
	struct rootfs_dir_cookie *cookie = _cookie;
	struct rootfs *fs = _fs;
	status_t status = B_OK;
	struct rootfs_vnode *childNode = NULL;
	const char *name = NULL;
	struct rootfs_vnode *nextChildNode = NULL;
	int nextState = cookie->state;

	TRACE(("rootfs_read_dir: vnode %p, cookie %p, buffer = %p, bufferSize = %ld, num = %p\n", _vnode, cookie, dirent, bufferSize,_num));

	mutex_lock(&fs->lock);

	switch (cookie->state) {
		case ITERATION_STATE_DOT:
			childNode = vnode;
			name = ".";
			nextChildNode = vnode->stream.dir.dir_head;
			nextState = cookie->state + 1;
			break;
		case ITERATION_STATE_DOT_DOT:
			childNode = vnode->parent;
			name = "..";
			nextChildNode = vnode->stream.dir.dir_head;
			nextState = cookie->state + 1;
			break;
		default:
			childNode = cookie->current;
			if (childNode) {
				name = childNode->name;
				nextChildNode = childNode->dir_next;
			}
			break;
	}

	if (!childNode) {
		// we're at the end of the directory
		*_num = 0;
		goto err;
	}

	dirent->d_dev = fs->id;
	dirent->d_ino = childNode->id;
	dirent->d_reclen = strlen(name) + sizeof(struct dirent);

	if (dirent->d_reclen > bufferSize) {
		status = ENOBUFS;
		goto err;
	}

	status = user_strlcpy(dirent->d_name, name,
		bufferSize - sizeof(struct dirent));
	if (status < B_OK)
		goto err;

	cookie->current = nextChildNode;
	cookie->state = nextState;
	status = B_OK;

err:
	mutex_unlock(&fs->lock);

	return status;
}


static status_t
rootfs_rewind_dir(fs_volume _fs, fs_vnode _vnode, fs_cookie _cookie)
{
	struct rootfs_dir_cookie *cookie = _cookie;
	struct rootfs_vnode *vnode = _vnode;
	struct rootfs *fs = _fs;

	mutex_lock(&fs->lock);

	cookie->current = vnode->stream.dir.dir_head;
	cookie->state = ITERATION_STATE_BEGIN;

	mutex_unlock(&fs->lock);
	return B_OK;
}


static status_t
rootfs_ioctl(fs_volume _fs, fs_vnode _v, fs_cookie _cookie, ulong op, void *buf, size_t len)
{
	TRACE(("rootfs_ioctl: vnode %p, cookie %p, op %ld, buf %p, len %ld\n", _v, _cookie, op, buf, len));

	return EINVAL;
}


static bool
rootfs_can_page(fs_volume _fs, fs_vnode _v, fs_cookie cookie)
{
	return false;
}


static status_t
rootfs_read_pages(fs_volume _fs, fs_vnode _v, fs_cookie cookie, off_t pos,
	const iovec *vecs, size_t count, size_t *_numBytes, bool reenter)
{
	return B_NOT_ALLOWED;
}


static status_t
rootfs_write_pages(fs_volume _fs, fs_vnode _v, fs_cookie cookie, off_t pos,
	const iovec *vecs, size_t count, size_t *_numBytes, bool reenter)
{
	return B_NOT_ALLOWED;
}


static status_t
rootfs_read_link(fs_volume _fs, fs_vnode _link, char *buffer, size_t *_bufferSize)
{
	struct rootfs_vnode *link = _link;
	size_t bufferSize = *_bufferSize;

	if (!S_ISLNK(link->stream.type))
		return B_BAD_VALUE;

	*_bufferSize = link->stream.symlink.length + 1;
		// we always need to return the number of bytes we intend to write!

	if (bufferSize <= link->stream.symlink.length)
		return B_BUFFER_OVERFLOW;

	memcpy(buffer, link->stream.symlink.path, link->stream.symlink.length + 1);
	return B_OK;
}


static status_t
rootfs_symlink(fs_volume _fs, fs_vnode _dir, const char *name, const char *path, int mode)
{
	struct rootfs *fs = _fs;
	struct rootfs_vnode *dir = _dir;
	struct rootfs_vnode *vnode;
	status_t status = B_OK;

	TRACE(("rootfs_symlink: dir %p, name = '%s', path = %s\n", dir, name, path));

	mutex_lock(&fs->lock);

	vnode = rootfs_find_in_dir(dir, name);
	if (vnode != NULL) {
		status = B_FILE_EXISTS;
		goto err;
	}

	TRACE(("rootfs_create: creating new symlink\n"));
	vnode = rootfs_create_vnode(fs, dir, name, S_IFLNK | (mode & S_IUMSK));
	if (vnode == NULL) {
		status = B_NO_MEMORY;
		goto err;
	}

	rootfs_insert_in_dir(fs, dir, vnode);
	hash_insert(fs->vnode_list_hash, vnode);

	vnode->stream.symlink.path = strdup(path);
	if (vnode->stream.symlink.path == NULL) {
		status = B_NO_MEMORY;
		goto err1;
	}
	vnode->stream.symlink.length = strlen(path);

	notify_entry_created(fs->id, dir->id, name, vnode->id);

	mutex_unlock(&fs->lock);
	return B_OK;

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
rootfs_rename(fs_volume _fs, fs_vnode _fromDir, const char *fromName, fs_vnode _toDir, const char *toName)
{
	struct rootfs *fs = _fs;
	struct rootfs_vnode *fromDirectory = _fromDir;
	struct rootfs_vnode *toDirectory = _toDir;
	struct rootfs_vnode *vnode, *targetVnode, *parent;
	status_t status;
	char *nameBuffer = NULL;

	TRACE(("rootfs_rename: from %p (0x%Lx), fromName '%s', to %p (0x%Lx), toName '%s'\n",
		fromDirectory, fromDirectory->id, fromName, toDirectory, toDirectory->id, toName));

	mutex_lock(&fs->lock);

	vnode = rootfs_find_in_dir(fromDirectory, fromName);
	if (vnode != NULL) {
		status = B_ENTRY_NOT_FOUND;
		goto err;
	}

	// make sure the target not a subdirectory of us
	parent = toDirectory->parent;
	while (parent != NULL) {
		if (parent == vnode) {
			status = B_BAD_VALUE;
			goto err;
		}

		parent = parent->parent;
	}

	// we'll reuse the name buffer if possible
	if (strlen(fromName) >= strlen(toName)) {
		nameBuffer = strdup(toName);
		if (nameBuffer == NULL) {
			status = B_NO_MEMORY;
			goto err;
		}
	}

	targetVnode = rootfs_find_in_dir(toDirectory, toName);
	if (targetVnode != NULL) {
		// target node exists, let's see if it is an empty directory
		if (S_ISDIR(targetVnode->stream.type) && !rootfs_is_dir_empty(targetVnode)) {
			status = B_NAME_IN_USE;
			goto err;
		}

		// so we can cleanly remove it
		remove_node(fs, toDirectory, targetVnode);
	}

	// change the name on this node
	if (nameBuffer == NULL) {
		// we can just copy it
		strcpy(vnode->name, toName);
	} else {
		free(vnode->name);
		vnode->name = nameBuffer;
	}

	// remove it from the dir
	rootfs_remove_from_dir(fs, fromDirectory, vnode);

	// Add it back to the dir with the new name.
	// We need to do this even in the same directory, 
	// so that it keeps sorted correctly.
	rootfs_insert_in_dir(fs, toDirectory, vnode);

	notify_entry_moved(fs->id, fromDirectory->id, fromName, toDirectory->id, toName, vnode->id);
	status = B_OK;

err:
	if (status != B_OK)
		free(nameBuffer);

	mutex_unlock(&fs->lock);

	return status;
}


static status_t
rootfs_read_stat(fs_volume _fs, fs_vnode _v, struct stat *stat)
{
	struct rootfs *fs = _fs;
	struct rootfs_vnode *vnode = _v;

	TRACE(("rootfs_read_stat: vnode %p (0x%Lx), stat %p\n", vnode, vnode->id, stat));

	// stream exists, but we know to return size 0, since we can only hold directories
	stat->st_dev = fs->id;
	stat->st_ino = vnode->id;
	stat->st_size = 0;
	stat->st_mode = vnode->stream.type;

	stat->st_nlink = 1;
	stat->st_blksize = 65536;

	stat->st_uid = vnode->uid;
	stat->st_gid = vnode->gid;

	stat->st_atime = time(NULL);
	stat->st_mtime = stat->st_ctime = vnode->modification_time;
	stat->st_crtime = vnode->creation_time;

	return B_OK;
}


static status_t
rootfs_write_stat(fs_volume _fs, fs_vnode _vnode, const struct stat *stat, uint32 statMask)
{
	struct rootfs *fs = _fs;
	struct rootfs_vnode *vnode = _vnode;

	TRACE(("rootfs_write_stat: vnode %p (0x%Lx), stat %p\n", vnode, vnode->id, stat));

	// we cannot change the size of anything
	if (statMask & B_STAT_SIZE)
		return B_BAD_VALUE;

	mutex_lock(&fs->lock);

	if (statMask & B_STAT_MODE)
		vnode->stream.type = (vnode->stream.type & ~S_IUMSK) | (stat->st_mode & S_IUMSK);

	if (statMask & B_STAT_UID)
		vnode->uid = stat->st_uid;
	if (statMask & B_STAT_GID)
		vnode->gid = stat->st_gid;

	if (statMask & B_STAT_MODIFICATION_TIME)
		vnode->modification_time = stat->st_mtime;
	if (statMask & B_STAT_CREATION_TIME) 
		vnode->creation_time = stat->st_crtime;

	mutex_unlock(&fs->lock);

	notify_stat_changed(fs->id, vnode->id, statMask);
	return B_OK;
}


static status_t
rootfs_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return B_OK;

		case B_MODULE_UNINIT:
			return B_OK;

		default:
			return B_ERROR;
	}
}


file_system_module_info gRootFileSystem = {
	{
		"file_systems/rootfs" B_CURRENT_FS_API_VERSION,
		0,
		rootfs_std_ops,
	},

	"Root File System",

	NULL,	// identify_partition()
	NULL,	// scan_partition()
	NULL,	// free_identify_partition_cookie()
	NULL,	// free_partition_content_cookie()

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

	NULL,	// get_file_map()

	/* common */
	&rootfs_ioctl,
	NULL,	// fs_set_flags()
	NULL,	// select
	NULL,	// deselect
	&rootfs_fsync,

	&rootfs_read_link,
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
	&rootfs_close,			// same as for files - it does nothing, anyway
	&rootfs_free_dir_cookie,
	&rootfs_read_dir,
	&rootfs_rewind_dir,

	// the other operations are not supported (attributes, indices, queries)
	NULL,
};

