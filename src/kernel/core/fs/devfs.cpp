/*
** Copyright 2002-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
**
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include "IOScheduler.h"

#include <KernelExport.h>
#include <Drivers.h>
#include <pnp_devfs.h>
#include <NodeMonitor.h>

#include <kdevice_manager.h>
#include <KPath.h>
#include <vfs.h>
#include <debug.h>
#include <khash.h>
#include <malloc.h>
#include <lock.h>
#include <vm.h>

#include <arch/cpu.h>
#include <devfs.h>

#include <string.h>
#include <stdio.h>
#include <sys/stat.h>


//#define TRACE_DEVFS
#ifdef TRACE_DEVFS
#	define TRACE(x) dprintf x
#else
#	define TRACE(x)
#endif


struct devfs_part_map {
	struct devfs_vnode *raw_device;
	partition_info info;
};

struct devfs_stream {
	mode_t type;
	union {
		struct stream_dir {
			struct devfs_vnode *dir_head;
			struct devfs_cookie *jar_head;
		} dir;
		struct stream_dev {
			pnp_node_info *node;
			pnp_devfs_driver_info *info;
			device_hooks *ops;
			struct devfs_part_map *part_map;
			IOScheduler *scheduler;
		} dev;
		struct stream_symlink {
			const char *path;
			size_t length;
		} symlink;
	} u;
};

struct devfs_vnode {
	struct devfs_vnode *all_next;
	vnode_id	id;
	char		*name;
	time_t		modification_time;
	time_t		creation_time;
	uid_t		uid;
	gid_t		gid;
	struct devfs_vnode *parent;
	struct devfs_vnode *dir_next;
	struct devfs_stream stream;
};

struct devfs {
	mount_id id;
	mutex lock;
	int next_vnode_id;
	hash_table *vnode_list_hash;
	struct devfs_vnode *root_vnode;
};

struct devfs_cookie {
	int oflags;
	union {
		struct cookie_dir {
			struct devfs_cookie *next;
			struct devfs_cookie *prev;
			struct devfs_vnode *ptr;
			int state;	// iteration state
		} dir;
		struct cookie_dev {
			void *dcookie;
		} dev;
	} u;
};

// directory iteration states
enum {
	ITERATION_STATE_DOT		= 0,
	ITERATION_STATE_DOT_DOT	= 1,
	ITERATION_STATE_OTHERS	= 2,
	ITERATION_STATE_BEGIN	= ITERATION_STATE_DOT,
};

/* the one and only allowed devfs instance */
static struct devfs *sDeviceFileSystem = NULL;


#define BOOTFS_HASH_SIZE 16


static uint32
devfs_vnode_hash_func(void *_vnode, const void *_key, uint32 range)
{
	struct devfs_vnode *vnode = (struct devfs_vnode *)_vnode;
	const vnode_id *key = (const vnode_id *)_key;

	if (vnode != NULL)
		return vnode->id % range;

	return (*key) % range;
}


static int
devfs_vnode_compare_func(void *_vnode, const void *_key)
{
	struct devfs_vnode *vnode = (struct devfs_vnode *)_vnode;
	const vnode_id *key = (const vnode_id *)_key;

	if (vnode->id == *key)
		return 0;

	return -1;
}


static struct devfs_vnode *
devfs_create_vnode(struct devfs *fs, devfs_vnode *parent, const char *name)
{
	struct devfs_vnode *vnode;

	vnode = (struct devfs_vnode *)malloc(sizeof(struct devfs_vnode));
	if (vnode == NULL)
		return NULL;

	memset(vnode, 0, sizeof(struct devfs_vnode));
	vnode->id = fs->next_vnode_id++;

	vnode->name = strdup(name);
	if (vnode->name == NULL) {
		free(vnode);
		return NULL;
	}

	vnode->creation_time = vnode->modification_time = time(NULL);
	vnode->uid = geteuid();
	vnode->gid = parent ? parent->gid : getegid();
		// inherit group from parent if possible

	return vnode;
}


static status_t
devfs_delete_vnode(struct devfs *fs, struct devfs_vnode *vnode, bool force_delete)
{
	// cant delete it if it's in a directory or is a directory
	// and has children
	if (!force_delete
		&& ((S_ISDIR(vnode->stream.type) && vnode->stream.u.dir.dir_head != NULL)
			|| vnode->dir_next != NULL))
		return EPERM;

	// remove it from the global hash table
	hash_remove(fs->vnode_list_hash, vnode);

	if (S_ISCHR(vnode->stream.type)) {
		// for partitions, we have to release the raw device
		if (vnode->stream.u.dev.part_map)
			put_vnode(fs->id, vnode->stream.u.dev.part_map->raw_device->id);

		// remove API conversion from old to new drivers
		if (vnode->stream.u.dev.node == NULL)
			free(vnode->stream.u.dev.info);
	}

	free(vnode->name);
	free(vnode);

	return B_OK;
}


#if 0
static void
insert_cookie_in_jar(struct devfs_vnode *dir, struct devfs_cookie *cookie)
{
	cookie->u.dir.next = dir->stream.u.dir.jar_head;
	dir->stream.u.dir.jar_head = cookie;
	cookie->u.dir.prev = NULL;
}


static void
remove_cookie_from_jar(struct devfs_vnode *dir, struct devfs_cookie *cookie)
{
	if (cookie->u.dir.next)
		cookie->u.dir.next->u.dir.prev = cookie->u.dir.prev;
	if (cookie->u.dir.prev)
		cookie->u.dir.prev->u.dir.next = cookie->u.dir.next;
	if (dir->stream.u.dir.jar_head == cookie)
		dir->stream.u.dir.jar_head = cookie->u.dir.next;

	cookie->u.dir.prev = cookie->u.dir.next = NULL;
}
#endif


/* makes sure none of the dircookies point to the vnode passed in */
static void
update_dircookies(struct devfs_vnode *dir, struct devfs_vnode *v)
{
	struct devfs_cookie *cookie;

	for (cookie = dir->stream.u.dir.jar_head; cookie; cookie = cookie->u.dir.next) {
		if (cookie->u.dir.ptr == v)
			cookie->u.dir.ptr = v->dir_next;
	}
}


static struct devfs_vnode *
devfs_find_in_dir(struct devfs_vnode *dir, const char *path)
{
	struct devfs_vnode *v;

	if (!S_ISDIR(dir->stream.type))
		return NULL;

	if (!strcmp(path, "."))
		return dir;
	if (!strcmp(path, ".."))
		return dir->parent;

	for (v = dir->stream.u.dir.dir_head; v; v = v->dir_next) {
		TRACE(("devfs_find_in_dir: looking at entry '%s'\n", v->name));
		if (strcmp(v->name, path) == 0) {
			TRACE(("devfs_find_in_dir: found it at %p\n", v));
			return v;
		}
	}
	return NULL;
}


static status_t
devfs_insert_in_dir(struct devfs_vnode *dir, struct devfs_vnode *v)
{
	if (!S_ISDIR(dir->stream.type))
		return B_BAD_VALUE;

	v->dir_next = dir->stream.u.dir.dir_head;
	dir->stream.u.dir.dir_head = v;

	v->parent = dir;
	dir->modification_time = time(NULL);
	return B_OK;
}


static status_t
devfs_remove_from_dir(struct devfs_vnode *dir, struct devfs_vnode *findit)
{
	struct devfs_vnode *v;
	struct devfs_vnode *last_v;

	for (v = dir->stream.u.dir.dir_head, last_v = NULL; v; last_v = v, v = v->dir_next) {
		if (v == findit) {
			/* make sure all dircookies dont point to this vnode */
			update_dircookies(dir, v);

			if (last_v)
				last_v->dir_next = v->dir_next;
			else
				dir->stream.u.dir.dir_head = v->dir_next;
			v->dir_next = NULL;
			dir->modification_time = time(NULL);
			return B_OK;
		}
	}
	return B_ENTRY_NOT_FOUND;
}


/* XXX seems to be unused
static bool
devfs_is_dir_empty(struct devfs_vnode *dir)
{
	if (dir->stream.type != STREAM_TYPE_DIR)
		return false;

	return !dir->stream.u.dir.dir_head;
}
*/


static status_t
devfs_get_partition_info(struct devfs *fs, struct devfs_vnode *v, 
	struct devfs_cookie *cookie, void *buffer, size_t length)
{
	struct devfs_part_map *map = v->stream.u.dev.part_map;

	if (!S_ISCHR(v->stream.type) || map == NULL || length != sizeof(partition_info))
		return B_BAD_VALUE;

	return user_memcpy(buffer, &map->info, sizeof(partition_info));
}


static status_t
add_partition(struct devfs *fs, struct devfs_vnode *device,
	const char *name, const partition_info &info)
{
	struct devfs_vnode *partition;
	status_t status;

	if (!S_ISCHR(device->stream.type))
		return B_BAD_VALUE;

	// we don't support nested partitions
	if (device->stream.u.dev.part_map)
		return B_BAD_VALUE;

	// reduce checks to a minimum - things like negative offsets could be useful
	if (info.size < 0)
		return B_BAD_VALUE;

	// create partition map
	struct devfs_part_map *map = (struct devfs_part_map *)malloc(sizeof(struct devfs_part_map));
	if (map == NULL)
		return B_NO_MEMORY;

	memcpy(&map->info, &info, sizeof(partition_info));

	mutex_lock(&fs->lock);

	// you cannot change a partition once set
	if (devfs_find_in_dir(device->parent, name)) {
		status = B_BAD_VALUE;
		goto err1;
	}

	// increase reference count of raw device - 
	// the partition device really needs it 
	// (at least to resolve its name on GET_PARTITION_INFO)
	status = get_vnode(fs->id, device->id, (fs_vnode *)&map->raw_device);
	if (status < B_OK)
		goto err1;

	// now create the partition vnode
	partition = devfs_create_vnode(fs, device->parent, name);
	if (partition == NULL) {
		status = B_NO_MEMORY;
		goto err2;
	}

	partition->stream.type = device->stream.type;
	partition->stream.u.dev.node = device->stream.u.dev.node;
	partition->stream.u.dev.info = device->stream.u.dev.info;
	partition->stream.u.dev.ops = device->stream.u.dev.ops;
	partition->stream.u.dev.part_map = map;
	partition->stream.u.dev.scheduler = device->stream.u.dev.scheduler;

	hash_insert(fs->vnode_list_hash, partition);

	devfs_insert_in_dir(device->parent, partition);

	mutex_unlock(&fs->lock);

	TRACE(("SET_PARTITION: Added partition\n"));
	return B_OK;

err1:
	mutex_unlock(&fs->lock);

	free(map);
	return status;

err2:
	mutex_unlock(&fs->lock);

	put_vnode(fs->id, device->id);
	free(map);
	return status;
}


static inline void
translate_partition_access(devfs_part_map *map, off_t &offset, size_t &size)
{
	if (offset < 0)
		offset = 0;

	if (offset > map->info.size) {
		size = 0;
		return;
	}

	size = min_c(size, map->info.size - offset);
	offset += map->info.offset;
}


static pnp_devfs_driver_info *
create_new_driver_info(device_hooks *ops)
{
	pnp_devfs_driver_info *info = (pnp_devfs_driver_info *)malloc(sizeof(pnp_devfs_driver_info));
	if (info == NULL)
		return NULL;

	memset(info, 0, sizeof(pnp_driver_info));

	info->open = NULL;
		// ops->open is used directly for old devices
	info->close = ops->close;
	info->free = ops->free;
	info->control = ops->control;
	info->select = ops->select;
	info->deselect = ops->deselect;
	info->read = ops->read;
	info->write = ops->write;

	info->read_pages = NULL;
	info->write_pages = NULL;
		// old devices can't know to do physical page access

	return info;
}


static status_t
get_node_for_path(struct devfs *fs, const char *path, struct devfs_vnode **_node)
{
	return vfs_get_fs_node_from_path(fs->id, path, true, (void **)_node);
}


static status_t
unpublish_node(struct devfs *fs, const char *path, int type)
{
	devfs_vnode *node;
	status_t status = get_node_for_path(fs, path, &node);
	if (status != B_OK)
		return status;

	if ((type & S_IFMT) != type) {
		status = B_BAD_TYPE;
		goto err1;
	}

	mutex_lock(&fs->lock);

	status = devfs_remove_from_dir(node->parent, node);
	if (status < B_OK)
		goto err2;

	status = remove_vnode(fs->id, node->id);

err2:
	mutex_unlock(&fs->lock);
err1:
	put_vnode(fs->id, node->id);
	return status;
}


static status_t
publish_node(struct devfs *fs, const char *path, struct devfs_vnode **_node)
{
	ASSERT_LOCKED_MUTEX(&fs->lock);

	// copy the path over to a temp buffer so we can munge it
	KPath tempPath(path);
	char *temp = tempPath.LockBuffer();

	// create the path leading to the device
	// parse the path passed in, stripping out '/'

	struct devfs_vnode *dir = fs->root_vnode;
	struct devfs_vnode *vnode = NULL;
	status_t status = B_OK;
	int32 i = 0, last = 0;
	bool atLeaf = false;

	for (;;) {
		if (temp[i] == 0) {
			atLeaf = true; // we'll be done after this one
		} else if (temp[i] == '/') {
			temp[i] = 0;
			i++;
		} else {
			i++;
			continue;
		}

		TRACE(("\tpath component '%s'\n", &temp[last]));

		// we have a path component
		vnode = devfs_find_in_dir(dir, &temp[last]);
		if (vnode) {
			if (!atLeaf) {
				// we are not at the leaf of the path, so as long as
				// this is a dir we're okay
				if (S_ISDIR(vnode->stream.type)) {
					last = i;
					dir = vnode;
					continue;
				}
			}
			// we are at the leaf and hit another node
			// or we aren't but hit a non-dir node.
			// we're screwed
			status = B_FILE_EXISTS;
			goto out;
		} else {
			vnode = devfs_create_vnode(fs, dir, &temp[last]);
			if (!vnode) {
				status = B_NO_MEMORY;
				goto out;
			}
		}

		// set up the new vnode
		if (!atLeaf) {
			// this is a dir
			vnode->stream.type = S_IFDIR | 0755;
			vnode->stream.u.dir.dir_head = NULL;
			vnode->stream.u.dir.jar_head = NULL;
		} else {
			// this is the last component
			*_node = vnode;
		}

		hash_insert(sDeviceFileSystem->vnode_list_hash, vnode);
		devfs_insert_in_dir(dir, vnode);

		if (atLeaf)
			break;

		last = i;
		dir = vnode;
	}

out:
	return status;
}


static status_t
publish_device(struct devfs *fs, const char *path, pnp_node_info *pnpNode,
	pnp_devfs_driver_info *info, device_hooks *ops)
{
	TRACE(("publish_device(path = \"%s\", node = %p, info = %p, hooks = %p)\n",
		path, pnpNode, info, ops));

	if (sDeviceFileSystem == NULL) {
		panic("publish_device() called before devfs mounted\n");
		return B_ERROR;
	}

	if ((ops == NULL && (pnpNode == NULL || info == NULL))
		|| path == NULL || path[0] == '/')
		return B_BAD_VALUE;

	// are the provided device hooks okay?
	if ((ops != NULL && (ops->open == NULL || ops->close == NULL
		|| ops->read == NULL || ops->write == NULL))
		|| info != NULL && (info->open == NULL || info->close == NULL
		|| info->read == NULL || info->write == NULL))
		return B_BAD_VALUE;

	// mark disk devices - they might get an I/O scheduler
	bool isDisk = false;
	if (!strncmp(path, "disk/", 5))
		isDisk = true;

	struct devfs_vnode *node;
	status_t status;

	mutex_lock(&fs->lock);

	status = publish_node(fs, path, &node);
	if (status != B_OK)
		goto out;

	// all went fine, let's initialize the node
	node->stream.type = S_IFCHR | 0644;

	if (pnpNode != NULL)
		node->stream.u.dev.info = info;
	else
		node->stream.u.dev.info = create_new_driver_info(ops);

	node->stream.u.dev.node = pnpNode;
	node->stream.u.dev.ops = ops;

	// every raw disk gets an I/O scheduler object attached
	// ToDo: the driver should ask for a scheduler (ie. using its devfs node attributes)
	if (isDisk && !strcmp(node->name, "raw")) 
		node->stream.u.dev.scheduler = new IOScheduler(path, info);

out:
	mutex_unlock(&fs->lock);
	return status;
}


/** Construct complete device name (as used for device_open()).
 *	This is safe to use only when the device is in use (and therefore
 *	cannot be unpublished during the iteration).
 */

static void
get_device_name(struct devfs_vnode *vnode, char *buffer, size_t size)
{
	struct devfs_vnode *leaf = vnode;
	size_t offset = 0;

	// count levels

	for (; vnode->parent && vnode->parent != vnode; vnode = vnode->parent) {
		offset += strlen(vnode->name) + 1;
	}

	// construct full path name
	
	for (vnode = leaf; vnode->parent && vnode->parent != vnode; vnode = vnode->parent) {
		size_t length = strlen(vnode->name);
		size_t start = offset - length - 1;

		if (size >= offset) {
			strcpy(buffer + start, vnode->name);
			if (vnode != leaf)
				buffer[offset - 1] = '/';
		}

		offset = start;	
	}
}


//	#pragma mark -


static status_t
devfs_mount(mount_id id, const char *devfs, void *args, fs_volume *_fs, vnode_id *root_vnid)
{
	struct devfs *fs;
	struct devfs_vnode *v;
	status_t err;

	TRACE(("devfs_mount: entry\n"));

	if (sDeviceFileSystem) {
		dprintf("double mount of devfs attempted\n");
		err = B_ERROR;
		goto err;
	}

	fs = (struct devfs *)malloc(sizeof(struct devfs));
	if (fs == NULL) {
		err = B_NO_MEMORY;
		goto err;
	}

	fs->id = id;
	fs->next_vnode_id = 0;

	err = mutex_init(&fs->lock, "devfs_mutex");
	if (err < B_OK)
		goto err1;

	fs->vnode_list_hash = hash_init(BOOTFS_HASH_SIZE, (addr_t)&v->all_next - (addr_t)v,
		&devfs_vnode_compare_func, &devfs_vnode_hash_func);
	if (fs->vnode_list_hash == NULL) {
		err = B_NO_MEMORY;
		goto err2;
	}

	// create a vnode
	v = devfs_create_vnode(fs, NULL, "");
	if (v == NULL) {
		err = B_NO_MEMORY;
		goto err3;
	}

	// set it up
	v->parent = v;

	// create a dir stream for it to hold
	v->stream.type = S_IFDIR | 0755;
	v->stream.u.dir.dir_head = NULL;
	v->stream.u.dir.jar_head = NULL;
	fs->root_vnode = v;

	hash_insert(fs->vnode_list_hash, v);

	*root_vnid = v->id;
	*_fs = fs;
	sDeviceFileSystem = fs;
	return B_OK;

	devfs_delete_vnode(fs, v, true);
err3:
	hash_uninit(fs->vnode_list_hash);
err2:
	mutex_destroy(&fs->lock);
err1:
	free(fs);
err:
	return err;
}


static status_t
devfs_unmount(fs_volume _fs)
{
	struct devfs *fs = (struct devfs *)_fs;
	struct devfs_vnode *v;
	struct hash_iterator i;

	TRACE(("devfs_unmount: entry fs = %p\n", fs));

	// delete all of the vnodes
	hash_open(fs->vnode_list_hash, &i);
	while((v = (struct devfs_vnode *)hash_next(fs->vnode_list_hash, &i)) != NULL) {
		devfs_delete_vnode(fs, v, true);
	}
	hash_close(fs->vnode_list_hash, &i, false);

	hash_uninit(fs->vnode_list_hash);
	mutex_destroy(&fs->lock);
	free(fs);

	return 0;
}


static status_t
devfs_sync(fs_volume fs)
{
	TRACE(("devfs_sync: entry\n"));

	return 0;
}


static status_t
devfs_lookup(fs_volume _fs, fs_vnode _dir, const char *name, vnode_id *_id, int *_type)
{
	struct devfs *fs = (struct devfs *)_fs;
	struct devfs_vnode *dir = (struct devfs_vnode *)_dir;
	struct devfs_vnode *vnode, *vdummy;
	status_t err;

	TRACE(("devfs_lookup: entry dir %p, name '%s'\n", dir, name));

	if (!S_ISDIR(dir->stream.type))
		return B_NOT_A_DIRECTORY;

	mutex_lock(&fs->lock);

	// look it up
	vnode = devfs_find_in_dir(dir, name);
	if (!vnode) {
		err = B_ENTRY_NOT_FOUND;
		goto err;
	}

	err = get_vnode(fs->id, vnode->id, (fs_vnode *)&vdummy);
	if (err < 0)
		goto err;

	*_id = vnode->id;
	*_type = vnode->stream.type;

err:
	mutex_unlock(&fs->lock);

	return err;
}


static status_t
devfs_get_vnode_name(fs_volume _fs, fs_vnode _vnode, char *buffer, size_t bufferSize)
{
	struct devfs_vnode *vnode = (struct devfs_vnode *)_vnode;

	TRACE(("devfs_get_vnode_name: vnode = %p\n", vnode));
	
	strlcpy(buffer, vnode->name, bufferSize);
	return B_OK;
}


static status_t
devfs_get_vnode(fs_volume _fs, vnode_id id, fs_vnode *_vnode, bool reenter)
{
	struct devfs *fs = (struct devfs *)_fs;

	TRACE(("devfs_get_vnode: asking for vnode id = %Ld, vnode = %p, r %d\n", id, _vnode, reenter));

	if (!reenter)
		mutex_lock(&fs->lock);

	*_vnode = hash_lookup(fs->vnode_list_hash, &id);

	if (!reenter)
		mutex_unlock(&fs->lock);

	TRACE(("devfs_get_vnode: looked it up at %p\n", *_vnode));

	if (*_vnode)
		return 0;

	return B_ENTRY_NOT_FOUND;
}


static status_t
devfs_put_vnode(fs_volume _fs, fs_vnode _v, bool reenter)
{
#ifdef TRACE_DEVFS
	struct devfs_vnode *vnode = (struct devfs_vnode *)_v;

	TRACE(("devfs_put_vnode: entry on vnode %p, id = %Ld, reenter %d\n", vnode, vnode->id, reenter));
#endif

	return 0; // whatever
}


static status_t
devfs_remove_vnode(fs_volume _fs, fs_vnode _v, bool reenter)
{
	struct devfs *fs = (struct devfs *)_fs;
	struct devfs_vnode *vnode = (struct devfs_vnode *)_v;

	TRACE(("devfs_removevnode: remove %p (%Ld), reenter %d\n", vnode, vnode->id, reenter));

	if (!reenter)
		mutex_lock(&fs->lock);

	if (vnode->dir_next) {
		// can't remove node if it's linked to the dir
		panic("devfs_removevnode: vnode %p asked to be removed is present in dir\n", vnode);
	}

	devfs_delete_vnode(fs, vnode, false);

	if (!reenter)
		mutex_unlock(&fs->lock);

	return B_OK;
}


static status_t
devfs_create(fs_volume _fs, fs_vnode _dir, const char *name, int openMode, int perms,
	fs_cookie *_cookie, vnode_id *_newVnodeID)
{
	struct devfs_vnode *dir = (struct devfs_vnode *)_dir;
	struct devfs *fs = (struct devfs *)_fs;
	struct devfs_cookie *cookie;
	struct devfs_vnode *vnode, *vdummy;
	status_t status = B_OK;

	TRACE(("devfs_create: vnode %p, oflags 0x%x, fs_cookie %p \n", vnode, openMode, _cookie));

	mutex_lock(&fs->lock);

	// look it up
	vnode = devfs_find_in_dir(dir, name);
	if (!vnode) {
		status = EROFS;
		goto err1;
	}

	if (openMode & O_EXCL)
		return B_FILE_EXISTS;

	status = get_vnode(fs->id, vnode->id, (fs_vnode *)&vdummy);
	if (status < B_OK)
		goto err1;

	*_newVnodeID = vnode->id;

	cookie = (struct devfs_cookie *)malloc(sizeof(struct devfs_cookie));
	if (cookie == NULL) {
		status = B_NO_MEMORY;
		goto err2;
	}

	if (S_ISCHR(vnode->stream.type)) {
		if (vnode->stream.u.dev.node != NULL) {
			status = vnode->stream.u.dev.info->open(
				vnode->stream.u.dev.node->parent->cookie, openMode,
				&cookie->u.dev.dcookie);
		} else {
			char buffer[B_FILE_NAME_LENGTH];
			get_device_name(vnode, buffer, sizeof(buffer));

			status = vnode->stream.u.dev.ops->open(buffer, openMode,
				&cookie->u.dev.dcookie);
		}
	}
	if (status < B_OK)
		goto err3;

	*_cookie = cookie;
	mutex_unlock(&fs->lock);
	return B_OK;

err3:
	free(cookie);
err2:
	put_vnode(fs->id, vnode->id);
err1:
	mutex_unlock(&fs->lock);
	return status;
}


static status_t
devfs_open(fs_volume _fs, fs_vnode _vnode, int openMode, fs_cookie *_cookie)
{
	struct devfs_vnode *vnode = (struct devfs_vnode *)_vnode;
	struct devfs_cookie *cookie;
	status_t status = B_OK;

	TRACE(("devfs_open: vnode %p, oflags 0x%x, fs_cookie %p \n", vnode, openMode, _cookie));

	cookie = (struct devfs_cookie *)malloc(sizeof(struct devfs_cookie));
	if (cookie == NULL)
		return B_NO_MEMORY;

	if (S_ISCHR(vnode->stream.type)) {
		if (vnode->stream.u.dev.node != NULL) {
			status = vnode->stream.u.dev.info->open(
				vnode->stream.u.dev.node->parent->cookie, openMode,
				&cookie->u.dev.dcookie);
		} else {
			char buffer[B_FILE_NAME_LENGTH];
			get_device_name(vnode, buffer, sizeof(buffer));

			status = vnode->stream.u.dev.ops->open(buffer, openMode,
				&cookie->u.dev.dcookie);
		}
	}
	if (status < B_OK)
		free(cookie);
	else
		*_cookie = cookie;

	return status;
}


static status_t
devfs_close(fs_volume _fs, fs_vnode _vnode, fs_cookie _cookie)
{
	struct devfs_vnode *vnode = (struct devfs_vnode *)_vnode;
	struct devfs_cookie *cookie = (struct devfs_cookie *)_cookie;

	TRACE(("devfs_close: entry vnode %p, cookie %p\n", vnode, cookie));

	if (S_ISCHR(vnode->stream.type)) {
		// pass the call through to the underlying device
		return vnode->stream.u.dev.info->close(cookie->u.dev.dcookie);
	}

	return B_OK;
}


static status_t
devfs_free_cookie(fs_volume _fs, fs_vnode _vnode, fs_cookie _cookie)
{
	struct devfs_vnode *vnode = (struct devfs_vnode *)_vnode;
	struct devfs_cookie *cookie = (struct devfs_cookie *)_cookie;

	TRACE(("devfs_freecookie: entry vnode %p, cookie %p\n", vnode, cookie));

	if (S_ISCHR(vnode->stream.type)) {
		// pass the call through to the underlying device
		vnode->stream.u.dev.info->free(cookie->u.dev.dcookie);
	}

	free(cookie);
	return 0;
}


static status_t
devfs_fsync(fs_volume _fs, fs_vnode _v)
{
	return 0;
}


static status_t
devfs_read_link(fs_volume _fs, fs_vnode _link, char *buffer, size_t bufferSize)
{
	struct devfs_vnode *link = (struct devfs_vnode *)_link;

	if (!S_ISLNK(link->stream.type))
		return B_BAD_VALUE;

	if (bufferSize <= link->stream.u.symlink.length)
		return B_NAME_TOO_LONG;

	memcpy(buffer, link->stream.u.symlink.path, link->stream.u.symlink.length + 1);
	return B_OK;
}


static status_t
devfs_read(fs_volume _fs, fs_vnode _vnode, fs_cookie _cookie, off_t pos,
	void *buffer, size_t *_length)
{
	struct devfs_vnode *vnode = (struct devfs_vnode *)_vnode;
	struct devfs_cookie *cookie = (struct devfs_cookie *)_cookie;

	TRACE(("devfs_read: vnode %p, cookie %p, pos %Ld, len %p\n", vnode, cookie, pos, _length));

	if (!S_ISCHR(vnode->stream.type))
		return B_BAD_VALUE;

	if (vnode->stream.u.dev.part_map)
		translate_partition_access(vnode->stream.u.dev.part_map, pos, *_length);

	if (*_length == 0)
		return B_OK;

	// if this device has an I/O scheduler attached, the request must go through it
	if (IOScheduler *scheduler = vnode->stream.u.dev.scheduler) {
		IORequest request(cookie->u.dev.dcookie, pos, buffer, *_length);

		status_t status = scheduler->Process(request);
		if (status == B_OK)
			*_length = request.Size();

		return status;
	}

	// pass the call through to the device
	return vnode->stream.u.dev.info->read(cookie->u.dev.dcookie, pos, buffer, _length);
}


static status_t
devfs_write(fs_volume _fs, fs_vnode _vnode, fs_cookie _cookie, off_t pos,
	const void *buffer, size_t *_length)
{
	struct devfs_vnode *vnode = (struct devfs_vnode *)_vnode;
	struct devfs_cookie *cookie = (struct devfs_cookie *)_cookie;

	TRACE(("devfs_write: vnode %p, cookie %p, pos %Ld, len %p\n", vnode, cookie, pos, _length));

	if (!S_ISCHR(vnode->stream.type))
		return B_BAD_VALUE;

	if (vnode->stream.u.dev.part_map)
		translate_partition_access(vnode->stream.u.dev.part_map, pos, *_length);

	if (*_length == 0)
		return B_OK;

	if (IOScheduler *scheduler = vnode->stream.u.dev.scheduler) {
		IORequest request(cookie->u.dev.dcookie, pos, buffer, *_length);

		status_t status = scheduler->Process(request);
		if (status == B_OK)
			*_length = request.Size();

		return status;
	}

	return vnode->stream.u.dev.info->write(cookie->u.dev.dcookie, pos, buffer, _length);
}


static status_t
devfs_create_dir(fs_volume _fs, fs_vnode _dir, const char *name, int perms, vnode_id *new_vnid)
{
	return EROFS;
}


static status_t
devfs_open_dir(fs_volume _fs, fs_vnode _vnode, fs_cookie *_cookie)
{
	struct devfs *fs = (struct devfs *)_fs;
	struct devfs_vnode *vnode = (struct devfs_vnode *)_vnode;
	struct devfs_cookie *cookie;

	TRACE(("devfs_open_dir: vnode %p\n", vnode));

	if (!S_ISDIR(vnode->stream.type))
		return B_BAD_VALUE;

	cookie = (struct devfs_cookie *)malloc(sizeof(struct devfs_cookie));
	if (cookie == NULL)
		return B_NO_MEMORY;

	mutex_lock(&fs->lock);

	cookie->u.dir.ptr = vnode->stream.u.dir.dir_head;
	cookie->u.dir.state = ITERATION_STATE_BEGIN;
	*_cookie = cookie;

	mutex_unlock(&fs->lock);
	return B_OK;
}


static status_t
devfs_read_dir(fs_volume _fs, fs_vnode _vnode, fs_cookie _cookie, struct dirent *dirent, size_t bufferSize, uint32 *_num)
{
	struct devfs_vnode *vnode = (struct devfs_vnode *)_vnode;
	struct devfs_cookie *cookie = (struct devfs_cookie *)_cookie;
	struct devfs *fs = (struct devfs *)_fs;
	status_t status = B_OK;
	struct devfs_vnode *childNode = NULL;
	const char *name = NULL;
	struct devfs_vnode *nextChildNode = NULL;
	int nextState = cookie->u.dir.state;

	TRACE(("devfs_read_dir: vnode %p, cookie %p, buffer %p, size %ld\n", _vnode, cookie, dirent, bufferSize));

	if (!S_ISDIR(vnode->stream.type))
		return B_BAD_VALUE;

	mutex_lock(&fs->lock);

	switch (cookie->u.dir.state) {
		case ITERATION_STATE_DOT:
			childNode = vnode;
			name = ".";
			nextChildNode = vnode->stream.u.dir.dir_head;
			nextState = cookie->u.dir.state + 1;
			break;
		case ITERATION_STATE_DOT_DOT:
			childNode = vnode->parent;
			name = "..";
			nextChildNode = vnode->stream.u.dir.dir_head;
			nextState = cookie->u.dir.state + 1;
			break;
		default:
			childNode = cookie->u.dir.ptr;
			if (childNode) {
				name = childNode->name;
				nextChildNode = childNode->dir_next;
			}
			break;
	}

	if (!childNode) {
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

	cookie->u.dir.ptr = nextChildNode;
	cookie->u.dir.state = nextState;
	status = B_OK;

err:
	mutex_unlock(&fs->lock);

	return status;
}


static status_t
devfs_rewind_dir(fs_volume _fs, fs_vnode _vnode, fs_cookie _cookie)
{
	struct devfs_vnode *vnode = (struct devfs_vnode *)_vnode;
	struct devfs_cookie *cookie = (struct devfs_cookie *)_cookie;
	struct devfs *fs = (struct devfs *)_fs;

	TRACE(("devfs_rewind_dir: vnode %p, cookie %p\n", _vnode, _cookie));

	if (!S_ISDIR(vnode->stream.type))
		return B_BAD_VALUE;

	mutex_lock(&fs->lock);

	cookie->u.dir.ptr = vnode->stream.u.dir.dir_head;
	cookie->u.dir.state = ITERATION_STATE_BEGIN;

	mutex_unlock(&fs->lock);
	return B_OK;
}


static status_t
devfs_ioctl(fs_volume _fs, fs_vnode _vnode, fs_cookie _cookie, ulong op, void *buffer, size_t length)
{
	struct devfs *fs = (struct devfs *)_fs;
	struct devfs_vnode *vnode = (struct devfs_vnode *)_vnode;
	struct devfs_cookie *cookie = (struct devfs_cookie *)_cookie;

	TRACE(("devfs_ioctl: vnode %p, cookie %p, op %ld, buf %p, len %ld\n", _vnode, _cookie, op, buffer, length));

	if (S_ISCHR(vnode->stream.type)) {
		switch (op) {
			case B_GET_PARTITION_INFO:
				return devfs_get_partition_info(fs, vnode, cookie, (partition_info *)buffer, length);

			case B_SET_PARTITION:
				return B_NOT_ALLOWED;
		}

		return vnode->stream.u.dev.info->control(cookie->u.dev.dcookie, op, buffer, length);
	}

	return B_BAD_VALUE;
}


static status_t
devfs_set_flags(fs_volume _fs, fs_vnode _vnode, fs_cookie _cookie, int flags)
{
	struct devfs_vnode *vnode = (struct devfs_vnode *)_vnode;
	struct devfs_cookie *cookie = (struct devfs_cookie *)_cookie;

	// we need to pass the O_NONBLOCK flag to the underlying device

	if (!S_ISCHR(vnode->stream.type))
		return B_NOT_ALLOWED;

	return vnode->stream.u.dev.info->control(cookie->u.dev.dcookie, flags & O_NONBLOCK ?
		B_SET_NONBLOCKING_IO : B_SET_BLOCKING_IO, NULL, 0);
}


static bool
devfs_can_page(fs_volume _fs, fs_vnode _vnode, fs_cookie cookie)
{
	struct devfs_vnode *vnode = (devfs_vnode *)_vnode;

	TRACE(("devfs_canpage: vnode %p\n", vnode));

	if (!S_ISCHR(vnode->stream.type)
		|| vnode->stream.u.dev.node == NULL
		|| cookie == NULL)
		return false;

	return vnode->stream.u.dev.info->read_pages != NULL;
}


static status_t
devfs_read_pages(fs_volume _fs, fs_vnode _vnode, fs_cookie _cookie, off_t pos, const iovec *vecs, size_t count, size_t *_numBytes)
{
	struct devfs_vnode *vnode = (devfs_vnode *)_vnode;
	struct devfs_cookie *cookie = (struct devfs_cookie *)_cookie;

	TRACE(("devfs_read_pages: vnode %p, vecs %p, count = %lu, pos = %Ld, size = %lu\n", vnode, vecs, count, pos, *_numBytes));

	if (!S_ISCHR(vnode->stream.type)
		|| vnode->stream.u.dev.info->read_pages == NULL
		|| cookie == NULL)
		return B_NOT_ALLOWED;

	if (vnode->stream.u.dev.part_map)
		translate_partition_access(vnode->stream.u.dev.part_map, pos, *_numBytes);

	return vnode->stream.u.dev.info->read_pages(cookie->u.dev.dcookie, pos, vecs, count, _numBytes);
}


static status_t
devfs_write_pages(fs_volume _fs, fs_vnode _vnode, fs_cookie _cookie, off_t pos, const iovec *vecs, size_t count, size_t *_numBytes)
{
	struct devfs_vnode *vnode = (devfs_vnode *)_vnode;
	struct devfs_cookie *cookie = (struct devfs_cookie *)_cookie;

	TRACE(("devfs_write_pages: vnode %p, vecs %p, count = %lu, pos = %Ld, size = %lu\n", vnode, vecs, count, pos, *_numBytes));

	if (!S_ISCHR(vnode->stream.type)
		|| vnode->stream.u.dev.info->write_pages == NULL
		|| cookie == NULL)
		return B_NOT_ALLOWED;

	if (vnode->stream.u.dev.part_map)
		translate_partition_access(vnode->stream.u.dev.part_map, pos, *_numBytes);

	return vnode->stream.u.dev.info->write_pages(cookie->u.dev.dcookie, pos, vecs, count, _numBytes);
}


static status_t
devfs_read_stat(fs_volume _fs, fs_vnode _vnode, struct stat *stat)
{
	struct devfs_vnode *vnode = (struct devfs_vnode *)_vnode;

	TRACE(("devfs_read_stat: vnode %p (%Ld), stat %p\n", vnode, vnode->id, stat));

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

	// ToDo: this only works for partitions right now - if we should decide
	//	to keep this feature, we should have a better solution
	if (S_ISCHR(vnode->stream.type)) {
		//device_geometry geometry;

		// if it's a real block device, then let's report a useful size
		if (vnode->stream.u.dev.part_map != NULL) {
			stat->st_size = vnode->stream.u.dev.part_map->info.size;
#if 0
		} else if (vnode->stream.u.dev.info->control(cookie->u.dev.dcookie,
					B_GET_GEOMETRY, &geometry, sizeof(struct device_geometry)) >= B_OK) {
			stat->st_size = 1LL * geometry.head_count * geometry.cylinder_count
				* geometry.sectors_per_track * geometry.bytes_per_sector;
#endif
		}

		// is this a real block device? then let's have it reported like that
		if (stat->st_size != 0)
			stat->st_mode = S_IFBLK | (vnode->stream.type & S_IUMSK);
	}

	return B_OK;
}


static status_t
devfs_write_stat(fs_volume _fs, fs_vnode _vnode, const struct stat *stat, uint32 statMask)
{
	struct devfs *fs = (struct devfs *)_fs;
	struct devfs_vnode *vnode = (struct devfs_vnode *)_vnode;

	TRACE(("devfs_write_stat: vnode %p (0x%Lx), stat %p\n", vnode, vnode->id, stat));

	// we cannot change the size of anything
	if (statMask & FS_WRITE_STAT_SIZE)
		return B_BAD_VALUE;

	mutex_lock(&fs->lock);

	if (statMask & FS_WRITE_STAT_MODE)
		vnode->stream.type = (vnode->stream.type & ~S_IUMSK) | (stat->st_mode & S_IUMSK);

	if (statMask & FS_WRITE_STAT_UID)
		vnode->uid = stat->st_uid;
	if (statMask & FS_WRITE_STAT_GID)
		vnode->gid = stat->st_gid;

	if (statMask & FS_WRITE_STAT_MTIME)
		vnode->modification_time = stat->st_mtime;
	if (statMask & FS_WRITE_STAT_CRTIME) 
		vnode->creation_time = stat->st_crtime;

	mutex_unlock(&fs->lock);

	notify_listener(B_STAT_CHANGED, fs->id, 0, 0, vnode->id, NULL);
	return B_OK;
}


static status_t
devfs_std_ops(int32 op, ...)
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


file_system_info gDeviceFileSystem = {
	{
		"file_systems/devfs" B_CURRENT_FS_API_VERSION,
		0,
		devfs_std_ops,
	},

	&devfs_mount,
	&devfs_unmount,
	NULL,
	NULL,
	&devfs_sync,

	&devfs_lookup,
	&devfs_get_vnode_name,

	&devfs_get_vnode,
	&devfs_put_vnode,
	&devfs_remove_vnode,

	&devfs_can_page,
	&devfs_read_pages,
	&devfs_write_pages,

	NULL,	// get_file_map

	/* common */
	&devfs_ioctl,
	&devfs_set_flags,
	&devfs_fsync,

	&devfs_read_link,
	NULL,	// write_link
	NULL,	// symlink
	NULL,	// link
	NULL,	// unlink
	NULL,	// rename

	NULL,	// access
	&devfs_read_stat,
	&devfs_write_stat,

	/* file */
	&devfs_create,
	&devfs_open,
	&devfs_close,
	&devfs_free_cookie,
	&devfs_read,
	&devfs_write,

	/* directory */
	&devfs_create_dir,
	NULL,	// remove_dir
	&devfs_open_dir,
	&devfs_close,			// we are using the same operations for directories
	&devfs_free_cookie,		// and files here - that's intended, not by accident
	&devfs_read_dir,
	&devfs_rewind_dir,

	// the other operations are not supported (attributes, indices, queries)
	NULL,
};


//	#pragma mark -
//	temporary hack to get it to work with the current device manager


static device_manager_info *pnp;


static const pnp_node_attr pnp_devfs_attrs[] =
{
	{ PNP_DRIVER_DRIVER, B_STRING_TYPE, { string: PNP_DEVFS_MODULE_NAME }},
	{ PNP_DRIVER_TYPE, B_STRING_TYPE, { string: "devfs_device_notifier" }},
	{ PNP_DRIVER_CONNECTION, B_STRING_TYPE, { string: "devfs" }},
	{ PNP_DRIVER_DEVICE_IDENTIFIER, B_STRING_TYPE, { string: "devfs" }},
	{ NULL }
};


/** someone registered a device */

static status_t
pnp_devfs_probe(pnp_node_handle parent)
{
	char *str = NULL, *filename = NULL;
	pnp_node_handle node;
	status_t status;

	TRACE(("pnp_devfs_probe()\n"));

	// make sure we can handle this parent
	if (pnp->get_attr_string(parent, PNP_DRIVER_TYPE, &str, false) != B_OK
		|| strcmp(str, PNP_DEVFS_TYPE_NAME) != 0) {
		status = B_ERROR;
		goto err1;
	}

	if (pnp->get_attr_string(parent, PNP_DEVFS_FILENAME, &filename, true) != B_OK) {
		dprintf("devfs: Item containing file name is missing\n");
		status = B_ERROR;
		goto err1;
	}

	TRACE(("Adding %s\n", filename));

	pnp_devfs_driver_info *info;
	status = pnp->load_driver(parent, NULL, (pnp_driver_info **)&info, NULL);
	if (status != B_OK)
		goto err1;

	status = pnp->register_device(parent, pnp_devfs_attrs, NULL, &node);
	if (status != B_OK || node == NULL)
		goto err2;

	//add_device(device);
	status = publish_device(sDeviceFileSystem, filename, node, info, NULL);
	if (status != B_OK)
		goto err3;
	//nudge();

	return B_OK;

err3:
	pnp->unregister_device(node);
err2:
	pnp->unload_driver(parent);
err1:
	free(str);
	free(filename);

	return status;
}


#if 0
// remove device from public list and add it to unpublish list
// (devices_lock must be hold)
static void
pnp_devfs_remove_device(device_info *device)
{
	TRACE(("removing device %s from public list\n", device->name));

	--num_devices;	
	REMOVE_DL_LIST( device, devices, );
	
	++num_unpublished_devices;
	ADD_DL_LIST_HEAD( device, devices_to_unpublish, );
	
	// (don't free it even if no handle is open - the device
	//  info block contains the hook list which may just got passed
	//  to the devfs layer; we better wait until next 
	//  publish_devices, so we are sure that devfs won't access 
	//  the hook list anymore)
}
#endif


// device got removed
static void
pnp_devfs_device_removed(pnp_node_handle node, void *cookie)
{
#if 0
	device_info *device;
	pnp_node_handle parent;
	status_t res = B_OK;
#endif
	TRACE(("pnp_devfs_device_removed()\n"));
#if 0
	parent = pnp->get_parent(node);
	
	// don't use cookie - we don't use pnp loading scheme but
	// global data and keep care of everything ourself!
	ACQUIRE_BEN( &device_list_lock );
	
	for( device = devices; device; device = device->next ) {
		if( device->parent == parent ) 
			break;
	}
	
	if( device != NULL ) {
		pnp_devfs_remove_device( device );
	} else {
		SHOW_ERROR( 0, "bug: node %p couldn't been found", node );
		res = B_NAME_NOT_FOUND;
	}
		
	RELEASE_BEN( &device_list_lock );
	
	//nudge();
#endif
}


static status_t
pnp_devfs_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return get_module(DEVICE_MANAGER_MODULE_NAME, (module_info **)&pnp);

		case B_MODULE_UNINIT:
			put_module(DEVICE_MANAGER_MODULE_NAME);
			return B_OK;

		default:
			return B_ERROR;
	}
}


pnp_driver_info gDeviceForDriversModule = {
	{
		PNP_DEVFS_MODULE_NAME,
		0 /*B_KEEP_LOADED*/,
		pnp_devfs_std_ops
	},

	NULL,
	NULL,
	pnp_devfs_probe,
	pnp_devfs_device_removed
};


//	#pragma mark -


extern "C" status_t
devfs_unpublish_file_device(const char *path)
{
	return unpublish_node(sDeviceFileSystem, path, S_IFLNK);
}


extern "C" status_t
devfs_publish_file_device(const char *path, const char *filePath)
{
	struct devfs_vnode *node;
	status_t status;

	filePath = strdup(filePath);
	if (filePath == NULL)
		return B_NO_MEMORY;

	mutex_lock(&sDeviceFileSystem->lock);

	status = publish_node(sDeviceFileSystem, path, &node);
	if (status != B_OK)
		goto out;

	// all went fine, let's initialize the node
	node->stream.type = S_IFLNK | 0644;
	node->stream.u.symlink.path = filePath;
	node->stream.u.symlink.length = strlen(filePath);

out:
	mutex_unlock(&sDeviceFileSystem->lock);
	return status;
}


extern "C" status_t
devfs_unpublish_partition(const char *path)
{
	return unpublish_node(sDeviceFileSystem, path, S_IFCHR);
}


extern "C" status_t
devfs_publish_partition(const char *path, const partition_info *info)
{
	if (path == NULL || info == NULL)
		return B_BAD_VALUE;

	TRACE(("publish partition: %s (device \"%s\", offset %Ld, size %Ld)\n",
		path, info->device, info->offset, info->size));

	// the partition and device paths must be the same until the leaves
	const char *lastPath = strrchr(path, '/');
	const char *lastDevice = strrchr(info->device, '/');
	if (lastPath == NULL || lastDevice == NULL
		|| lastPath - path != lastDevice - info->device
		|| strncmp(path, info->device, lastPath - path))
		return B_BAD_VALUE;

	devfs_vnode *device;
	status_t status = get_node_for_path(sDeviceFileSystem, info->device, &device);
	if (status != B_OK)
		return status;

	status = add_partition(sDeviceFileSystem, device, lastPath + 1, *info);

	put_vnode(sDeviceFileSystem->id, device->id);
	return status;
}


extern "C" status_t
devfs_publish_device(const char *path, void *obsolete, device_hooks *ops)
{
	return publish_device(sDeviceFileSystem, path, NULL, NULL, ops);
}

