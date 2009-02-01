/*
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <fs/devfs.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <Drivers.h>
#include <KernelExport.h>
#include <NodeMonitor.h>

#include <arch/cpu.h>
#include <boot/kernel_args.h>
#include <boot_device.h>
#include <debug.h>
#include <elf.h>
#include <FindDirectory.h>
#include <fs/KPath.h>
#include <fs/node_monitor.h>
#include <kdevice_manager.h>
#include <lock.h>
#include <Notifications.h>
#include <util/AutoLock.h>
#include <util/khash.h>
#include <vfs.h>
#include <vm.h>

#include "BaseDevice.h"
#include "io_requests.h"
#include "legacy_drivers.h"


//#define TRACE_DEVFS
#ifdef TRACE_DEVFS
#	define TRACE(x) dprintf x
#else
#	define TRACE(x)
#endif


struct devfs_partition {
	struct devfs_vnode*	raw_device;
	partition_info		info;
};

struct driver_entry;

enum {
	kNotScanned = 0,
	kBootScan,
	kNormalScan,
};

struct devfs_stream {
	mode_t				type;
	union {
		struct stream_dir {
			struct devfs_vnode*		dir_head;
			struct list				cookies;
			int32					scanned;
		} dir;
		struct stream_dev {
			BaseDevice*				device;
			struct devfs_partition*	partition;
		} dev;
		struct stream_symlink {
			const char*				path;
			size_t					length;
		} symlink;
	} u;
};

struct devfs_vnode {
	struct devfs_vnode*	all_next;
	ino_t				id;
	char*				name;
	time_t				modification_time;
	time_t				creation_time;
	uid_t				uid;
	gid_t				gid;
	struct devfs_vnode*	parent;
	struct devfs_vnode*	dir_next;
	struct devfs_stream	stream;
};

#define DEVFS_HASH_SIZE 16

struct devfs {
	dev_t				id;
	fs_volume*			volume;
	recursive_lock		lock;
 	int32				next_vnode_id;
	hash_table*			vnode_hash;
	struct devfs_vnode*	root_vnode;
};

struct devfs_dir_cookie {
	struct list_link	link;
	struct devfs_vnode*	current;
	int32				state;	// iteration state
};

struct devfs_cookie {
	void*				device_cookie;
};

struct synchronous_io_cookie {
	BaseDevice*		device;
	void*			cookie;
};

// directory iteration states
enum {
	ITERATION_STATE_DOT		= 0,
	ITERATION_STATE_DOT_DOT	= 1,
	ITERATION_STATE_OTHERS	= 2,
	ITERATION_STATE_BEGIN	= ITERATION_STATE_DOT,
};

// extern and in a private namespace only to make forward declaration possible
namespace {
	extern fs_volume_ops kVolumeOps;
	extern fs_vnode_ops kVnodeOps;
}


static status_t get_node_for_path(struct devfs *fs, const char *path,
	struct devfs_vnode **_node);
static void get_device_name(struct devfs_vnode *vnode, char *buffer,
	size_t size);
static status_t unpublish_node(struct devfs *fs, devfs_vnode *node,
	mode_t type);
static status_t publish_device(struct devfs *fs, const char *path,
	BaseDevice* device);


/* the one and only allowed devfs instance */
static struct devfs* sDeviceFileSystem = NULL;


//	#pragma mark - devfs private


static int32
scan_mode(void)
{
	// We may scan every device twice:
	//  - once before there is a boot device,
	//  - and once when there is one

	return gBootDevice >= 0 ? kNormalScan : kBootScan;
}


static status_t
scan_for_drivers(devfs_vnode* dir)
{
	KPath path;
	if (path.InitCheck() != B_OK)
		return B_NO_MEMORY;

	get_device_name(dir, path.LockBuffer(), path.BufferSize());
	path.UnlockBuffer();

	TRACE(("scan_for_drivers: mode %ld: %s\n", scan_mode(), path.Path()));

	// scan for drivers at this path
	static int32 updateCycle = 1;
	device_manager_probe(path.Path(), updateCycle++);
	legacy_driver_probe(path.Path());

	dir->stream.u.dir.scanned = scan_mode();
	return B_OK;
}


static uint32
devfs_vnode_hash(void* _vnode, const void* _key, uint32 range)
{
	struct devfs_vnode* vnode = (struct devfs_vnode*)_vnode;
	const ino_t* key = (const ino_t*)_key;

	if (vnode != NULL)
		return vnode->id % range;

	return (uint64)*key % range;
}


static int
devfs_vnode_compare(void* _vnode, const void* _key)
{
	struct devfs_vnode* vnode = (struct devfs_vnode*)_vnode;
	const ino_t* key = (const ino_t*)_key;

	if (vnode->id == *key)
		return 0;

	return -1;
}


static struct devfs_vnode*
devfs_create_vnode(struct devfs* fs, devfs_vnode* parent, const char* name)
{
	struct devfs_vnode* vnode;

	vnode = (struct devfs_vnode*)malloc(sizeof(struct devfs_vnode));
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
devfs_delete_vnode(struct devfs* fs, struct devfs_vnode* vnode,
	bool forceDelete)
{
	// cant delete it if it's in a directory or is a directory
	// and has children
	if (!forceDelete && ((S_ISDIR(vnode->stream.type)
				&& vnode->stream.u.dir.dir_head != NULL)
			|| vnode->dir_next != NULL))
		return B_NOT_ALLOWED;

	// remove it from the global hash table
	hash_remove(fs->vnode_hash, vnode);

	if (S_ISCHR(vnode->stream.type)) {
		// for partitions, we have to release the raw device but must
		// not free the device info as it was inherited from the raw
		// device and is still in use there
		if (vnode->stream.u.dev.partition) {
			put_vnode(fs->volume,
				vnode->stream.u.dev.partition->raw_device->id);
		}
	}

	free(vnode->name);
	free(vnode);

	return B_OK;
}


/*! Makes sure none of the dircookies point to the vnode passed in */
static void
update_dir_cookies(struct devfs_vnode* dir, struct devfs_vnode* vnode)
{
	struct devfs_dir_cookie* cookie = NULL;

	while ((cookie = (devfs_dir_cookie*)list_get_next_item(
			&dir->stream.u.dir.cookies, cookie)) != NULL) {
		if (cookie->current == vnode)
			cookie->current = vnode->dir_next;
	}
}


static struct devfs_vnode*
devfs_find_in_dir(struct devfs_vnode* dir, const char* path)
{
	struct devfs_vnode* vnode;

	if (!S_ISDIR(dir->stream.type))
		return NULL;

	if (!strcmp(path, "."))
		return dir;
	if (!strcmp(path, ".."))
		return dir->parent;

	for (vnode = dir->stream.u.dir.dir_head; vnode; vnode = vnode->dir_next) {
		//TRACE(("devfs_find_in_dir: looking at entry '%s'\n", vnode->name));
		if (strcmp(vnode->name, path) == 0) {
			//TRACE(("devfs_find_in_dir: found it at %p\n", vnode));
			return vnode;
		}
	}
	return NULL;
}


static status_t
devfs_insert_in_dir(struct devfs_vnode* dir, struct devfs_vnode* vnode)
{
	if (!S_ISDIR(dir->stream.type))
		return B_BAD_VALUE;

	// make sure the directory stays sorted alphabetically

	devfs_vnode* node = dir->stream.u.dir.dir_head;
	devfs_vnode* last = NULL;
	while (node && strcmp(node->name, vnode->name) < 0) {
		last = node;
		node = node->dir_next;
	}
	if (last == NULL) {
		// the new vnode is the first entry in the list
		vnode->dir_next = dir->stream.u.dir.dir_head;
		dir->stream.u.dir.dir_head = vnode;
	} else {
		// insert after that node
		vnode->dir_next = last->dir_next;
		last->dir_next = vnode;
	}

	vnode->parent = dir;
	dir->modification_time = time(NULL);

	notify_entry_created(sDeviceFileSystem->id, dir->id, vnode->name,
		vnode->id);
	notify_stat_changed(sDeviceFileSystem->id, dir->id,
		B_STAT_MODIFICATION_TIME);

	return B_OK;
}


static status_t
devfs_remove_from_dir(struct devfs_vnode* dir, struct devfs_vnode* removeNode)
{
	struct devfs_vnode *vnode = dir->stream.u.dir.dir_head;
	struct devfs_vnode *lastNode = NULL;

	for (; vnode != NULL; lastNode = vnode, vnode = vnode->dir_next) {
		if (vnode == removeNode) {
			// make sure no dircookies point to this vnode
			update_dir_cookies(dir, vnode);

			if (lastNode)
				lastNode->dir_next = vnode->dir_next;
			else
				dir->stream.u.dir.dir_head = vnode->dir_next;
			vnode->dir_next = NULL;
			dir->modification_time = time(NULL);

			notify_entry_removed(sDeviceFileSystem->id, dir->id, vnode->name,
				vnode->id);
			notify_stat_changed(sDeviceFileSystem->id, dir->id,
				B_STAT_MODIFICATION_TIME);
			return B_OK;
		}
	}
	return B_ENTRY_NOT_FOUND;
}


static status_t
add_partition(struct devfs* fs, struct devfs_vnode* device, const char* name,
	const partition_info& info)
{
	struct devfs_vnode* partitionNode;
	status_t status;

	if (!S_ISCHR(device->stream.type))
		return B_BAD_VALUE;

	// we don't support nested partitions
	if (device->stream.u.dev.partition)
		return B_BAD_VALUE;

	// reduce checks to a minimum - things like negative offsets could be useful
	if (info.size < 0)
		return B_BAD_VALUE;

	// create partition
	struct devfs_partition* partition = (struct devfs_partition*)malloc(
		sizeof(struct devfs_partition));
	if (partition == NULL)
		return B_NO_MEMORY;

	memcpy(&partition->info, &info, sizeof(partition_info));

	RecursiveLocker locker(fs->lock);

	// you cannot change a partition once set
	if (devfs_find_in_dir(device->parent, name)) {
		status = B_BAD_VALUE;
		goto err1;
	}

	// increase reference count of raw device -
	// the partition device really needs it
	status = get_vnode(fs->volume, device->id, (void**)&partition->raw_device);
	if (status < B_OK)
		goto err1;

	// now create the partition vnode
	partitionNode = devfs_create_vnode(fs, device->parent, name);
	if (partitionNode == NULL) {
		status = B_NO_MEMORY;
		goto err2;
	}

	partitionNode->stream.type = device->stream.type;
	partitionNode->stream.u.dev.device = device->stream.u.dev.device;
	partitionNode->stream.u.dev.partition = partition;

	hash_insert(fs->vnode_hash, partitionNode);
	devfs_insert_in_dir(device->parent, partitionNode);

	TRACE(("add_partition(name = %s, offset = %Ld, size = %Ld)\n",
		name, info.offset, info.size));
	return B_OK;

err2:
	put_vnode(fs->volume, device->id);
err1:
	free(partition);
	return status;
}


static inline void
translate_partition_access(devfs_partition* partition, off_t& offset,
	size_t& size)
{
	ASSERT(offset >= 0);
	ASSERT(offset < partition->info.size);

	size = min_c(size, partition->info.size - offset);
	offset += partition->info.offset;
}


static inline void
translate_partition_access(devfs_partition* partition, io_request* request)
{
	off_t offset = request->Offset();

	ASSERT(offset >= 0);
	ASSERT(offset + request->Length() <= partition->info.size);

	request->SetOffset(offset + partition->info.offset);
}


static status_t
get_node_for_path(struct devfs *fs, const char *path,
	struct devfs_vnode **_node)
{
	return vfs_get_fs_node_from_path(fs->volume, path, true, (void **)_node);
}


static status_t
unpublish_node(struct devfs *fs, devfs_vnode *node, mode_t type)
{
	if ((node->stream.type & S_IFMT) != type)
		return B_BAD_TYPE;

	recursive_lock_lock(&fs->lock);

	status_t status = devfs_remove_from_dir(node->parent, node);
	if (status < B_OK)
		goto out;

	status = remove_vnode(fs->volume, node->id);

out:
	recursive_lock_unlock(&fs->lock);
	return status;
}


static status_t
unpublish_node(struct devfs *fs, const char *path, mode_t type)
{
	devfs_vnode *node;
	status_t status = get_node_for_path(fs, path, &node);
	if (status != B_OK)
		return status;

	status = unpublish_node(fs, node, type);

	put_vnode(fs->volume, node->id);
	return status;
}


static status_t
publish_directory(struct devfs *fs, const char *path)
{
	ASSERT_LOCKED_RECURSIVE(&fs->lock);

	// copy the path over to a temp buffer so we can munge it
	KPath tempPath(path);
	if (tempPath.InitCheck() != B_OK)
		return B_NO_MEMORY;

	TRACE(("devfs: publish directory \"%s\"\n", path));
	char *temp = tempPath.LockBuffer();

	// create the path leading to the device
	// parse the path passed in, stripping out '/'

	struct devfs_vnode *dir = fs->root_vnode;
	struct devfs_vnode *vnode = NULL;
	status_t status = B_OK;
	int32 i = 0, last = 0;

	while (temp[last]) {
		if (temp[i] == '/') {
			temp[i] = '\0';
			i++;
		} else if (temp[i] != '\0') {
			i++;
			continue;
		}

		//TRACE(("\tpath component '%s'\n", &temp[last]));

		// we have a path component
		vnode = devfs_find_in_dir(dir, &temp[last]);
		if (vnode) {
			if (S_ISDIR(vnode->stream.type)) {
				last = i;
				dir = vnode;
				continue;
			}

			// we hit something on our path that's not a directory
			status = B_FILE_EXISTS;
			goto out;
		} else {
			vnode = devfs_create_vnode(fs, dir, &temp[last]);
			if (!vnode) {
				status = B_NO_MEMORY;
				goto out;
			}
		}

		// set up the new directory
		vnode->stream.type = S_IFDIR | 0755;
		vnode->stream.u.dir.dir_head = NULL;
		list_init(&vnode->stream.u.dir.cookies);

		hash_insert(sDeviceFileSystem->vnode_hash, vnode);
		devfs_insert_in_dir(dir, vnode);

		last = i;
		dir = vnode;
	}

out:
	return status;
}


static status_t
new_node(struct devfs *fs, const char *path, struct devfs_vnode **_node,
	struct devfs_vnode **_dir)
{
	ASSERT_LOCKED_RECURSIVE(&fs->lock);

	// copy the path over to a temp buffer so we can munge it
	KPath tempPath(path);
	if (tempPath.InitCheck() != B_OK)
		return B_NO_MEMORY;

	char *temp = tempPath.LockBuffer();

	// create the path leading to the device
	// parse the path passed in, stripping out '/'

	struct devfs_vnode *dir = fs->root_vnode;
	struct devfs_vnode *vnode = NULL;
	status_t status = B_OK;
	int32 i = 0, last = 0;
	bool atLeaf = false;

	for (;;) {
		if (temp[i] == '\0') {
			atLeaf = true; // we'll be done after this one
		} else if (temp[i] == '/') {
			temp[i] = '\0';
			i++;
		} else {
			i++;
			continue;
		}

		//TRACE(("\tpath component '%s'\n", &temp[last]));

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
			list_init(&vnode->stream.u.dir.cookies);

			hash_insert(fs->vnode_hash, vnode);
			devfs_insert_in_dir(dir, vnode);
		} else {
			// this is the last component
			// Note: We do not yet insert the node into the directory, as it
			// is not yet fully initialized. Instead we return the directory
			// vnode so that the calling function can insert it after all
			// initialization is done. This ensures that no create notification
			// is sent out for a vnode that is not yet fully valid.
			*_node = vnode;
			*_dir = dir;
			break;
		}

		last = i;
		dir = vnode;
	}

out:
	return status;
}


static void
publish_node(devfs* fs, devfs_vnode* dirNode, struct devfs_vnode* node)
{
	hash_insert(fs->vnode_hash, node);
	devfs_insert_in_dir(dirNode, node);
}


static status_t
publish_device(struct devfs* fs, const char* path, BaseDevice* device)
{
	TRACE(("publish_device(path = \"%s\", device = %p)\n", path, device));

	if (sDeviceFileSystem == NULL) {
		panic("publish_device() called before devfs mounted\n");
		return B_ERROR;
	}

	if (device == NULL || path == NULL || path[0] == '\0' || path[0] == '/')
		return B_BAD_VALUE;

// TODO: this has to be done in the BaseDevice sub classes!
#if 0
	// are the provided device hooks okay?
	if (info->device_open == NULL || info->device_close == NULL
		|| info->device_free == NULL
		|| ((info->device_read == NULL || info->device_write == NULL)
			&& info->device_io == NULL))
		return B_BAD_VALUE;
#endif

	struct devfs_vnode* node;
	struct devfs_vnode* dirNode;
	status_t status;

	RecursiveLocker locker(&fs->lock);

	status = new_node(fs, path, &node, &dirNode);
	if (status != B_OK)
		return status;

	// all went fine, let's initialize the node
	node->stream.type = S_IFCHR | 0644;
	node->stream.u.dev.device = device;
	device->SetID(node->id);

	// the node is now fully valid and we may insert it into the dir
	publish_node(fs, dirNode, node);
	return B_OK;
}


/*!	Construct complete device name (as used for device_open()).
	This is safe to use only when the device is in use (and therefore
	cannot be unpublished during the iteration).
*/
static void
get_device_name(struct devfs_vnode* vnode, char* buffer, size_t size)
{
	struct devfs_vnode *leaf = vnode;
	size_t offset = 0;

	// count levels

	for (; vnode->parent && vnode->parent != vnode; vnode = vnode->parent) {
		offset += strlen(vnode->name) + 1;
	}

	// construct full path name

	for (vnode = leaf; vnode->parent && vnode->parent != vnode;
			vnode = vnode->parent) {
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


static status_t
device_read(void* _cookie, off_t offset, void* buffer, size_t* length)
{
	synchronous_io_cookie* cookie = (synchronous_io_cookie*)_cookie;
	return cookie->device->Read(cookie->cookie, offset, buffer, length);
}


static status_t
device_write(void* _cookie, off_t offset, void* buffer, size_t* length)
{
	synchronous_io_cookie* cookie = (synchronous_io_cookie*)_cookie;
	return cookie->device->Write(cookie->cookie, offset, buffer, length);
}


static int
dump_node(int argc, char **argv)
{
	if (argc < 2 || !strcmp(argv[1], "--help")) {
		kprintf("usage: %s <address>\n", argv[0]);
		return 0;
	}

	struct devfs_vnode *vnode = (struct devfs_vnode *)parse_expression(argv[1]);
	if (vnode == NULL) {
		kprintf("invalid node address\n");
		return 0;
	}

	kprintf("DEVFS NODE: %p\n", vnode);
	kprintf(" id:          %Ld\n", vnode->id);
	kprintf(" name:        \"%s\"\n", vnode->name);
	kprintf(" type:        %x\n", vnode->stream.type);
	kprintf(" parent:      %p\n", vnode->parent);
	kprintf(" dir next:    %p\n", vnode->dir_next);

	if (S_ISDIR(vnode->stream.type)) {
		kprintf(" dir scanned: %ld\n", vnode->stream.u.dir.scanned);
		kprintf(" contents:\n");

		devfs_vnode *children = vnode->stream.u.dir.dir_head;
		while (children != NULL) {
			kprintf("   %p, id %Ld\n", children, children->id);
			children = children->dir_next;
		}
	} else if (S_ISLNK(vnode->stream.type)) {
		kprintf(" symlink to:  %s\n", vnode->stream.u.symlink.path);
	} else {
		kprintf(" device:      %p\n", vnode->stream.u.dev.device);
		kprintf(" node:        %p\n", vnode->stream.u.dev.device->Node());
		kprintf(" partition:   %p\n", vnode->stream.u.dev.partition);
		if (vnode->stream.u.dev.partition != NULL) {
			partition_info& info = vnode->stream.u.dev.partition->info;
			kprintf("  raw device node: %p\n",
				vnode->stream.u.dev.partition->raw_device);
			kprintf("  offset:          %Ld\n", info.offset);
			kprintf("  size:            %Ld\n", info.size);
			kprintf("  block size:      %ld\n", info.logical_block_size);
			kprintf("  session:         %ld\n", info.session);
			kprintf("  partition:       %ld\n", info.partition);
			kprintf("  device:          %s\n", info.device);
			set_debug_variable("_raw",
				(addr_t)vnode->stream.u.dev.partition->raw_device);
		}
	}

	return 0;
}


//	#pragma mark - file system interface


static status_t
devfs_mount(fs_volume *volume, const char *devfs, uint32 flags,
	const char *args, ino_t *root_vnid)
{
	struct devfs_vnode *vnode;
	struct devfs *fs;
	status_t err;

	TRACE(("devfs_mount: entry\n"));

	if (sDeviceFileSystem) {
		TRACE(("double mount of devfs attempted\n"));
		err = B_ERROR;
		goto err;
	}

	fs = (struct devfs *)malloc(sizeof(struct devfs));
	if (fs == NULL) {
		err = B_NO_MEMORY;
		goto err;
 	}

	volume->private_volume = fs;
	volume->ops = &kVolumeOps;
	fs->volume = volume;
	fs->id = volume->id;
	fs->next_vnode_id = 0;

	recursive_lock_init(&fs->lock, "devfs lock");

	fs->vnode_hash = hash_init(DEVFS_HASH_SIZE, offsetof(devfs_vnode, all_next),
		//(addr_t)&vnode->all_next - (addr_t)vnode,
		&devfs_vnode_compare, &devfs_vnode_hash);
	if (fs->vnode_hash == NULL) {
		err = B_NO_MEMORY;
		goto err2;
	}

	// create a vnode
	vnode = devfs_create_vnode(fs, NULL, "");
	if (vnode == NULL) {
		err = B_NO_MEMORY;
		goto err3;
	}

	// set it up
	vnode->parent = vnode;

	// create a dir stream for it to hold
	vnode->stream.type = S_IFDIR | 0755;
	vnode->stream.u.dir.dir_head = NULL;
	list_init(&vnode->stream.u.dir.cookies);
	fs->root_vnode = vnode;

	hash_insert(fs->vnode_hash, vnode);
	publish_vnode(volume, vnode->id, vnode, &kVnodeOps, vnode->stream.type, 0);

	*root_vnid = vnode->id;
	sDeviceFileSystem = fs;
	return B_OK;

err3:
	hash_uninit(fs->vnode_hash);
err2:
	recursive_lock_destroy(&fs->lock);
	free(fs);
err:
	return err;
}


static status_t
devfs_unmount(fs_volume *_volume)
{
	struct devfs *fs = (struct devfs *)_volume->private_volume;
	struct devfs_vnode *vnode;
	struct hash_iterator i;

	TRACE(("devfs_unmount: entry fs = %p\n", fs));

	recursive_lock_lock(&fs->lock);

	// release the reference to the root
	put_vnode(fs->volume, fs->root_vnode->id);

	// delete all of the vnodes
	hash_open(fs->vnode_hash, &i);
	while ((vnode = (devfs_vnode *)hash_next(fs->vnode_hash, &i)) != NULL) {
		devfs_delete_vnode(fs, vnode, true);
	}
	hash_close(fs->vnode_hash, &i, false);
	hash_uninit(fs->vnode_hash);

	recursive_lock_destroy(&fs->lock);
	free(fs);

	return B_OK;
}


static status_t
devfs_sync(fs_volume *_volume)
{
	TRACE(("devfs_sync: entry\n"));

	return B_OK;
}


static status_t
devfs_lookup(fs_volume *_volume, fs_vnode *_dir, const char *name, ino_t *_id)
{
	struct devfs *fs = (struct devfs *)_volume->private_volume;
	struct devfs_vnode *dir = (struct devfs_vnode *)_dir->private_node;
	struct devfs_vnode *vnode, *vdummy;
	status_t status;

	TRACE(("devfs_lookup: entry dir %p, name '%s'\n", dir, name));

	if (!S_ISDIR(dir->stream.type))
		return B_NOT_A_DIRECTORY;

	RecursiveLocker locker(&fs->lock);

	if (dir->stream.u.dir.scanned < scan_mode())
		scan_for_drivers(dir);

	// look it up
	vnode = devfs_find_in_dir(dir, name);
	if (vnode == NULL) {
		// We don't have to rescan here, because thanks to node monitoring
		// we already know it does not exist
		return B_ENTRY_NOT_FOUND;
	}

	status = get_vnode(fs->volume, vnode->id, (void**)&vdummy);
	if (status < B_OK)
		return status;

	*_id = vnode->id;

	return B_OK;
}


static status_t
devfs_get_vnode_name(fs_volume *_volume, fs_vnode *_vnode, char *buffer,
	size_t bufferSize)
{
	struct devfs_vnode *vnode = (struct devfs_vnode *)_vnode->private_node;

	TRACE(("devfs_get_vnode_name: vnode = %p\n", vnode));

	strlcpy(buffer, vnode->name, bufferSize);
	return B_OK;
}


static status_t
devfs_get_vnode(fs_volume *_volume, ino_t id, fs_vnode *_vnode, int *_type,
	uint32 *_flags, bool reenter)
{
	struct devfs *fs = (struct devfs *)_volume->private_volume;

	TRACE(("devfs_get_vnode: asking for vnode id = %Ld, vnode = %p, r %d\n", id, _vnode, reenter));

	RecursiveLocker _(fs->lock);

	struct devfs_vnode *vnode = (devfs_vnode *)hash_lookup(fs->vnode_hash, &id);
	if (vnode == NULL)
		return B_ENTRY_NOT_FOUND;

	TRACE(("devfs_get_vnode: looked it up at %p\n", vnode));

	_vnode->private_node = vnode;
	_vnode->ops = &kVnodeOps;
	*_type = vnode->stream.type;
	*_flags = 0;
	return B_OK;
}


static status_t
devfs_put_vnode(fs_volume* _volume, fs_vnode* _vnode, bool reenter)
{
#ifdef TRACE_DEVFS
	struct devfs_vnode* vnode = (struct devfs_vnode*)_vnode->private_node;

	TRACE(("devfs_put_vnode: entry on vnode %p, id = %Ld, reenter %d\n",
		vnode, vnode->id, reenter));
#endif

	return B_OK;
}


static status_t
devfs_remove_vnode(fs_volume *_volume, fs_vnode *_v, bool reenter)
{
	struct devfs *fs = (struct devfs *)_volume->private_volume;
	struct devfs_vnode *vnode = (struct devfs_vnode *)_v->private_node;

	TRACE(("devfs_removevnode: remove %p (%Ld), reenter %d\n", vnode, vnode->id, reenter));

	RecursiveLocker locker(&fs->lock);

	if (vnode->dir_next) {
		// can't remove node if it's linked to the dir
		panic("devfs_removevnode: vnode %p asked to be removed is present in dir\n", vnode);
	}

	devfs_delete_vnode(fs, vnode, false);

	return B_OK;
}


static status_t
devfs_create(fs_volume* _volume, fs_vnode* _dir, const char* name, int openMode,
	int perms, void** _cookie, ino_t* _newVnodeID)
{
	struct devfs_vnode* dir = (struct devfs_vnode*)_dir->private_node;
	struct devfs* fs = (struct devfs*)_volume->private_volume;
	struct devfs_cookie* cookie;
	struct devfs_vnode* vnode;
	status_t status = B_OK;

	TRACE(("devfs_create: dir %p, name \"%s\", openMode 0x%x, fs_cookie %p \n", dir, name, openMode, _cookie));

	RecursiveLocker locker(fs->lock);

	// look it up
	vnode = devfs_find_in_dir(dir, name);
	if (vnode == NULL)
		return EROFS;

	if (openMode & O_EXCL)
		return B_FILE_EXISTS;

	status = get_vnode(fs->volume, vnode->id, NULL);
	if (status < B_OK)
		return status;

	locker.Unlock();

	*_newVnodeID = vnode->id;

	cookie = (struct devfs_cookie*)malloc(sizeof(struct devfs_cookie));
	if (cookie == NULL) {
		status = B_NO_MEMORY;
		goto err1;
	}

	if (S_ISCHR(vnode->stream.type)) {
		BaseDevice* device = vnode->stream.u.dev.device;
		status = device->InitDevice();
		if (status < B_OK)
			return status;

		locker.Lock();
		char path[B_FILE_NAME_LENGTH];
		get_device_name(vnode, path, sizeof(path));
		locker.Unlock();

		status = device->Open(path, openMode, &cookie->device_cookie);
		if (status != B_OK)
			device->UninitDevice();
	}
	if (status < B_OK)
		goto err2;

	*_cookie = cookie;
	return B_OK;

err2:
	free(cookie);
err1:
	put_vnode(fs->volume, vnode->id);
	return status;
}


static status_t
devfs_open(fs_volume* _volume, fs_vnode* _vnode, int openMode,
	void** _cookie)
{
	struct devfs_vnode* vnode = (struct devfs_vnode*)_vnode->private_node;
	struct devfs* fs = (struct devfs*)_volume->private_volume;
	struct devfs_cookie* cookie;
	status_t status = B_OK;

	cookie = (struct devfs_cookie*)malloc(sizeof(struct devfs_cookie));
	if (cookie == NULL)
		return B_NO_MEMORY;

	TRACE(("devfs_open: vnode %p, openMode 0x%x, cookie %p\n", vnode, openMode, cookie));
	cookie->device_cookie = NULL;

	if (S_ISCHR(vnode->stream.type)) {
		BaseDevice* device = vnode->stream.u.dev.device;
		status = device->InitDevice();
		if (status < B_OK)
			return status;

		RecursiveLocker locker(fs->lock);
		char path[B_FILE_NAME_LENGTH];
		get_device_name(vnode, path, sizeof(path));
		locker.Unlock();

		status = device->Open(path, openMode, &cookie->device_cookie);
		if (status != B_OK)
			device->UninitDevice();
	}

	if (status < B_OK)
		free(cookie);
	else
		*_cookie = cookie;

	return status;
}


static status_t
devfs_close(fs_volume *_volume, fs_vnode *_vnode, void *_cookie)
{
	struct devfs_vnode *vnode = (struct devfs_vnode *)_vnode->private_node;
	struct devfs_cookie *cookie = (struct devfs_cookie *)_cookie;

	TRACE(("devfs_close: entry vnode %p, cookie %p\n", vnode, cookie));

	if (S_ISCHR(vnode->stream.type)) {
		// pass the call through to the underlying device
		return vnode->stream.u.dev.device->Close(cookie->device_cookie);
	}

	return B_OK;
}


static status_t
devfs_free_cookie(fs_volume *_volume, fs_vnode *_vnode, void *_cookie)
{
	struct devfs_vnode *vnode = (struct devfs_vnode *)_vnode->private_node;
	struct devfs_cookie *cookie = (struct devfs_cookie *)_cookie;

	TRACE(("devfs_freecookie: entry vnode %p, cookie %p\n", vnode, cookie));

	if (S_ISCHR(vnode->stream.type)) {
		// pass the call through to the underlying device
		vnode->stream.u.dev.device->Free(cookie->device_cookie);
		vnode->stream.u.dev.device->UninitDevice();
	}

	free(cookie);
	return B_OK;
}


static status_t
devfs_fsync(fs_volume *_volume, fs_vnode *_v)
{
	return B_OK;
}


static status_t
devfs_read_link(fs_volume *_volume, fs_vnode *_link, char *buffer,
	size_t *_bufferSize)
{
	struct devfs_vnode *link = (struct devfs_vnode *)_link->private_node;

	if (!S_ISLNK(link->stream.type))
		return B_BAD_VALUE;

	if (link->stream.u.symlink.length < *_bufferSize)
		*_bufferSize = link->stream.u.symlink.length;

	memcpy(buffer, link->stream.u.symlink.path, *_bufferSize);
	return B_OK;
}


static status_t
devfs_read(fs_volume* _volume, fs_vnode* _vnode, void* _cookie, off_t pos,
	void* buffer, size_t* _length)
{
	struct devfs_vnode* vnode = (struct devfs_vnode*)_vnode->private_node;
	struct devfs_cookie* cookie = (struct devfs_cookie*)_cookie;

	//TRACE(("devfs_read: vnode %p, cookie %p, pos %Ld, len %p\n",
	//	vnode, cookie, pos, _length));

	if (!S_ISCHR(vnode->stream.type))
		return B_BAD_VALUE;

	if (pos < 0)
		return B_BAD_VALUE;

	if (vnode->stream.u.dev.partition) {
		if (pos >= vnode->stream.u.dev.partition->info.size)
			return B_BAD_VALUE;
		translate_partition_access(vnode->stream.u.dev.partition, pos,
			*_length);
	}

	if (*_length == 0)
		return B_OK;

	// pass the call through to the device
	return vnode->stream.u.dev.device->Read(cookie->device_cookie, pos, buffer,
		_length);
}


static status_t
devfs_write(fs_volume* _volume, fs_vnode* _vnode, void* _cookie, off_t pos,
	const void* buffer, size_t* _length)
{
	struct devfs_vnode* vnode = (struct devfs_vnode*)_vnode->private_node;
	struct devfs_cookie* cookie = (struct devfs_cookie*)_cookie;

	//TRACE(("devfs_write: vnode %p, cookie %p, pos %Ld, len %p\n",
	//	vnode, cookie, pos, _length));

	if (!S_ISCHR(vnode->stream.type))
		return B_BAD_VALUE;

	if (pos < 0)
		return B_BAD_VALUE;

	if (vnode->stream.u.dev.partition) {
		if (pos >= vnode->stream.u.dev.partition->info.size)
			return B_BAD_VALUE;
		translate_partition_access(vnode->stream.u.dev.partition, pos,
			*_length);
	}

	if (*_length == 0)
		return B_OK;

	return vnode->stream.u.dev.device->Write(cookie->device_cookie, pos, buffer,
		_length);
}


static status_t
devfs_create_dir(fs_volume *_volume, fs_vnode *_dir, const char *name,
	int perms, ino_t *_newVnodeID)
{
	struct devfs *fs = (struct devfs *)_volume->private_volume;
	struct devfs_vnode *dir = (struct devfs_vnode *)_dir->private_node;

	struct devfs_vnode *vnode = devfs_find_in_dir(dir, name);
	if (vnode != NULL) {
		return EEXIST;
	}

	vnode = devfs_create_vnode(fs, dir, name);
	if (vnode == NULL) {
		return B_NO_MEMORY;
	}

	// set up the new directory
	vnode->stream.type = S_IFDIR | perms;
	vnode->stream.u.dir.dir_head = NULL;
	list_init(&vnode->stream.u.dir.cookies);

	hash_insert(sDeviceFileSystem->vnode_hash, vnode);
	devfs_insert_in_dir(dir, vnode);

	*_newVnodeID = vnode->id;
	return B_OK;
}


static status_t
devfs_open_dir(fs_volume *_volume, fs_vnode *_vnode, void **_cookie)
{
	struct devfs *fs = (struct devfs *)_volume->private_volume;
	struct devfs_vnode *vnode = (struct devfs_vnode *)_vnode->private_node;
	struct devfs_dir_cookie *cookie;

	TRACE(("devfs_open_dir: vnode %p\n", vnode));

	if (!S_ISDIR(vnode->stream.type))
		return B_BAD_VALUE;

	cookie = (devfs_dir_cookie *)malloc(sizeof(devfs_dir_cookie));
	if (cookie == NULL)
		return B_NO_MEMORY;

	RecursiveLocker locker(&fs->lock);

	// make sure the directory has up-to-date contents
	if (vnode->stream.u.dir.scanned < scan_mode())
		scan_for_drivers(vnode);

	cookie->current = vnode->stream.u.dir.dir_head;
	cookie->state = ITERATION_STATE_BEGIN;

	list_add_item(&vnode->stream.u.dir.cookies, cookie);
	*_cookie = cookie;

	return B_OK;
}


static status_t
devfs_free_dir_cookie(fs_volume *_volume, fs_vnode *_vnode, void *_cookie)
{
	struct devfs_vnode *vnode = (struct devfs_vnode *)_vnode->private_node;
	struct devfs_dir_cookie *cookie = (devfs_dir_cookie *)_cookie;
	struct devfs *fs = (struct devfs *)_volume->private_volume;

	TRACE(("devfs_free_dir_cookie: entry vnode %p, cookie %p\n", vnode, cookie));

	RecursiveLocker locker(&fs->lock);

	list_remove_item(&vnode->stream.u.dir.cookies, cookie);
	free(cookie);
	return B_OK;
}


static status_t
devfs_read_dir(fs_volume *_volume, fs_vnode *_vnode, void *_cookie,
	struct dirent *dirent, size_t bufferSize, uint32 *_num)
{
	struct devfs_vnode *vnode = (devfs_vnode *)_vnode->private_node;
	struct devfs_dir_cookie *cookie = (devfs_dir_cookie *)_cookie;
	struct devfs *fs = (struct devfs *)_volume->private_volume;
	status_t status = B_OK;
	struct devfs_vnode *childNode = NULL;
	const char *name = NULL;
	struct devfs_vnode *nextChildNode = NULL;
	int32 nextState = cookie->state;

	TRACE(("devfs_read_dir: vnode %p, cookie %p, buffer %p, size %ld\n",
		_vnode, cookie, dirent, bufferSize));

	if (!S_ISDIR(vnode->stream.type))
		return B_BAD_VALUE;

	RecursiveLocker locker(&fs->lock);

	switch (cookie->state) {
		case ITERATION_STATE_DOT:
			childNode = vnode;
			name = ".";
			nextChildNode = vnode->stream.u.dir.dir_head;
			nextState = cookie->state + 1;
			break;
		case ITERATION_STATE_DOT_DOT:
			childNode = vnode->parent;
			name = "..";
			nextChildNode = vnode->stream.u.dir.dir_head;
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
		*_num = 0;
		return B_OK;
	}

	dirent->d_dev = fs->id;
	dirent->d_ino = childNode->id;
	dirent->d_reclen = strlen(name) + sizeof(struct dirent);

	if (dirent->d_reclen > bufferSize)
		return ENOBUFS;

	status = user_strlcpy(dirent->d_name, name,
		bufferSize - sizeof(struct dirent));
	if (status < B_OK)
		return status;

	cookie->current = nextChildNode;
	cookie->state = nextState;
	*_num = 1;

	return B_OK;
}


static status_t
devfs_rewind_dir(fs_volume *_volume, fs_vnode *_vnode, void *_cookie)
{
	struct devfs_vnode *vnode = (struct devfs_vnode *)_vnode->private_node;
	struct devfs_dir_cookie *cookie = (devfs_dir_cookie *)_cookie;
	struct devfs *fs = (struct devfs *)_volume->private_volume;

	TRACE(("devfs_rewind_dir: vnode %p, cookie %p\n", vnode, cookie));

	if (!S_ISDIR(vnode->stream.type))
		return B_BAD_VALUE;

	RecursiveLocker locker(&fs->lock);

	cookie->current = vnode->stream.u.dir.dir_head;
	cookie->state = ITERATION_STATE_BEGIN;

	return B_OK;
}


/*!	Forwards the opcode to the device driver, but also handles some devfs
	specific functionality, like partitions.
*/
static status_t
devfs_ioctl(fs_volume *_volume, fs_vnode *_vnode, void *_cookie, ulong op,
	void *buffer, size_t length)
{
	struct devfs_vnode *vnode = (struct devfs_vnode *)_vnode->private_node;
	struct devfs_cookie *cookie = (struct devfs_cookie *)_cookie;

	TRACE(("devfs_ioctl: vnode %p, cookie %p, op %ld, buf %p, len %ld\n",
		vnode, cookie, op, buffer, length));

	// we are actually checking for a *device* here, we don't make the
	// distinction between char and block devices
	if (S_ISCHR(vnode->stream.type)) {
		switch (op) {
			case B_GET_GEOMETRY:
			{
				struct devfs_partition *partition
					= vnode->stream.u.dev.partition;
				if (partition == NULL)
					break;

				device_geometry geometry;
				status_t status = vnode->stream.u.dev.device->Control(
					cookie->device_cookie, op, &geometry, length);
				if (status < B_OK)
					return status;

				// patch values to match partition size
				geometry.sectors_per_track = 0;
				if (geometry.bytes_per_sector == 0)
					geometry.bytes_per_sector = 512;
				geometry.sectors_per_track = partition->info.size
					/ geometry.bytes_per_sector;
				geometry.head_count = 1;
				geometry.cylinder_count = 1;

				return user_memcpy(buffer, &geometry, sizeof(device_geometry));
			}

			case B_GET_DRIVER_FOR_DEVICE:
			{
#if 0
				const char* path;
				if (!vnode->stream.u.dev.driver)
					return B_ENTRY_NOT_FOUND;
				path = vnode->stream.u.dev.driver->path;
				if (path == NULL)
					return B_ENTRY_NOT_FOUND;

				return user_strlcpy((char *)buffer, path, B_FILE_NAME_LENGTH);
#endif
				return B_ERROR;
			}

			case B_GET_PARTITION_INFO:
			{
				struct devfs_partition *partition
					= vnode->stream.u.dev.partition;
				if (!S_ISCHR(vnode->stream.type)
					|| partition == NULL
					|| length != sizeof(partition_info))
					return B_BAD_VALUE;

				return user_memcpy(buffer, &partition->info,
					sizeof(partition_info));
			}

			case B_SET_PARTITION:
				return B_NOT_ALLOWED;

			case B_GET_PATH_FOR_DEVICE:
			{
				char path[256];
				/* TODO: we might want to actually find the mountpoint
				 * of that instance of devfs...
				 * but for now we assume it's mounted on /dev
				 */
				strcpy(path, "/dev/");
				get_device_name(vnode, path + 5, sizeof(path) - 5);
				if (length && (length <= strlen(path)))
					return ERANGE;
				return user_strlcpy((char *)buffer, path, sizeof(path));
			}

			// old unsupported R5 private stuff

			case B_GET_NEXT_OPEN_DEVICE:
				dprintf("devfs: unsupported legacy ioctl B_GET_NEXT_OPEN_DEVICE\n");
				return B_NOT_SUPPORTED;
			case B_ADD_FIXED_DRIVER:
				dprintf("devfs: unsupported legacy ioctl B_ADD_FIXED_DRIVER\n");
				return B_NOT_SUPPORTED;
			case B_REMOVE_FIXED_DRIVER:
				dprintf("devfs: unsupported legacy ioctl B_REMOVE_FIXED_DRIVER\n");
				return B_NOT_SUPPORTED;

		}

		return vnode->stream.u.dev.device->Control(cookie->device_cookie,
			op, buffer, length);
	}

	return B_BAD_VALUE;
}


static status_t
devfs_set_flags(fs_volume* _volume, fs_vnode* _vnode, void* _cookie,
	int flags)
{
	struct devfs_vnode* vnode = (struct devfs_vnode*)_vnode->private_node;
	struct devfs_cookie* cookie = (struct devfs_cookie*)_cookie;

	// we need to pass the O_NONBLOCK flag to the underlying device

	if (!S_ISCHR(vnode->stream.type))
		return B_NOT_ALLOWED;

	return vnode->stream.u.dev.device->Control(cookie->device_cookie,
		flags & O_NONBLOCK ? B_SET_NONBLOCKING_IO : B_SET_BLOCKING_IO, NULL, 0);
}


static status_t
devfs_select(fs_volume* _volume, fs_vnode* _vnode, void* _cookie,
	uint8 event, selectsync* sync)
{
	struct devfs_vnode* vnode = (struct devfs_vnode*)_vnode->private_node;
	struct devfs_cookie* cookie = (struct devfs_cookie*)_cookie;

	if (!S_ISCHR(vnode->stream.type))
		return B_NOT_ALLOWED;

	// If the device has no select() hook, notify select() now.
	if (!vnode->stream.u.dev.device->HasSelect())
		return notify_select_event((selectsync*)sync, event);

	return vnode->stream.u.dev.device->Select(cookie->device_cookie, event,
		(selectsync*)sync);
}


static status_t
devfs_deselect(fs_volume* _volume, fs_vnode* _vnode, void* _cookie,
	uint8 event, selectsync* sync)
{
	struct devfs_vnode* vnode = (struct devfs_vnode*)_vnode->private_node;
	struct devfs_cookie* cookie = (struct devfs_cookie*)_cookie;

	if (!S_ISCHR(vnode->stream.type))
		return B_NOT_ALLOWED;

	// If the device has no select() hook, notify select() now.
	if (!vnode->stream.u.dev.device->HasDeselect())
		return B_OK;

	return vnode->stream.u.dev.device->Deselect(cookie->device_cookie, event,
		(selectsync*)sync);
}


static bool
devfs_can_page(fs_volume *_volume, fs_vnode *_vnode, void *cookie)
{
	struct devfs_vnode *vnode = (devfs_vnode *)_vnode->private_node;

	//TRACE(("devfs_canpage: vnode %p\n", vnode));

	if (!S_ISCHR(vnode->stream.type)
		|| vnode->stream.u.dev.device->Node() == NULL
		|| cookie == NULL)
		return false;

	return vnode->stream.u.dev.device->HasRead()
		|| vnode->stream.u.dev.device->HasIO();
}


static status_t
devfs_read_pages(fs_volume *_volume, fs_vnode *_vnode, void *_cookie,
	off_t pos, const iovec *vecs, size_t count, size_t *_numBytes)
{
	struct devfs_vnode *vnode = (devfs_vnode *)_vnode->private_node;
	struct devfs_cookie *cookie = (struct devfs_cookie *)_cookie;

	//TRACE(("devfs_read_pages: vnode %p, vecs %p, count = %lu, pos = %Ld, size = %lu\n", vnode, vecs, count, pos, *_numBytes));

	if (!S_ISCHR(vnode->stream.type)
		|| (!vnode->stream.u.dev.device->HasRead()
			&& !vnode->stream.u.dev.device->HasIO())
		|| cookie == NULL)
		return B_NOT_ALLOWED;

	if (pos < 0)
		return B_BAD_VALUE;

	if (vnode->stream.u.dev.partition) {
		if (pos >= vnode->stream.u.dev.partition->info.size)
			return B_BAD_VALUE;
		translate_partition_access(vnode->stream.u.dev.partition, pos,
			*_numBytes);
	}

	if (vnode->stream.u.dev.device->HasIO()) {
		// TODO: use io_requests for this!
	}

	// emulate read_pages() using read()

	status_t error = B_OK;
	size_t bytesTransferred = 0;

	size_t remainingBytes = *_numBytes;
	for (size_t i = 0; i < count && remainingBytes > 0; i++) {
		size_t toRead = min_c(vecs[i].iov_len, remainingBytes);
		size_t length = toRead;

		error = vnode->stream.u.dev.device->Read(cookie->device_cookie, pos,
			vecs[i].iov_base, &length);
		if (error != B_OK)
			break;

		pos += length;
		bytesTransferred += length;
		remainingBytes -= length;

		if (length < toRead)
			break;
	}

	*_numBytes = bytesTransferred;

	return bytesTransferred > 0 ? B_OK : error;
}


static status_t
devfs_write_pages(fs_volume* _volume, fs_vnode* _vnode, void* _cookie,
	off_t pos, const iovec* vecs, size_t count, size_t* _numBytes)
{
	struct devfs_vnode *vnode = (devfs_vnode *)_vnode->private_node;
	struct devfs_cookie *cookie = (struct devfs_cookie *)_cookie;

	//TRACE(("devfs_write_pages: vnode %p, vecs %p, count = %lu, pos = %Ld, size = %lu\n", vnode, vecs, count, pos, *_numBytes));

	if (!S_ISCHR(vnode->stream.type)
		|| (!vnode->stream.u.dev.device->HasWrite()
			&& !vnode->stream.u.dev.device->HasIO())
		|| cookie == NULL)
		return B_NOT_ALLOWED;

	if (pos < 0)
		return B_BAD_VALUE;

	if (vnode->stream.u.dev.partition) {
		if (pos >= vnode->stream.u.dev.partition->info.size)
			return B_BAD_VALUE;
		translate_partition_access(vnode->stream.u.dev.partition, pos,
			*_numBytes);
	}

	if (vnode->stream.u.dev.device->HasIO()) {
		// TODO: use io_requests for this!
	}

	// emulate write_pages() using write()

	status_t error = B_OK;
	size_t bytesTransferred = 0;

	size_t remainingBytes = *_numBytes;
	for (size_t i = 0; i < count && remainingBytes > 0; i++) {
		size_t toWrite = min_c(vecs[i].iov_len, remainingBytes);
		size_t length = toWrite;

		error = vnode->stream.u.dev.device->Write(cookie->device_cookie, pos,
			vecs[i].iov_base, &length);
		if (error != B_OK)
			break;

		pos += length;
		bytesTransferred += length;
		remainingBytes -= length;

		if (length < toWrite)
			break;
	}

	*_numBytes = bytesTransferred;

	return bytesTransferred > 0 ? B_OK : error;
}


static status_t
devfs_io(fs_volume *volume, fs_vnode *_vnode, void *_cookie,
	io_request *request)
{
	TRACE(("[%ld] devfs_io(request: %p)\n", find_thread(NULL), request));

	devfs_vnode* vnode = (devfs_vnode*)_vnode->private_node;
	devfs_cookie* cookie = (devfs_cookie*)_cookie;

	bool isWrite = request->IsWrite();

	if (!S_ISCHR(vnode->stream.type)
		|| (((isWrite && !vnode->stream.u.dev.device->HasWrite())
				|| (!isWrite && !vnode->stream.u.dev.device->HasRead()))
			&& !vnode->stream.u.dev.device->HasIO())
		|| cookie == NULL) {
		return B_NOT_ALLOWED;
	}

	if (vnode->stream.u.dev.partition) {
		if (request->Offset() + request->Length()
				>= vnode->stream.u.dev.partition->info.size) {
			return B_BAD_VALUE;
		}
		translate_partition_access(vnode->stream.u.dev.partition, request);
	}

	if (vnode->stream.u.dev.device->HasIO())
		return vnode->stream.u.dev.device->IO(cookie->device_cookie, request);

	synchronous_io_cookie synchronousCookie = {
		vnode->stream.u.dev.device,
		cookie->device_cookie
	};

	return vfs_synchronous_io(request,
		request->IsWrite() ? &device_write : &device_read, &synchronousCookie);
}


static status_t
devfs_read_stat(fs_volume* _volume, fs_vnode* _vnode, struct stat* stat)
{
	struct devfs_vnode* vnode = (struct devfs_vnode*)_vnode->private_node;

	TRACE(("devfs_read_stat: vnode %p (%Ld), stat %p\n", vnode, vnode->id,
		stat));

	stat->st_ino = vnode->id;
	stat->st_size = 0;
	stat->st_mode = vnode->stream.type;

	stat->st_nlink = 1;
	stat->st_blksize = 65536;
	stat->st_blocks = 0;

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
		if (vnode->stream.u.dev.partition != NULL) {
			stat->st_size = vnode->stream.u.dev.partition->info.size;
#if 0
		} else if (vnode->stream.u.dev.info->control(cookie->device_cookie,
					B_GET_GEOMETRY, &geometry, sizeof(struct device_geometry)) >= B_OK) {
			stat->st_size = 1LL * geometry.head_count * geometry.cylinder_count
				* geometry.sectors_per_track * geometry.bytes_per_sector;
#endif
		}

		// is this a real block device? then let's have it reported like that
		if (stat->st_size != 0)
			stat->st_mode = S_IFBLK | (vnode->stream.type & S_IUMSK);
	} else if (S_ISLNK(vnode->stream.type)) {
		stat->st_size = vnode->stream.u.symlink.length;
	}

	return B_OK;
}


static status_t
devfs_write_stat(fs_volume* _volume, fs_vnode* _vnode, const struct stat* stat,
	uint32 statMask)
{
	struct devfs* fs = (struct devfs*)_volume->private_volume;
	struct devfs_vnode* vnode = (struct devfs_vnode*)_vnode->private_node;

	TRACE(("devfs_write_stat: vnode %p (0x%Lx), stat %p\n", vnode, vnode->id,
		stat));

	// we cannot change the size of anything
	if (statMask & B_STAT_SIZE)
		return B_BAD_VALUE;

	RecursiveLocker locker(&fs->lock);

	if (statMask & B_STAT_MODE) {
		vnode->stream.type = (vnode->stream.type & ~S_IUMSK)
			| (stat->st_mode & S_IUMSK);
	}

	if (statMask & B_STAT_UID)
		vnode->uid = stat->st_uid;
	if (statMask & B_STAT_GID)
		vnode->gid = stat->st_gid;

	if (statMask & B_STAT_MODIFICATION_TIME)
		vnode->modification_time = stat->st_mtime;
	if (statMask & B_STAT_CREATION_TIME)
		vnode->creation_time = stat->st_crtime;

	notify_stat_changed(fs->id, vnode->id, statMask);
	return B_OK;
}


static status_t
devfs_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			add_debugger_command("devfs_node", &dump_node,
				"info about a private devfs node");

			legacy_driver_init();
			return B_OK;

		case B_MODULE_UNINIT:
			remove_debugger_command("devfs_node", &dump_node);
			return B_OK;

		default:
			return B_ERROR;
	}
}

namespace {

fs_volume_ops kVolumeOps = {
	&devfs_unmount,
	NULL,
	NULL,
	&devfs_sync,
	&devfs_get_vnode,

	// the other operations are not supported (attributes, indices, queries)
	NULL,
};

fs_vnode_ops kVnodeOps = {
	&devfs_lookup,
	&devfs_get_vnode_name,

	&devfs_put_vnode,
	&devfs_remove_vnode,

	&devfs_can_page,
	&devfs_read_pages,
	&devfs_write_pages,

	&devfs_io,
	NULL,	// cancel_io()

	NULL,	// get_file_map

	/* common */
	&devfs_ioctl,
	&devfs_set_flags,
	&devfs_select,
	&devfs_deselect,
	&devfs_fsync,

	&devfs_read_link,
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
	&devfs_close,
		// same as for files - it does nothing for directories, anyway
	&devfs_free_dir_cookie,
	&devfs_read_dir,
	&devfs_rewind_dir,

	// attributes operations are not supported
	NULL,
};

}	// namespace

file_system_module_info gDeviceFileSystem = {
	{
		"file_systems/devfs" B_CURRENT_FS_API_VERSION,
		0,
		devfs_std_ops,
	},

	"devfs",					// short_name
	"Device File System",		// pretty_name
	0,							// DDM flags

	NULL,	// identify_partition()
	NULL,	// scan_partition()
	NULL,	// free_identify_partition_cookie()
	NULL,	// free_partition_content_cookie()

	&devfs_mount,
};


//	#pragma mark - kernel private API


extern "C" status_t
devfs_unpublish_file_device(const char *path)
{
	return unpublish_node(sDeviceFileSystem, path, S_IFLNK);
}


extern "C" status_t
devfs_publish_file_device(const char *path, const char *filePath)
{
	struct devfs_vnode *node;
	struct devfs_vnode *dirNode;
	status_t status;

	filePath = strdup(filePath);
	if (filePath == NULL)
		return B_NO_MEMORY;

	RecursiveLocker locker(&sDeviceFileSystem->lock);

	status = new_node(sDeviceFileSystem, path, &node, &dirNode);
	if (status != B_OK) {
		free((char*)filePath);
		return status;
	}

	// all went fine, let's initialize the node
	node->stream.type = S_IFLNK | 0644;
	node->stream.u.symlink.path = filePath;
	node->stream.u.symlink.length = strlen(filePath);

	// the node is now fully valid and we may insert it into the dir
	publish_node(sDeviceFileSystem, dirNode, node);
	return B_OK;
}


extern "C" status_t
devfs_unpublish_partition(const char *path)
{
	return unpublish_node(sDeviceFileSystem, path, S_IFCHR);
}


extern "C" status_t
devfs_publish_partition(const char* path, const partition_info* info)
{
	if (path == NULL || info == NULL)
		return B_BAD_VALUE;

	TRACE(("publish partition: %s (device \"%s\", offset %Ld, size %Ld)\n",
		path, info->device, info->offset, info->size));

	// the partition and device paths must be the same until the leaves
	const char* lastPath = strrchr(path, '/');
	const char* lastDevice = strrchr(info->device, '/');
	if (lastPath == NULL || lastDevice == NULL)
		return B_BAD_VALUE;

	size_t length = lastDevice - (lastPath - path) - info->device;
	if (strncmp(path, info->device + length, lastPath - path))
		return B_BAD_VALUE;

	devfs_vnode* device;
	status_t status = get_node_for_path(sDeviceFileSystem, info->device,
		&device);
	if (status != B_OK)
		return status;

	status = add_partition(sDeviceFileSystem, device, lastPath + 1, *info);

	put_vnode(sDeviceFileSystem->volume, device->id);
	return status;
}


extern "C" status_t
devfs_publish_directory(const char* path)
{
	RecursiveLocker locker(&sDeviceFileSystem->lock);

	return publish_directory(sDeviceFileSystem, path);
}


extern "C" status_t
devfs_unpublish_device(const char* path, bool disconnect)
{
	devfs_vnode* node;
	status_t status = get_node_for_path(sDeviceFileSystem, path, &node);
	if (status != B_OK)
		return status;

	status = unpublish_node(sDeviceFileSystem, node, S_IFCHR);

	if (status == B_OK && disconnect)
		vfs_disconnect_vnode(sDeviceFileSystem->id, node->id);

	put_vnode(sDeviceFileSystem->volume, node->id);
	return status;
}


//	#pragma mark - device_manager private API


status_t
devfs_publish_device(const char* path, BaseDevice* device)
{
	return publish_device(sDeviceFileSystem, path, device);
}


status_t
devfs_unpublish_device(BaseDevice* device, bool disconnect)
{
	devfs_vnode* node;
	status_t status = get_vnode(sDeviceFileSystem->volume, device->ID(),
		(void**)&node);
	if (status != B_OK)
		return status;

	status = unpublish_node(sDeviceFileSystem, node, S_IFCHR);

	if (status == B_OK && disconnect)
		vfs_disconnect_vnode(sDeviceFileSystem->id, node->id);

	put_vnode(sDeviceFileSystem->volume, node->id);
	return status;
}


//	#pragma mark - support API for legacy drivers


extern "C" status_t
devfs_rescan_driver(const char* driverName)
{
	TRACE(("devfs_rescan_driver: %s\n", driverName));

	return legacy_driver_rescan(driverName);
}


extern "C" status_t
devfs_publish_device(const char* path, device_hooks* hooks)
{
	return legacy_driver_publish(path, hooks);
}


