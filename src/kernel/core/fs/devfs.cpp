/*
** Copyright 2002-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
**
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include "IOScheduler.h"

#include <SupportDefs.h>
#include <KernelExport.h>
#include <Drivers.h>
#include <pnp_devfs.h>

#include <kdevice_manager.h>
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

typedef enum {
	STREAM_TYPE_DIR = S_IFDIR,
	STREAM_TYPE_DEVICE = S_IFCHR,
	STREAM_TYPE_SYMLINK = S_IFLNK
} stream_type;

struct devfs_part_map {
	off_t offset;
	off_t size;
	uint32 logical_block_size;
	struct devfs_vnode *raw_vnode;
	
	// following info could be recreated on the fly, but it's not worth it
	uint32 session;
	uint32 partition;
};

struct devfs_stream {
	stream_type type;
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
			char *path;
		} symlink;
	} u;
};

struct devfs_vnode {
	struct devfs_vnode *all_next;
	vnode_id id;
	char *name;
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
devfs_create_vnode(struct devfs *fs, const char *name)
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

	return vnode;
}


static status_t
devfs_delete_vnode(struct devfs *fs, struct devfs_vnode *vnode, bool force_delete)
{
	// cant delete it if it's in a directory or is a directory
	// and has children
	if (!force_delete
		&& ((vnode->stream.type == STREAM_TYPE_DIR && vnode->stream.u.dir.dir_head != NULL)
			|| vnode->dir_next != NULL))
		return EPERM;

	// remove it from the global hash table
	hash_remove(fs->vnode_list_hash, vnode);

	if (vnode->stream.type == STREAM_TYPE_DEVICE) {
		// for partitions, we have to release the raw device
		if (vnode->stream.u.dev.part_map)
			put_vnode(fs->id, vnode->stream.u.dev.part_map->raw_vnode->id);

		// remove API conversion from old to new drivers
		if (vnode->stream.u.dev.node == NULL)
			free(vnode->stream.u.dev.info);
	}

	free(vnode->name);
	free(vnode);

	return 0;
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

	if (dir->stream.type != STREAM_TYPE_DIR)
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
	if (dir->stream.type != STREAM_TYPE_DIR)
		return EINVAL;

	v->dir_next = dir->stream.u.dir.dir_head;
	dir->stream.u.dir.dir_head = v;

	v->parent = dir;
	return 0;
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
			return 0;
		}
	}
	return -1;
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
devfs_get_partition_info( struct devfs *fs, struct devfs_vnode *v, 
	struct devfs_cookie *cookie, void *buf, size_t len)
{
	// ToDo: make me userspace safe!

	partition_info *info = (partition_info *)buf;
	struct devfs_part_map *part_map = v->stream.u.dev.part_map;

	if (v->stream.type != STREAM_TYPE_DEVICE || part_map == NULL)
		return EINVAL;

	info->offset = part_map->offset;
	info->size = part_map->size;
	info->logical_block_size = part_map->logical_block_size;
	info->session = part_map->session;
	info->partition = part_map->partition;

	// XXX: todo - create raw device name out of raw_vnode 
	//             we need vfs support for that (see vfs_get_cwd)
	strcpy(info->device, "something_raw");
	
	return B_NO_ERROR;
}


static status_t
devfs_set_partition(struct devfs *fs, struct devfs_vnode *vnode,
	struct devfs_cookie *cookie, partition_info &info, size_t length)
{
	struct devfs_vnode *part_node;
	status_t status;

	if (length != sizeof(partition_info)
		|| vnode->stream.type != STREAM_TYPE_DEVICE)
		return B_BAD_VALUE;

	// we don't support nested partitions
	if (vnode->stream.u.dev.part_map)
		return B_BAD_VALUE;

	// reduce checks to a minimum - things like negative offsets could be useful
	if (info.size < 0)
		return B_BAD_VALUE;

	// create partition map
	struct devfs_part_map *part_map = (struct devfs_part_map *)malloc(sizeof(*part_map));
	if (part_map == NULL)
		return B_NO_MEMORY;

	part_map->offset = info.offset;
	part_map->size = info.size;
	part_map->logical_block_size = info.logical_block_size;
	part_map->session = info.session;
	part_map->partition = info.partition;

	char name[30];
	sprintf(name, "%li_%li", info.session, info.partition);

	mutex_lock(&sDeviceFileSystem->lock);

	// you cannot change a partition once set
	if (devfs_find_in_dir(vnode->parent, name)) {
		status = B_BAD_VALUE;
		goto err1;
	}

	// increase reference count of raw device - 
	// the partition device really needs it 
	// (at least to resolve its name on GET_PARTITION_INFO)
	status = get_vnode(fs->id, vnode->id, (fs_vnode *)&part_map->raw_vnode);
	if (status < B_OK)
		goto err1;

	// now create the partition vnode
	part_node = devfs_create_vnode(fs, name);
	if (part_node == NULL) {
		status = B_NO_MEMORY;
		goto err2;
	}

	part_node->stream.type = STREAM_TYPE_DEVICE;
	part_node->stream.u.dev.node = vnode->stream.u.dev.node;
	part_node->stream.u.dev.info = vnode->stream.u.dev.info;
	part_node->stream.u.dev.ops = vnode->stream.u.dev.ops;
	part_node->stream.u.dev.part_map = part_map;
	part_node->stream.u.dev.scheduler = vnode->stream.u.dev.scheduler;

	hash_insert(fs->vnode_list_hash, part_node);

	devfs_insert_in_dir(vnode->parent, part_node);

	mutex_unlock(&sDeviceFileSystem->lock);

	TRACE(("SET_PARTITION: Added partition\n"));
	return B_OK;

err1:
	mutex_unlock(&sDeviceFileSystem->lock);

	free(part_map);
	return status;
		
err2:
	mutex_unlock(&sDeviceFileSystem->lock);

	put_vnode(fs->id, vnode->id);
	free(part_map);
	return status;
}


static inline void
translate_partition_access(devfs_part_map *map, off_t &offset, size_t &size)
{
	if (offset < 0)
		offset = 0;

	if (offset > map->size) {
		size = 0;
		return;
	}

	size = min_c(size, map->size - offset);
	offset += map->offset;
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
devfs_publish_device(const char *path, pnp_node_info *node, pnp_devfs_driver_info *info, device_hooks *ops)
{
	status_t status = B_OK;
	char temp[B_PATH_NAME_LENGTH + 1];

	TRACE(("devfs_publish_device: entry path '%s', node %p, info %p, hooks %p\n", path, node, info, ops));

	if (sDeviceFileSystem == NULL) {
		panic("devfs_publish_device called before devfs mounted\n");
		return B_ERROR;
	}

	if ((ops == NULL && (node == NULL || info == NULL))
		|| path == NULL || path[0] == '/')
		return B_BAD_VALUE;

	// are the provided device hooks okay?
	if ((ops != NULL && (ops->open == NULL || ops->close == NULL
		|| ops->read == NULL || ops->write == NULL))
		|| info != NULL && (info->open == NULL || info->close == NULL
		|| info->read == NULL || info->write == NULL))
		return B_BAD_VALUE;

	// copy the path over to a temp buffer so we can munge it
	strlcpy(temp, path, B_PATH_NAME_LENGTH);

	mutex_lock(&sDeviceFileSystem->lock);

	// create the path leading to the device
	// parse the path passed in, stripping out '/'
	struct devfs_vnode *dir = sDeviceFileSystem->root_vnode;
	struct devfs_vnode *vnode = NULL;
	int32 i = 0, last = 0;
	bool atLeaf = false;
	bool isDisk = false;

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
				if (vnode->stream.type == STREAM_TYPE_DIR) {
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
			vnode = devfs_create_vnode(sDeviceFileSystem, &temp[last]);
			if (!vnode) {
				status = B_NO_MEMORY;
				goto out;
			}
		}

		// set up the new vnode
		if (atLeaf) {
			// this is the last component
			vnode->stream.type = STREAM_TYPE_DEVICE;

			if (node != NULL)
				vnode->stream.u.dev.info = info;
			else
				vnode->stream.u.dev.info = create_new_driver_info(ops);

			vnode->stream.u.dev.node = node;
			vnode->stream.u.dev.ops = ops;

			// every raw disk gets an I/O scheduler object attached
			// ToDo: the driver should ask for a scheduler (ie. using its devfs node attributes)
			if (isDisk && !strcmp(&temp[last], "raw")) 
				vnode->stream.u.dev.scheduler = new IOScheduler(path, info);
		} else {
			// this is a dir
			vnode->stream.type = STREAM_TYPE_DIR;
			vnode->stream.u.dir.dir_head = NULL;
			vnode->stream.u.dir.jar_head = NULL;

			// mark disk devices - they might get an I/O scheduler
			if (last == 0 && !strcmp(temp, "disk"))
				isDisk = true;
		}

		hash_insert(sDeviceFileSystem->vnode_list_hash, vnode);

		devfs_insert_in_dir(dir, vnode);

		if (atLeaf)
			break;
		last = i;
		dir = vnode;
	}

out:
	mutex_unlock(&sDeviceFileSystem->lock);
	return status;
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

	fs->vnode_list_hash = hash_init(BOOTFS_HASH_SIZE, (addr)&v->all_next - (addr)v,
		&devfs_vnode_compare_func, &devfs_vnode_hash_func);
	if (fs->vnode_list_hash == NULL) {
		err = B_NO_MEMORY;
		goto err2;
	}

	// create a vnode
	v = devfs_create_vnode(fs, "");
	if (v == NULL) {
		err = B_NO_MEMORY;
		goto err3;
	}

	// set it up
	v->parent = v;

	// create a dir stream for it to hold
	v->stream.type = STREAM_TYPE_DIR;
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

	if (dir->stream.type != STREAM_TYPE_DIR)
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
devfs_create(fs_volume _fs, fs_vnode _dir, const char *name, int omode, int perms, fs_cookie *_cookie, vnode_id *new_vnid)
{
	return EROFS;
}


static status_t
devfs_open(fs_volume _fs, fs_vnode _vnode, int openMode, fs_cookie *_cookie)
{
	struct devfs_vnode *vnode = (struct devfs_vnode *)_vnode;
	struct devfs_cookie *cookie;
	status_t status = 0;

	TRACE(("devfs_open: vnode %p, oflags 0x%x, fs_cookie %p \n", vnode, openMode, _cookie));

	cookie = (struct devfs_cookie *)malloc(sizeof(struct devfs_cookie));
	if (cookie == NULL)
		return B_NO_MEMORY;

	if (vnode->stream.type == STREAM_TYPE_DEVICE) {
		if (vnode->stream.u.dev.node != NULL) {
			status = vnode->stream.u.dev.info->open(
				vnode->stream.u.dev.node->parent->cookie, openMode,
				&cookie->u.dev.dcookie);
		} else {
			status = vnode->stream.u.dev.ops->open(vnode->name, openMode,
				&cookie->u.dev.dcookie);
		}
	}

	*_cookie = cookie;

	return status;
}


static status_t
devfs_close(fs_volume _fs, fs_vnode _vnode, fs_cookie _cookie)
{
	struct devfs_vnode *vnode = (struct devfs_vnode *)_vnode;
	struct devfs_cookie *cookie = (struct devfs_cookie *)_cookie;

	TRACE(("devfs_close: entry vnode %p, cookie %p\n", vnode, cookie));

	if (vnode->stream.type == STREAM_TYPE_DEVICE) {
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

	if (vnode->stream.type == STREAM_TYPE_DEVICE) {
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
devfs_read(fs_volume _fs, fs_vnode _vnode, fs_cookie _cookie, off_t pos,
	void *buffer, size_t *_length)
{
	struct devfs_vnode *vnode = (struct devfs_vnode *)_vnode;
	struct devfs_cookie *cookie = (struct devfs_cookie *)_cookie;

	TRACE(("devfs_read: vnode %p, cookie %p, pos %Ld, len %p\n", vnode, cookie, pos, _length));

	if (vnode->stream.type != STREAM_TYPE_DEVICE)
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

	if (vnode->stream.type != STREAM_TYPE_DEVICE)
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

	if (vnode->stream.type != STREAM_TYPE_DIR)
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

	if (vnode->stream.type != STREAM_TYPE_DIR)
		return EINVAL;

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

	if (vnode->stream.type != STREAM_TYPE_DIR)
		return EINVAL;
	
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

	if (vnode->stream.type == STREAM_TYPE_DEVICE) {
		switch (op) {
			case B_GET_PARTITION_INFO:
				return devfs_get_partition_info(fs, vnode, cookie, (partition_info *)buffer, length);

			case B_SET_PARTITION:
				return devfs_set_partition(fs, vnode, cookie, *(partition_info *)buffer, length);
		}

		return vnode->stream.u.dev.info->control(cookie->u.dev.dcookie, op, buffer, length);
	}

	return B_BAD_VALUE;
}


static bool
devfs_can_page(fs_volume _fs, fs_vnode _vnode, fs_cookie cookie)
{
	struct devfs_vnode *vnode = (devfs_vnode *)_vnode;

	TRACE(("devfs_canpage: vnode %p\n", vnode));

	if (vnode->stream.type != STREAM_TYPE_DEVICE
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

	if (vnode->stream.type != STREAM_TYPE_DEVICE
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

	if (vnode->stream.type != STREAM_TYPE_DEVICE
		|| vnode->stream.u.dev.info->write_pages == NULL
		|| cookie == NULL)
		return B_NOT_ALLOWED;

	if (vnode->stream.u.dev.part_map)
		translate_partition_access(vnode->stream.u.dev.part_map, pos, *_numBytes);

	return vnode->stream.u.dev.info->write_pages(cookie->u.dev.dcookie, pos, vecs, count, _numBytes);
}


static status_t
devfs_unlink(fs_volume _fs, fs_vnode _dir, const char *name)
{
	struct devfs *fs = (struct devfs *)_fs;
	struct devfs_vnode *dir = (struct devfs_vnode *)_dir;
	struct devfs_vnode *vnode;
	status_t status = B_NO_ERROR;

	mutex_lock(&fs->lock);

	vnode = devfs_find_in_dir(dir, name);
	if (!vnode) {
		status = B_ENTRY_NOT_FOUND;
		goto err;
	}
	
	// you can unlink partitions only
	if (vnode->stream.type != STREAM_TYPE_DEVICE || !vnode->stream.u.dev.part_map) {
		status = EROFS;
		goto err;
	}

	status = devfs_remove_from_dir(vnode->parent, vnode);
	if (status < 0)
		goto err;

	status = remove_vnode(fs->id, vnode->id);

err:
	mutex_unlock(&fs->lock);

	return status;
}


static status_t
devfs_rename(fs_volume _fs, fs_vnode _olddir, const char *oldname, fs_vnode _newdir, const char *newname)
{
	return EROFS;
}


static status_t
devfs_read_stat(fs_volume _fs, fs_vnode _vnode, struct stat *stat)
{
	struct devfs_vnode *vnode = (struct devfs_vnode *)_vnode;

	TRACE(("devfs_rstat: vnode %p (%Ld), stat %p\n", vnode, vnode->id, stat));

	stat->st_ino = vnode->id;
	stat->st_size = 0;
	// ToDo: or should this be just DEFFILEMODE (0666 instead of 0644)?
	stat->st_mode = vnode->stream.type | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

	return 0;
}


static status_t
devfs_write_stat(fs_volume _fs, fs_vnode _vnode, const struct stat *stat, uint32 statMask)
{
#ifdef TRACE_DEVFS
	struct devfs_vnode *vnode = (struct devfs_vnode *)_vnode;

	TRACE(("devfs_wstat: vnode %p (%Ld), stat %p\n", vnode, vnode->id, stat));
#endif
	// cannot change anything
	return EPERM;
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
	&devfs_fsync,

	NULL,	// read_link
	NULL,	// write_link
	NULL,	// symlink
	NULL,	// link
	&devfs_unlink,
	&devfs_rename,

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
	devfs_publish_device(filename, node, info, NULL);
	//nudge();

	return B_OK;

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
devfs_unpublish_partition(const char *path)
{
	dprintf("unpublish partition: %s\n", path);
	return B_OK;
}


extern "C" status_t
devfs_publish_partition(const char *path, const partition_info *info)
{
	if (info == NULL)
		return B_BAD_VALUE;

	dprintf("publish partition: %s (device \"%s\", offset %Ld, size %Ld)\n", path, info->device, info->offset, info->size);
	return B_OK;
}


extern "C" status_t
devfs_publish_device(const char *path, void *obsolete, device_hooks *ops)
{
	return devfs_publish_device(path, NULL, NULL, ops);
}

