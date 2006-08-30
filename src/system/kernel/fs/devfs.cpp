/*
 * Copyright 2002-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include "IOScheduler.h"

#include <KernelExport.h>
#include <Drivers.h>
#include <pnp_devfs.h>
#include <NodeMonitor.h>

#include <boot_device.h>
#include <kdevice_manager.h>
#include <KPath.h>
#include <vfs.h>
#include <debug.h>
#include <util/khash.h>
#include <util/AutoLock.h>
#include <elf.h>
#include <lock.h>
#include <vm.h>
#include <arch/cpu.h>

#include <devfs.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>


//#define TRACE_DEVFS
#ifdef TRACE_DEVFS
#	define TRACE(x) dprintf x
#else
#	define TRACE(x)
#endif


struct devfs_partition {
	struct devfs_vnode	*raw_device;
	partition_info		info;
};

struct driver_entry;

enum {
	kNotScanned = 0,
	kBootScan,
	kNormalScan,
};

struct devfs_stream {
	mode_t type;
	union {
		struct stream_dir {
			struct devfs_vnode		*dir_head;
			struct list				cookies;
			int32					scanned;
		} dir;
		struct stream_dev {
			device_node_info		*node;
			pnp_devfs_driver_info	*info;
			device_hooks			*ops;
			struct devfs_partition	*partition;
			IOScheduler				*scheduler;
			driver_entry			*driver;
		} dev;
		struct stream_symlink {
			const char				*path;
			size_t					length;
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

#define DEVFS_HASH_SIZE 16

struct devfs {
	mount_id	id;
	recursive_lock lock;
	int32		next_vnode_id;
	hash_table	*vnode_hash;
	struct devfs_vnode *root_vnode;
	hash_table	*driver_hash;
};

struct devfs_dir_cookie {
	struct list_link link;
	struct devfs_vnode *current;
	int32 state;	// iteration state
};

struct devfs_cookie {
	void *device_cookie;
};

// directory iteration states
enum {
	ITERATION_STATE_DOT		= 0,
	ITERATION_STATE_DOT_DOT	= 1,
	ITERATION_STATE_OTHERS	= 2,
	ITERATION_STATE_BEGIN	= ITERATION_STATE_DOT,
};

struct driver_entry {
	driver_entry	*next;
	const char		*path;
	mount_id		device;
	vnode_id		node;
	time_t			last_modified;
	image_id		image;
};


static void get_device_name(struct devfs_vnode *vnode, char *buffer, size_t size);
static status_t publish_device(struct devfs *fs, const char *path,
					device_node_info *deviceNode, pnp_devfs_driver_info *info,
					driver_entry *driver, device_hooks *ops);


/* the one and only allowed devfs instance */
static struct devfs *sDeviceFileSystem = NULL;


//	#pragma mark - driver private


static uint32
driver_entry_hash(void *_driver, const void *_key, uint32 range)
{
	driver_entry *driver = (driver_entry *)_driver;
	const vnode_id *key = (const vnode_id *)_key;

	if (driver != NULL)
		return driver->node % range;

	return (uint64)*key % range;
}


static int
driver_entry_compare(void *_driver, const void *_key)
{
	driver_entry *driver = (driver_entry *)_driver;
	const vnode_id *key = (const vnode_id *)_key;

	if (driver->node == *key)
		return 0;

	return -1;
}


static status_t
load_driver(driver_entry *driver)
{
	status_t (*init_hardware)(void);
	status_t (*init_driver)(void);
	const char **devicePaths;
	int32 exported = 0;
	status_t status;

	// load the module
	image_id image = load_kernel_add_on(driver->path);
	if (image < 0)
		return image;

	// for prettier debug output
	const char *name = strrchr(driver->path, '/');
	if (name == NULL)
		name = driver->path;
	else
		name++;

	// For a valid device driver the following exports are required

	uint32 *api_version;
	if (get_image_symbol(image, "api_version", B_SYMBOL_TYPE_DATA, (void **)&api_version) != B_OK)
		dprintf("%s: api_version missing\n", name);

	device_hooks *(*find_device)(const char *);
	const char **(*publish_devices)(void);
	if (get_image_symbol(image, "publish_devices", B_SYMBOL_TYPE_TEXT, (void **)&publish_devices) != B_OK
		|| get_image_symbol(image, "find_device", B_SYMBOL_TYPE_TEXT, (void **)&find_device) != B_OK) {
		dprintf("%s: mandatory driver symbol(s) missing!\n", name);
		status = B_BAD_VALUE;
		goto error1;
	}

	// Init the driver

	if (get_image_symbol(image, "init_hardware", B_SYMBOL_TYPE_TEXT,
			(void **)&init_hardware) == B_OK
		&& (status = init_hardware()) != B_OK) {
		dprintf("%s: init_hardware() failed: %s\n", name, strerror(status));
		status = ENXIO;
		goto error1;
	}

	if (get_image_symbol(image, "init_driver", B_SYMBOL_TYPE_TEXT,
			(void **)&init_driver) == B_OK
		&& (status = init_driver()) != B_OK) {
		dprintf("%s: init_driver() failed: %s\n", name, strerror(status));
		status = ENXIO;
		goto error2;
	}

	// The driver has successfully been initialized, now we can
	// finally publish its device entries

	// we keep the driver loaded if it exports at least a single interface
	// ToDo: we could/should always unload drivers until they will be used for real
	// ToDo: this function is probably better kept in devfs, so that it could remember
	//	the driver stuff (and even keep it loaded if there is enough memory)
	devicePaths = publish_devices();
	if (devicePaths == NULL) {
		dprintf("%s: publish_devices() returned NULL.\n", name);
		status = ENXIO;
		goto error3;
	}

	for (; devicePaths[0]; devicePaths++) {
		device_hooks *hooks = find_device(devicePaths[0]);

		if (hooks != NULL
			&& publish_device(sDeviceFileSystem, devicePaths[0],
					NULL, NULL, driver, hooks) == B_OK)
			exported++;
	}

	// we're all done, driver will be kept loaded (for now, see above comment)
	if (exported > 0) {
		driver->image = image;
		return B_OK;
	}

	status = B_ERROR;	// whatever...

error3:
	{
		status_t (*uninit_driver)(void);
		if (get_image_symbol(image, "uninit_driver", B_SYMBOL_TYPE_TEXT,
				(void **)&uninit_driver) == B_OK)
			uninit_driver();
	}

error2:
	{
		status_t (*uninit_hardware)(void);
		if (get_image_symbol(image, "uninit_hardware", B_SYMBOL_TYPE_TEXT,
				(void **)&uninit_hardware) == B_OK)
			uninit_hardware();
	}

error1:
	unload_kernel_add_on(image);
	driver->image = status;

	return status;
}


/** This is no longer part of the public kernel API, so we just export the symbol */

status_t load_driver_symbols(const char *driverName);
status_t
load_driver_symbols(const char *driverName)
{
	// This will be globally done for the whole kernel via the settings file.
	// We don't have to do anything here.

	return B_OK;
}


static int32
scan_mode(void)
{
	// We may scan every device twice:
	//  - once before there is a boot device,
	//  - and once when there is one

	return gBootDevice >= 0 ? kNormalScan : kBootScan;
}


static status_t
scan_for_drivers(devfs_vnode *dir)
{
	KPath path;
	if (path.InitCheck() != B_OK)
		return B_NO_MEMORY;

	get_device_name(dir, path.LockBuffer(), path.BufferSize());
	path.UnlockBuffer();

	TRACE(("scan_for_drivers: mode %ld: %s\n", scan_mode(), path.Path()));

	// scan for drivers at this path
	probe_for_device_type(path.Path());

	dir->stream.u.dir.scanned = scan_mode();
	return B_OK;
}


//	#pragma mark - devfs private


static uint32
devfs_vnode_hash(void *_vnode, const void *_key, uint32 range)
{
	struct devfs_vnode *vnode = (struct devfs_vnode *)_vnode;
	const vnode_id *key = (const vnode_id *)_key;

	if (vnode != NULL)
		return vnode->id % range;

	return (uint64)*key % range;
}


static int
devfs_vnode_compare(void *_vnode, const void *_key)
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
		return B_NOT_ALLOWED;

	// remove it from the global hash table
	hash_remove(fs->vnode_hash, vnode);

	if (S_ISCHR(vnode->stream.type)) {
		// for partitions, we have to release the raw device
		if (vnode->stream.u.dev.partition)
			put_vnode(fs->id, vnode->stream.u.dev.partition->raw_device->id);
		else
			delete vnode->stream.u.dev.scheduler;

		// remove API conversion from old to new drivers
		if (vnode->stream.u.dev.node == NULL)
			free(vnode->stream.u.dev.info);
	}

	free(vnode->name);
	free(vnode);

	return B_OK;
}


/** makes sure none of the dircookies point to the vnode passed in */

static void
update_dir_cookies(struct devfs_vnode *dir, struct devfs_vnode *vnode)
{
	struct devfs_dir_cookie *cookie = NULL;

	while ((cookie = (devfs_dir_cookie *)list_get_next_item(&dir->stream.u.dir.cookies, cookie)) != NULL) {
		if (cookie->current == vnode)
			cookie->current = vnode->dir_next;
	}
}


static struct devfs_vnode *
devfs_find_in_dir(struct devfs_vnode *dir, const char *path)
{
	struct devfs_vnode *vnode;

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
devfs_insert_in_dir(struct devfs_vnode *dir, struct devfs_vnode *vnode)
{
	if (!S_ISDIR(dir->stream.type))
		return B_BAD_VALUE;

	// make sure the directory stays sorted alphabetically

	devfs_vnode *node = dir->stream.u.dir.dir_head, *last = NULL;
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

	notify_entry_created(sDeviceFileSystem->id, dir->id, vnode->name, vnode->id);
	notify_stat_changed(sDeviceFileSystem->id, dir->id, B_STAT_MODIFICATION_TIME);

	return B_OK;
}


static status_t
devfs_remove_from_dir(struct devfs_vnode *dir, struct devfs_vnode *removeNode)
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

			notify_entry_removed(sDeviceFileSystem->id, dir->id, vnode->name, vnode->id);
			notify_stat_changed(sDeviceFileSystem->id, dir->id, B_STAT_MODIFICATION_TIME);
			return B_OK;
		}
	}
	return B_ENTRY_NOT_FOUND;
}


static status_t
add_partition(struct devfs *fs, struct devfs_vnode *device,
	const char *name, const partition_info &info)
{
	struct devfs_vnode *partitionNode;
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
	struct devfs_partition *partition = (struct devfs_partition *)malloc(sizeof(struct devfs_partition));
	if (partition == NULL)
		return B_NO_MEMORY;

	memcpy(&partition->info, &info, sizeof(partition_info));

	RecursiveLocker locker(&fs->lock);

	// you cannot change a partition once set
	if (devfs_find_in_dir(device->parent, name)) {
		status = B_BAD_VALUE;
		goto err1;
	}

	// increase reference count of raw device - 
	// the partition device really needs it 
	status = get_vnode(fs->id, device->id, (fs_vnode *)&partition->raw_device);
	if (status < B_OK)
		goto err1;

	// now create the partition vnode
	partitionNode = devfs_create_vnode(fs, device->parent, name);
	if (partitionNode == NULL) {
		status = B_NO_MEMORY;
		goto err2;
	}

	partitionNode->stream.type = device->stream.type;
	partitionNode->stream.u.dev.node = device->stream.u.dev.node;
	partitionNode->stream.u.dev.info = device->stream.u.dev.info;
	partitionNode->stream.u.dev.ops = device->stream.u.dev.ops;
	partitionNode->stream.u.dev.partition = partition;
	partitionNode->stream.u.dev.scheduler = device->stream.u.dev.scheduler;

	hash_insert(fs->vnode_hash, partitionNode);
	devfs_insert_in_dir(device->parent, partitionNode);

	TRACE(("add_partition(name = %s, offset = %Ld, size = %Ld)\n",
		name, info.offset, info.size));
	return B_OK;

err2:
	put_vnode(fs->id, device->id);
err1:
	free(partition);
	return status;
}


static inline void
translate_partition_access(devfs_partition *partition, off_t &offset, size_t &size)
{
	if (offset < 0)
		offset = 0;

	if (offset > partition->info.size) {
		size = 0;
		return;
	}

	size = min_c(size, partition->info.size - offset);
	offset += partition->info.offset;
}


static pnp_devfs_driver_info *
create_new_driver_info(device_hooks *ops)
{
	pnp_devfs_driver_info *info = (pnp_devfs_driver_info *)malloc(sizeof(pnp_devfs_driver_info));
	if (info == NULL)
		return NULL;

	memset(info, 0, sizeof(driver_module_info));

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
unpublish_node(struct devfs *fs, devfs_vnode *node, mode_t type)
{
	if ((node->stream.type & S_IFMT) != type)
		return B_BAD_TYPE;

	recursive_lock_lock(&fs->lock);

	status_t status = devfs_remove_from_dir(node->parent, node);
	if (status < B_OK)
		goto out;

	status = remove_vnode(fs->id, node->id);

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

	put_vnode(fs->id, node->id);
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
publish_node(struct devfs *fs, const char *path, struct devfs_vnode **_node)
{
	ASSERT_LOCKED_MUTEX(&fs->lock);

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
		} else {
			// this is the last component
			*_node = vnode;
		}

		hash_insert(sDeviceFileSystem->vnode_hash, vnode);
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
publish_device(struct devfs *fs, const char *path, device_node_info *deviceNode,
	pnp_devfs_driver_info *info, driver_entry *driver, device_hooks *ops)
{
	TRACE(("publish_device(path = \"%s\", node = %p, info = %p, hooks = %p)\n",
		path, deviceNode, info, ops));

	if (sDeviceFileSystem == NULL) {
		panic("publish_device() called before devfs mounted\n");
		return B_ERROR;
	}

	if ((ops == NULL && (deviceNode == NULL || info == NULL))
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

	RecursiveLocker locker(&fs->lock);

	status = publish_node(fs, path, &node);
	if (status != B_OK)
		return status;

	// all went fine, let's initialize the node
	node->stream.type = S_IFCHR | 0644;

	if (deviceNode != NULL)
		node->stream.u.dev.info = info;
	else
		node->stream.u.dev.info = create_new_driver_info(ops);

	node->stream.u.dev.node = deviceNode;
	node->stream.u.dev.driver = driver;
	node->stream.u.dev.ops = ops;

	// every raw disk gets an I/O scheduler object attached
	// ToDo: the driver should ask for a scheduler (ie. using its devfs node attributes)
	if (isDisk && !strcmp(node->name, "raw")) 
		node->stream.u.dev.scheduler = new IOScheduler(path, info);

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


//	#pragma mark - file system interface


static status_t
devfs_mount(mount_id id, const char *devfs, uint32 flags, const char *args,
	fs_volume *_fs, vnode_id *root_vnid)
{
	struct devfs_vnode *vnode;
	struct devfs *fs;
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

	err = recursive_lock_init(&fs->lock, "devfs lock");
	if (err < B_OK)
		goto err1;

	fs->vnode_hash = hash_init(DEVFS_HASH_SIZE, offsetof(devfs_vnode, all_next),
		//(addr_t)&vnode->all_next - (addr_t)vnode,
		&devfs_vnode_compare, &devfs_vnode_hash);
	if (fs->vnode_hash == NULL) {
		err = B_NO_MEMORY;
		goto err2;
	}

	fs->driver_hash = hash_init(DEVFS_HASH_SIZE, offsetof(driver_entry, next),
		&driver_entry_compare, &driver_entry_hash);
	if (fs->driver_hash == NULL) {
		err = B_NO_MEMORY;
		goto err3;
	}

	// create a vnode
	vnode = devfs_create_vnode(fs, NULL, "");
	if (vnode == NULL) {
		err = B_NO_MEMORY;
		goto err4;
	}

	// set it up
	vnode->parent = vnode;

	// create a dir stream for it to hold
	vnode->stream.type = S_IFDIR | 0755;
	vnode->stream.u.dir.dir_head = NULL;
	list_init(&vnode->stream.u.dir.cookies);
	fs->root_vnode = vnode;

	hash_insert(fs->vnode_hash, vnode);
	publish_vnode(id, vnode->id, vnode);

	*root_vnid = vnode->id;
	*_fs = fs;
	sDeviceFileSystem = fs;
	return B_OK;

err4:
	hash_uninit(fs->driver_hash);
err3:
	hash_uninit(fs->vnode_hash);
err2:
	recursive_lock_destroy(&fs->lock);
err1:
	free(fs);
err:
	return err;
}


static status_t
devfs_unmount(fs_volume _fs)
{
	struct devfs *fs = (struct devfs *)_fs;
	struct devfs_vnode *vnode;
	struct hash_iterator i;

	TRACE(("devfs_unmount: entry fs = %p\n", fs));

	// release the reference to the root
	put_vnode(fs->id, fs->root_vnode->id);

	// delete all of the vnodes
	hash_open(fs->vnode_hash, &i);
	while ((vnode = (devfs_vnode *)hash_next(fs->vnode_hash, &i)) != NULL) {
		devfs_delete_vnode(fs, vnode, true);
	}
	hash_close(fs->vnode_hash, &i, false);

	hash_uninit(fs->vnode_hash);
	hash_uninit(fs->driver_hash);

	recursive_lock_destroy(&fs->lock);
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
#if 0
		// ToDo: with node monitoring in place, we *know* here that the file does not exist
		//		and don't have to scan for it again.
		// scan for drivers in the given directory (that indicates the type of the driver)
		KPath path;
		if (path.InitCheck() != B_OK)
			return B_NO_MEMORY;

		get_device_name(dir, path.LockBuffer(), path.BufferSize());
		path.UnlockBuffer();
		path.Append(name);
		dprintf("lookup: \"%s\"\n", path.Path());

		// scan for drivers of this type
		probe_for_device_type(path.Path());

		vnode = devfs_find_in_dir(dir, name);
		if (vnode == NULL) {
#endif
			return B_ENTRY_NOT_FOUND;
#if 0
		}

		if (S_ISDIR(vnode->stream.type) && gBootDevice >= 0)
			vnode->stream.u.dir.scanned = true;
#endif
	}

	status = get_vnode(fs->id, vnode->id, (fs_vnode *)&vdummy);
	if (status < B_OK)
		return status;

	*_id = vnode->id;
	*_type = vnode->stream.type;

	return B_OK;
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
	struct devfs_vnode *vnode;

	TRACE(("devfs_get_vnode: asking for vnode id = %Ld, vnode = %p, r %d\n", id, _vnode, reenter));

	if (!reenter)
		recursive_lock_lock(&fs->lock);

	vnode = (devfs_vnode *)hash_lookup(fs->vnode_hash, &id);

	if (!reenter)
		recursive_lock_unlock(&fs->lock);

	TRACE(("devfs_get_vnode: looked it up at %p\n", *_vnode));

	if (vnode == NULL)
		return B_ENTRY_NOT_FOUND;

	*_vnode = vnode;
	return B_OK;
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

	RecursiveLocker locker(&fs->lock);

	if (vnode->dir_next) {
		// can't remove node if it's linked to the dir
		panic("devfs_removevnode: vnode %p asked to be removed is present in dir\n", vnode);
	}

	devfs_delete_vnode(fs, vnode, false);

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

	TRACE(("devfs_create: dir %p, name \"%s\", openMode 0x%x, fs_cookie %p \n", dir, name, openMode, _cookie));

	RecursiveLocker locker(&fs->lock);

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
				&cookie->device_cookie);
		} else {
			char buffer[B_FILE_NAME_LENGTH];
			get_device_name(vnode, buffer, sizeof(buffer));

			status = vnode->stream.u.dev.ops->open(buffer, openMode,
				&cookie->device_cookie);
		}
	}
	if (status < B_OK)
		goto err3;

	*_cookie = cookie;
	return B_OK;

err3:
	free(cookie);
err2:
	put_vnode(fs->id, vnode->id);
err1:
	return status;
}


static status_t
devfs_open(fs_volume _fs, fs_vnode _vnode, int openMode, fs_cookie *_cookie)
{
	struct devfs_vnode *vnode = (struct devfs_vnode *)_vnode;
	struct devfs_cookie *cookie;
	status_t status = B_OK;

	TRACE(("devfs_open: vnode %p, openMode 0x%x, fs_cookie %p \n", vnode, openMode, _cookie));

	cookie = (struct devfs_cookie *)malloc(sizeof(struct devfs_cookie));
	if (cookie == NULL)
		return B_NO_MEMORY;

	if (S_ISCHR(vnode->stream.type)) {
		if (vnode->stream.u.dev.node != NULL) {
			status = vnode->stream.u.dev.info->open(
				vnode->stream.u.dev.node->parent->cookie, openMode,
				&cookie->device_cookie);
		} else {
			char buffer[B_FILE_NAME_LENGTH];
			get_device_name(vnode, buffer, sizeof(buffer));

			status = vnode->stream.u.dev.ops->open(buffer, openMode,
				&cookie->device_cookie);
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
		return vnode->stream.u.dev.info->close(cookie->device_cookie);
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
		vnode->stream.u.dev.info->free(cookie->device_cookie);
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
devfs_read_link(fs_volume _fs, fs_vnode _link, char *buffer, size_t *_bufferSize)
{
	struct devfs_vnode *link = (struct devfs_vnode *)_link;
	size_t bufferSize = *_bufferSize;

	if (!S_ISLNK(link->stream.type))
		return B_BAD_VALUE;

	*_bufferSize = link->stream.u.symlink.length + 1;
		// we always need to return the number of bytes we intend to write!

	if (bufferSize <= link->stream.u.symlink.length)
		return B_BUFFER_OVERFLOW;

	memcpy(buffer, link->stream.u.symlink.path, link->stream.u.symlink.length + 1);
	return B_OK;
}


static status_t
devfs_read(fs_volume _fs, fs_vnode _vnode, fs_cookie _cookie, off_t pos,
	void *buffer, size_t *_length)
{
	struct devfs_vnode *vnode = (struct devfs_vnode *)_vnode;
	struct devfs_cookie *cookie = (struct devfs_cookie *)_cookie;

	//TRACE(("devfs_read: vnode %p, cookie %p, pos %Ld, len %p\n",
	//	vnode, cookie, pos, _length));

	if (!S_ISCHR(vnode->stream.type))
		return B_BAD_VALUE;

	if (vnode->stream.u.dev.partition)
		translate_partition_access(vnode->stream.u.dev.partition, pos, *_length);

	if (*_length == 0)
		return B_OK;

	// if this device has an I/O scheduler attached, the request must go through it
	if (IOScheduler *scheduler = vnode->stream.u.dev.scheduler) {
		IORequest request(cookie->device_cookie, pos, buffer, *_length);

		status_t status = scheduler->Process(request);
		if (status == B_OK)
			*_length = request.Size();

		return status;
	}

	// pass the call through to the device
	return vnode->stream.u.dev.info->read(cookie->device_cookie, pos, buffer, _length);
}


static status_t
devfs_write(fs_volume _fs, fs_vnode _vnode, fs_cookie _cookie, off_t pos,
	const void *buffer, size_t *_length)
{
	struct devfs_vnode *vnode = (struct devfs_vnode *)_vnode;
	struct devfs_cookie *cookie = (struct devfs_cookie *)_cookie;

	//TRACE(("devfs_write: vnode %p, cookie %p, pos %Ld, len %p\n",
	//	vnode, cookie, pos, _length));

	if (!S_ISCHR(vnode->stream.type))
		return B_BAD_VALUE;

	if (vnode->stream.u.dev.partition)
		translate_partition_access(vnode->stream.u.dev.partition, pos, *_length);

	if (*_length == 0)
		return B_OK;

	if (IOScheduler *scheduler = vnode->stream.u.dev.scheduler) {
		IORequest request(cookie->device_cookie, pos, buffer, *_length);

		status_t status = scheduler->Process(request);
		if (status == B_OK)
			*_length = request.Size();

		return status;
	}

	return vnode->stream.u.dev.info->write(cookie->device_cookie, pos, buffer, _length);
}


static status_t
devfs_create_dir(fs_volume _fs, fs_vnode _dir, const char *name,
	int perms, vnode_id *_newVnodeID)
{
	return EROFS;
}


static status_t
devfs_open_dir(fs_volume _fs, fs_vnode _vnode, fs_cookie *_cookie)
{
	struct devfs *fs = (struct devfs *)_fs;
	struct devfs_vnode *vnode = (struct devfs_vnode *)_vnode;
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
devfs_free_dir_cookie(fs_volume _fs, fs_vnode _vnode, fs_cookie _cookie)
{
	struct devfs_vnode *vnode = (struct devfs_vnode *)_vnode;
	struct devfs_dir_cookie *cookie = (devfs_dir_cookie *)_cookie;
	struct devfs *fs = (struct devfs *)_fs;

	TRACE(("devfs_free_dir_cookie: entry vnode %p, cookie %p\n", vnode, cookie));

	RecursiveLocker locker(&fs->lock);

	list_remove_item(&vnode->stream.u.dir.cookies, cookie);
	free(cookie);
	return B_OK;
}


static status_t
devfs_read_dir(fs_volume _fs, fs_vnode _vnode, fs_cookie _cookie,
	struct dirent *dirent, size_t bufferSize, uint32 *_num)
{
	struct devfs_vnode *vnode = (devfs_vnode *)_vnode;
	struct devfs_dir_cookie *cookie = (devfs_dir_cookie *)_cookie;
	struct devfs *fs = (struct devfs *)_fs;
	status_t status = B_OK;
	struct devfs_vnode *childNode = NULL;
	const char *name = NULL;
	struct devfs_vnode *nextChildNode = NULL;
	int32 nextState = cookie->state;

	TRACE(("devfs_read_dir: vnode %p, cookie %p, buffer %p, size %ld\n", _vnode, cookie, dirent, bufferSize));

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

	return B_OK;
}


static status_t
devfs_rewind_dir(fs_volume _fs, fs_vnode _vnode, fs_cookie _cookie)
{
	struct devfs_vnode *vnode = (struct devfs_vnode *)_vnode;
	struct devfs_dir_cookie *cookie = (devfs_dir_cookie *)_cookie;
	struct devfs *fs = (struct devfs *)_fs;

	TRACE(("devfs_rewind_dir: vnode %p, cookie %p\n", _vnode, _cookie));

	if (!S_ISDIR(vnode->stream.type))
		return B_BAD_VALUE;

	RecursiveLocker locker(&fs->lock);

	cookie->current = vnode->stream.u.dir.dir_head;
	cookie->state = ITERATION_STATE_BEGIN;

	return B_OK;
}


/**	Forwards the opcode to the device driver, but also handles some devfs specific
 *	functionality, like partitions.
 */

static status_t
devfs_ioctl(fs_volume _fs, fs_vnode _vnode, fs_cookie _cookie, ulong op,
	void *buffer, size_t length)
{
	struct devfs *fs = (struct devfs *)_fs;
	struct devfs_vnode *vnode = (struct devfs_vnode *)_vnode;
	struct devfs_cookie *cookie = (struct devfs_cookie *)_cookie;

	TRACE(("devfs_ioctl: vnode %p, cookie %p, op %ld, buf %p, len %ld\n",
		_vnode, _cookie, op, buffer, length));

	// we are actually checking for a *device* here, we don't make the distinction
	// between char and block devices
	if (S_ISCHR(vnode->stream.type)) {
		switch (op) {
			case B_GET_GEOMETRY:
			{
				struct devfs_partition *partition = vnode->stream.u.dev.partition;
				if (partition == NULL)
					break;

				device_geometry geometry;
				status_t status = vnode->stream.u.dev.info->control(
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
				const char *path;
				if (!vnode->stream.u.dev.driver)
					return B_ENTRY_NOT_FOUND;
				path = vnode->stream.u.dev.driver->path;
				if (path == NULL)
					return B_ENTRY_NOT_FOUND;

				return user_strlcpy((char *)buffer, path, B_FILE_NAME_LENGTH);
			}

			case B_GET_PARTITION_INFO:
			{
				struct devfs_partition *partition = vnode->stream.u.dev.partition;
				if (!S_ISCHR(vnode->stream.type)
					|| partition == NULL
					|| length != sizeof(partition_info))
					return B_BAD_VALUE;

				return user_memcpy(buffer, &partition->info, sizeof(partition_info));
			}

			case B_SET_PARTITION:
				return B_NOT_ALLOWED;

			case B_GET_PATH_FOR_DEVICE:
			{
				char path[256];
				status_t err;
				/* TODO: we might want to actually find the mountpoint
				 * of that instance of devfs...
				 * but for now we assume it's mounted on /dev
				 */
				strcpy(path, "/dev/");
				get_device_name(vnode, path + 5, sizeof(path) - 5);
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

		return vnode->stream.u.dev.info->control(cookie->device_cookie,
			op, buffer, length);
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

	return vnode->stream.u.dev.info->control(cookie->device_cookie, flags & O_NONBLOCK ?
		B_SET_NONBLOCKING_IO : B_SET_BLOCKING_IO, NULL, 0);
}


static status_t
devfs_select(fs_volume _fs, fs_vnode _vnode, fs_cookie _cookie, uint8 event,
	uint32 ref, selectsync *sync)
{
	struct devfs_vnode *vnode = (struct devfs_vnode *)_vnode;
	struct devfs_cookie *cookie = (struct devfs_cookie *)_cookie;

	if (!S_ISCHR(vnode->stream.type))
		return B_NOT_ALLOWED;

	// If the device has no select() hook, notify select() now.
	if (!vnode->stream.u.dev.info->select)
		return notify_select_event((selectsync*)sync, ref, event);

	return vnode->stream.u.dev.info->select(cookie->device_cookie, event, ref,
		(selectsync*)sync);
}


static status_t
devfs_deselect(fs_volume _fs, fs_vnode _vnode, fs_cookie _cookie, uint8 event,
	selectsync *sync)
{
	struct devfs_vnode *vnode = (struct devfs_vnode *)_vnode;
	struct devfs_cookie *cookie = (struct devfs_cookie *)_cookie;

	if (!S_ISCHR(vnode->stream.type))
		return B_NOT_ALLOWED;

	// If the device has no select() hook, notify select() now.
	if (!vnode->stream.u.dev.info->deselect)
		return B_OK;

	return vnode->stream.u.dev.info->deselect(cookie->device_cookie, event,
		(selectsync*)sync);
}


static bool
devfs_can_page(fs_volume _fs, fs_vnode _vnode, fs_cookie cookie)
{
	struct devfs_vnode *vnode = (devfs_vnode *)_vnode;

	//TRACE(("devfs_canpage: vnode %p\n", vnode));

	if (!S_ISCHR(vnode->stream.type)
		|| vnode->stream.u.dev.node == NULL
		|| cookie == NULL)
		return false;

	return vnode->stream.u.dev.info->read_pages != NULL;
}


static status_t
devfs_read_pages(fs_volume _fs, fs_vnode _vnode, fs_cookie _cookie, off_t pos,
	const iovec *vecs, size_t count, size_t *_numBytes, bool reenter)
{
	struct devfs_vnode *vnode = (devfs_vnode *)_vnode;
	struct devfs_cookie *cookie = (struct devfs_cookie *)_cookie;

	//TRACE(("devfs_read_pages: vnode %p, vecs %p, count = %lu, pos = %Ld, size = %lu\n", vnode, vecs, count, pos, *_numBytes));

	if (!S_ISCHR(vnode->stream.type)
		|| vnode->stream.u.dev.info->read_pages == NULL
		|| cookie == NULL)
		return B_NOT_ALLOWED;

	if (vnode->stream.u.dev.partition)
		translate_partition_access(vnode->stream.u.dev.partition, pos, *_numBytes);

	return vnode->stream.u.dev.info->read_pages(cookie->device_cookie, pos, vecs, count, _numBytes);
}


static status_t
devfs_write_pages(fs_volume _fs, fs_vnode _vnode, fs_cookie _cookie, off_t pos,
	const iovec *vecs, size_t count, size_t *_numBytes, bool reenter)
{
	struct devfs_vnode *vnode = (devfs_vnode *)_vnode;
	struct devfs_cookie *cookie = (struct devfs_cookie *)_cookie;

	//TRACE(("devfs_write_pages: vnode %p, vecs %p, count = %lu, pos = %Ld, size = %lu\n", vnode, vecs, count, pos, *_numBytes));

	if (!S_ISCHR(vnode->stream.type)
		|| vnode->stream.u.dev.info->write_pages == NULL
		|| cookie == NULL)
		return B_NOT_ALLOWED;

	if (vnode->stream.u.dev.partition)
		translate_partition_access(vnode->stream.u.dev.partition, pos, *_numBytes);

	return vnode->stream.u.dev.info->write_pages(cookie->device_cookie, pos, vecs, count, _numBytes);
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

	RecursiveLocker locker(&fs->lock);

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

	notify_stat_changed(fs->id, vnode->id, statMask);
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


file_system_module_info gDeviceFileSystem = {
	{
		"file_systems/devfs" B_CURRENT_FS_API_VERSION,
		0,
		devfs_std_ops,
	},

	"Device File System",

	NULL,	// identify_partition()
	NULL,	// scan_partition()
	NULL,	// free_identify_partition_cookie()
	NULL,	// free_partition_content_cookie()

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
	&devfs_select,
	&devfs_deselect,
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
	&devfs_close,			// same as for files - it does nothing for directories, anyway
	&devfs_free_dir_cookie,
	&devfs_read_dir,
	&devfs_rewind_dir,

	// the other operations are not supported (attributes, indices, queries)
	NULL,
};


//	#pragma mark - device node
//	temporary hack to get it to work with the current device manager


static device_manager_info *sDeviceManager;


static const device_attr pnp_devfs_attrs[] = {
	{ B_DRIVER_MODULE, B_STRING_TYPE, { string: PNP_DEVFS_MODULE_NAME }},
	{ NULL }
};


/** someone registered a device */

static status_t
pnp_devfs_register_device(device_node_handle parent)
{
	char *filename = NULL;
	device_node_handle node;
	status_t status;

	TRACE(("pnp_devfs_probe()\n"));

	if (sDeviceManager->get_attr_string(parent, PNP_DEVFS_FILENAME, &filename, true) != B_OK) {
		dprintf("devfs: Item containing file name is missing\n");
		status = B_ERROR;
		goto err1;
	}

	TRACE(("Adding %s\n", filename));

	status = sDeviceManager->register_device(parent, pnp_devfs_attrs, NULL, &node);
	if (status != B_OK || node == NULL)
		goto err1;

	// ToDo: this is a hack to get things working (init_driver() only works for registered nodes)
	parent->registered = true;

	pnp_devfs_driver_info *info;
	status = sDeviceManager->init_driver(parent, NULL, (driver_module_info **)&info, NULL);
	if (status != B_OK)
		goto err2;

	//add_device(device);
	status = publish_device(sDeviceFileSystem, filename, node, info, NULL, NULL);
	if (status != B_OK)
		goto err3;
	//nudge();

	return B_OK;

err3:
	sDeviceManager->uninit_driver(parent);
err2:
	sDeviceManager->unregister_device(node);
err1:
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
pnp_devfs_device_removed(device_node_handle node, void *cookie)
{
#if 0
	device_info *device;
	device_node_handle parent;
	status_t res = B_OK;
#endif
	TRACE(("pnp_devfs_device_removed()\n"));
#if 0
	parent = sDeviceManager->get_parent(node);
	
	// don't use cookie - we don't use sDeviceManager loading scheme but
	// global data and keep care of everything ourself!
	ACQUIRE_BEN( &device_list_lock );
	
	for( device = devices; device; device = device->next ) {
		if( device->parent == parent ) 
			break;
	}

	if( device != NULL ) {
		pnp_devfs_remove_device(device);
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
			return get_module(B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&sDeviceManager);

		case B_MODULE_UNINIT:
			put_module(B_DEVICE_MANAGER_MODULE_NAME);
			return B_OK;

		default:
			return B_ERROR;
	}
}


driver_module_info gDeviceForDriversModule = {
	{
		PNP_DEVFS_MODULE_NAME,
		0 /*B_KEEP_LOADED*/,
		pnp_devfs_std_ops
	},

	NULL,	// supports device
	pnp_devfs_register_device,
	NULL,	// init driver
	NULL,	// uninit driver
	pnp_devfs_device_removed,
	NULL,	// cleanup
	NULL,	// get paths
};


//	#pragma mark - kernel private API


extern "C" status_t
devfs_add_driver(const char *path)
{
	// see if we already know this driver

	struct stat stat;
	if (::stat(path, &stat) != 0)
		return errno;

	RecursiveLocker locker(&sDeviceFileSystem->lock);

	driver_entry *driver = (driver_entry *)hash_lookup(
		sDeviceFileSystem->driver_hash, &stat.st_ino);
	if (driver != NULL) {
		// we know this driver
		// ToDo: test for changes here? Although node monitoring should be enough.
		if (driver->image < B_OK)
			return driver->image;

		return B_OK;
	}

	// we don't know this driver, create a new entry for it

	driver = (driver_entry *)malloc(sizeof(driver_entry));
	if (driver == NULL)
		return B_NO_MEMORY;

	driver->path = strdup(path);
	if (driver->path == NULL) {
		free(driver);
		return B_NO_MEMORY;
	}

	driver->device = stat.st_dev;
	driver->node = stat.st_ino;
	driver->last_modified = stat.st_mtime;

	hash_insert(sDeviceFileSystem->driver_hash, driver);

	// Even if loading the driver fails - its entry will stay with us
	// so that we don't have to go through it again
	return load_driver(driver);
}


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

	RecursiveLocker locker(&sDeviceFileSystem->lock);

	status = publish_node(sDeviceFileSystem, path, &node);
	if (status != B_OK)
		return status;

	// all went fine, let's initialize the node
	node->stream.type = S_IFLNK | 0644;
	node->stream.u.symlink.path = filePath;
	node->stream.u.symlink.length = strlen(filePath);

	return B_OK;
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
	if (lastPath == NULL || lastDevice == NULL)
		return B_BAD_VALUE;

	size_t length = lastDevice - (lastPath - path) - info->device;
	if (strncmp(path, info->device + length, lastPath - path))
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
devfs_unpublish_device(const char *path, bool disconnect)
{
	devfs_vnode *node;
	status_t status = get_node_for_path(sDeviceFileSystem, path, &node);
	if (status != B_OK)
		return status;

	status = unpublish_node(sDeviceFileSystem, node, S_IFCHR);

	if (status == B_OK && disconnect)
		vfs_disconnect_vnode(sDeviceFileSystem->id, node->id);

	put_vnode(sDeviceFileSystem->id, node->id);
	return status;
}


extern "C" status_t
devfs_publish_device(const char *path, void *obsolete, device_hooks *ops)
{
	return publish_device(sDeviceFileSystem, path, NULL, NULL, NULL, ops);
}


extern "C" status_t
devfs_publish_directory(const char *path)
{
	RecursiveLocker locker(&sDeviceFileSystem->lock);

	return publish_directory(sDeviceFileSystem, path);
}

