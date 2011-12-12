/*
 * Copyright 2002-2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "legacy_drivers.h"

#include <dirent.h>
#include <errno.h>
#include <new>
#include <stdio.h>

#include <FindDirectory.h>
#include <image.h>
#include <NodeMonitor.h>

#include <boot_device.h>
#include <boot/kernel_args.h>
#include <elf.h>
#include <fs/devfs.h>
#include <fs/KPath.h>
#include <fs/node_monitor.h>
#include <Notifications.h>
#include <safemode.h>
#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h>
#include <util/Stack.h>
#include <vfs.h>

#include "AbstractModuleDevice.h"
#include "devfs_private.h"


//#define TRACE_LEGACY_DRIVERS
#ifdef TRACE_LEGACY_DRIVERS
#	define TRACE(x) dprintf x
#else
#	define TRACE(x)
#endif

#define DRIVER_HASH_SIZE 16


namespace {

struct legacy_driver;

class LegacyDevice : public AbstractModuleDevice,
	public DoublyLinkedListLinkImpl<LegacyDevice> {
public:
							LegacyDevice(legacy_driver* driver,
								const char* path, device_hooks* hooks);
	virtual					~LegacyDevice();

			status_t		InitCheck() const;

	virtual	status_t		InitDevice();
	virtual	void			UninitDevice();

	virtual	void			Removed();

			void			SetHooks(device_hooks* hooks);

			legacy_driver*	Driver() const { return fDriver; }
			const char*		Path() const { return fPath; }
			device_hooks*	Hooks() const { return fHooks; }

	virtual	status_t		Open(const char* path, int openMode,
								void** _cookie);
	virtual	status_t		Select(void* cookie, uint8 event, selectsync* sync);

			bool			Republished() const { return fRepublished; }
			void			SetRepublished(bool republished)
								{ fRepublished = republished; }

			void			SetRemovedFromParent(bool removed)
								{ fRemovedFromParent = removed; }

private:
	legacy_driver*			fDriver;
	const char*				fPath;
	device_hooks*			fHooks;
	bool					fRepublished;
	bool					fRemovedFromParent;
};

typedef DoublyLinkedList<LegacyDevice> DeviceList;

struct legacy_driver {
	legacy_driver*	next;
	const char*		path;
	const char*		name;
	dev_t			device;
	ino_t			node;
	timespec		last_modified;
	image_id		image;
	uint32			devices_used;
	bool			binary_updated;
	int32			priority;
	DeviceList		devices;

	// driver image information
	int32			api_version;
	device_hooks*	(*find_device)(const char *);
	const char**	(*publish_devices)(void);
	status_t		(*uninit_driver)(void);
	status_t		(*uninit_hardware)(void);
};


enum driver_event_type {
	kAddDriver,
	kRemoveDriver,
	kAddWatcher,
	kRemoveWatcher
};

struct driver_event : DoublyLinkedListLinkImpl<driver_event> {
	driver_event(driver_event_type _type) : type(_type) {}

	struct ref {
		dev_t		device;
		ino_t		node;
	};

	driver_event_type	type;
	union {
		char			path[B_PATH_NAME_LENGTH];
		ref				node;
	};
};

typedef DoublyLinkedList<driver_event> DriverEventList;


struct driver_entry : DoublyLinkedListLinkImpl<driver_entry> {
	char*			path;
	dev_t			device;
	ino_t			node;
	int32			busses;
};

typedef DoublyLinkedList<driver_entry> DriverEntryList;


struct node_entry : DoublyLinkedListLinkImpl<node_entry> {
};

typedef DoublyLinkedList<node_entry> NodeList;


struct directory_node_entry {
	directory_node_entry*	hash_link;
	ino_t					node;
};

struct DirectoryNodeHashDefinition {
	typedef ino_t* KeyType;
	typedef directory_node_entry ValueType;

	size_t HashKey(ino_t* key) const
		{ return _Hash(*key); }
	size_t Hash(directory_node_entry* entry) const
		{ return _Hash(entry->node); }
	bool Compare(ino_t* key, directory_node_entry* entry) const
		{ return *key == entry->node; }
	directory_node_entry*&
		GetLink(directory_node_entry* entry) const
		{ return entry->hash_link; }

	uint32 _Hash(ino_t node) const
		{ return (uint32)(node >> 32) + (uint32)node; }
};

typedef BOpenHashTable<DirectoryNodeHashDefinition> DirectoryNodeHash;

class DirectoryIterator {
public:
						DirectoryIterator(const char *path,
							const char *subPath = NULL, bool recursive = false);
						~DirectoryIterator();

			void		SetTo(const char *path, const char *subPath = NULL,
							bool recursive = false);

			status_t	GetNext(KPath &path, struct stat &stat);
			const char*	CurrentName() const { return fCurrentName; }

			void		Unset();
			void		AddPath(const char *path, const char *subPath = NULL);

private:
	Stack<KPath*>		fPaths;
	bool				fRecursive;
	DIR*				fDirectory;
	KPath*				fBasePath;
	const char*			fCurrentName;
};


class DirectoryWatcher : public NotificationListener {
public:
						DirectoryWatcher();
	virtual				~DirectoryWatcher();

	virtual void		EventOccurred(NotificationService& service,
							const KMessage* event);
};

class DriverWatcher : public NotificationListener {
public:
						DriverWatcher();
	virtual				~DriverWatcher();

	virtual void		EventOccurred(NotificationService& service,
							const KMessage* event);
};


}	// unnamed namespace


static status_t unload_driver(legacy_driver *driver);
static status_t load_driver(legacy_driver *driver);


static hash_table* sDriverHash;
static DriverWatcher sDriverWatcher;
static int32 sDriverEventsPending;
static DriverEventList sDriverEvents;
static mutex sDriverEventsLock = MUTEX_INITIALIZER("driver events");
	// inner lock, protects the sDriverEvents list only
static DirectoryWatcher sDirectoryWatcher;
static DirectoryNodeHash sDirectoryNodeHash;
static recursive_lock sLock;
static bool sWatching;


//	#pragma mark - driver private


static uint32
driver_entry_hash(void *_driver, const void *_key, uint32 range)
{
	legacy_driver *driver = (legacy_driver *)_driver;
	const char *key = (const char *)_key;

	if (driver != NULL)
		return hash_hash_string(driver->name) % range;

	return hash_hash_string(key) % range;
}


static int
driver_entry_compare(void *_driver, const void *_key)
{
	legacy_driver *driver = (legacy_driver *)_driver;
	const char *key = (const char *)_key;

	return strcmp(driver->name, key);
}


/*!	Collects all published devices of a driver, compares them to what the
	driver would publish now, and then publishes/unpublishes the devices
	as needed.
	If the driver does not publish any devices anymore, it is unloaded.
*/
static status_t
republish_driver(legacy_driver* driver)
{
	if (driver->image < 0) {
		// The driver is not yet loaded - go through the normal load procedure
		return load_driver(driver);
	}

	// mark all devices
	DeviceList::Iterator iterator = driver->devices.GetIterator();
	while (LegacyDevice* device = iterator.Next()) {
		device->SetRepublished(false);
	}

	// now ask the driver for it's currently published devices
	const char** devicePaths = driver->publish_devices();

	int32 exported = 0;
	for (; devicePaths != NULL && devicePaths[0]; devicePaths++) {
		LegacyDevice* device;

		iterator = driver->devices.GetIterator();
		while ((device = iterator.Next()) != NULL) {
			if (!strncmp(device->Path(), devicePaths[0], B_PATH_NAME_LENGTH)) {
				// mark device as republished
				device->SetRepublished(true);
				exported++;
				break;
			}
		}

		device_hooks* hooks = driver->find_device(devicePaths[0]);
		if (hooks == NULL)
			continue;

		if (device != NULL) {
			// update hooks
			device->SetHooks(hooks);
			continue;
		}

		// the device was not present before -> publish it now
		TRACE(("devfs: publishing new device \"%s\"\n", devicePaths[0]));
		device = new(std::nothrow) LegacyDevice(driver, devicePaths[0], hooks);
		if (device != NULL && device->InitCheck() == B_OK
			&& devfs_publish_device(devicePaths[0], device) == B_OK) {
			driver->devices.Add(device);
			exported++;
		} else
			delete device;
	}

	// remove all devices that weren't republished
	iterator = driver->devices.GetIterator();
	while (LegacyDevice* device = iterator.Next()) {
		if (device->Republished())
			continue;

		TRACE(("devfs: unpublishing no more present \"%s\"\n", device->Path()));
		iterator.Remove();
		device->SetRemovedFromParent(true);

		devfs_unpublish_device(device, true);
	}

	if (exported == 0 && driver->devices_used == 0) {
		TRACE(("devfs: driver \"%s\" does not publish any more nodes and is "
			"unloaded\n", driver->path));
		unload_driver(driver);
	}

	return B_OK;
}


static status_t
load_driver(legacy_driver *driver)
{
	status_t (*init_hardware)(void);
	status_t (*init_driver)(void);
	status_t status;

	driver->binary_updated = false;

	// load the module
	image_id image = driver->image;
	if (image < 0) {
		image = load_kernel_add_on(driver->path);
		if (image < 0)
			return image;
	}

	// For a valid device driver the following exports are required

	int32 *apiVersion;
	if (get_image_symbol(image, "api_version", B_SYMBOL_TYPE_DATA,
			(void **)&apiVersion) == B_OK) {
#if B_CUR_DRIVER_API_VERSION != 2
		// just in case someone decides to bump up the api version
#error Add checks here for new vs old api version!
#endif
		if (*apiVersion > B_CUR_DRIVER_API_VERSION) {
			dprintf("devfs: \"%s\" api_version %ld not handled\n", driver->name,
				*apiVersion);
			status = B_BAD_VALUE;
			goto error1;
		}
		if (*apiVersion < 1) {
			dprintf("devfs: \"%s\" api_version invalid\n", driver->name);
			status = B_BAD_VALUE;
			goto error1;
		}

		driver->api_version = *apiVersion;
	} else
		dprintf("devfs: \"%s\" api_version missing\n", driver->name);

	if (get_image_symbol(image, "publish_devices", B_SYMBOL_TYPE_TEXT,
				(void **)&driver->publish_devices) != B_OK
		|| get_image_symbol(image, "find_device", B_SYMBOL_TYPE_TEXT,
				(void **)&driver->find_device) != B_OK) {
		dprintf("devfs: \"%s\" mandatory driver symbol(s) missing!\n",
			driver->name);
		status = B_BAD_VALUE;
		goto error1;
	}

	// Init the driver

	if (get_image_symbol(image, "init_hardware", B_SYMBOL_TYPE_TEXT,
			(void **)&init_hardware) == B_OK
		&& (status = init_hardware()) != B_OK) {
		TRACE(("%s: init_hardware() failed: %s\n", driver->name,
			strerror(status)));
		status = ENXIO;
		goto error1;
	}

	if (get_image_symbol(image, "init_driver", B_SYMBOL_TYPE_TEXT,
			(void **)&init_driver) == B_OK
		&& (status = init_driver()) != B_OK) {
		TRACE(("%s: init_driver() failed: %s\n", driver->name,
			strerror(status)));
		status = ENXIO;
		goto error2;
	}

	// resolve and cache those for the driver unload code
	if (get_image_symbol(image, "uninit_driver", B_SYMBOL_TYPE_TEXT,
		(void **)&driver->uninit_driver) != B_OK)
		driver->uninit_driver = NULL;
	if (get_image_symbol(image, "uninit_hardware", B_SYMBOL_TYPE_TEXT,
		(void **)&driver->uninit_hardware) != B_OK)
		driver->uninit_hardware = NULL;

	// The driver has successfully been initialized, now we can
	// finally publish its device entries

	driver->image = image;
	return republish_driver(driver);

error2:
	if (driver->uninit_hardware)
		driver->uninit_hardware();

error1:
	if (driver->image < 0) {
		unload_kernel_add_on(image);
		driver->image = status;
	}

	return status;
}


static status_t
unload_driver(legacy_driver *driver)
{
	if (driver->image < 0) {
		// driver is not currently loaded
		return B_NO_INIT;
	}

	if (driver->uninit_driver)
		driver->uninit_driver();

	if (driver->uninit_hardware)
		driver->uninit_hardware();

	unload_kernel_add_on(driver->image);
	driver->image = -1;
	driver->binary_updated = false;
	driver->find_device = NULL;
	driver->publish_devices = NULL;
	driver->uninit_driver = NULL;
	driver->uninit_hardware = NULL;

	return B_OK;
}


/*!	Unpublishes all devices belonging to the \a driver. */
static void
unpublish_driver(legacy_driver *driver)
{
	while (LegacyDevice* device = driver->devices.RemoveHead()) {
		device->SetRemovedFromParent(true);
		devfs_unpublish_device(device, true);
	}
}


static void
change_driver_watcher(dev_t device, ino_t node, bool add)
{
	if (device == -1)
		return;

	driver_event* event = new (std::nothrow) driver_event(
		add ? kAddWatcher : kRemoveWatcher);
	if (event == NULL)
		return;

	event->node.device = device;
	event->node.node = node;

	MutexLocker _(sDriverEventsLock);
	sDriverEvents.Add(event);

	atomic_add(&sDriverEventsPending, 1);
}


static int32
get_priority(const char* path)
{
	// TODO: would it be better to initialize a static structure here
	// using find_directory()?
	const directory_which whichPath[] = {
		B_BEOS_DIRECTORY,
		B_COMMON_DIRECTORY,
		B_USER_DIRECTORY
	};
	KPath pathBuffer;

	for (uint32 index = 0; index < sizeof(whichPath) / sizeof(whichPath[0]);
			index++) {
		if (find_directory(whichPath[index], gBootDevice, false,
			pathBuffer.LockBuffer(), pathBuffer.BufferSize()) == B_OK) {
			pathBuffer.UnlockBuffer();
			if (!strncmp(pathBuffer.Path(), path, pathBuffer.BufferSize()))
				return index;
		} else
			pathBuffer.UnlockBuffer();
	}

	return -1;
}


static const char *
get_leaf(const char *path)
{
	const char *name = strrchr(path, '/');
	if (name == NULL)
		return path;

	return name + 1;
}


static legacy_driver *
find_driver(dev_t device, ino_t node)
{
	hash_iterator iterator;
	hash_open(sDriverHash, &iterator);
	legacy_driver *driver;
	while (true) {
		driver = (legacy_driver *)hash_next(sDriverHash, &iterator);
		if (driver == NULL
			|| (driver->device == device && driver->node == node))
			break;
	}

	hash_close(sDriverHash, &iterator, false);
	return driver;
}


static status_t
add_driver(const char *path, image_id image)
{
	// Check if we already know this driver

	struct stat stat;
	if (image >= 0) {
		// The image ID should be a small number and hopefully the boot FS
		// doesn't use small negative values -- if it is inode based, we should
		// be relatively safe.
		stat.st_dev = -1;
		stat.st_ino = -1;
	} else {
		if (::stat(path, &stat) != 0)
			return errno;
	}

	int32 priority = get_priority(path);

	RecursiveLocker _(sLock);

	legacy_driver *driver = (legacy_driver *)hash_lookup(sDriverHash,
		get_leaf(path));
	if (driver != NULL) {
		// we know this driver
		if (strcmp(driver->path, path) != 0) {
			// TODO: do properly, but for now we just update the path if it
			// isn't the same anymore so rescanning of drivers will work in
			// case this driver was loaded so early that it has a boot module
			// path and not a proper driver path
			free((char*)driver->path);
			driver->path = strdup(path);
			driver->name = get_leaf(driver->path);
			driver->binary_updated = true;
		}

		// TODO: check if this driver is a different one and has precendence
		// (ie. common supersedes system).
		//dprintf("new driver has priority %ld, old %ld\n", priority, driver->priority);
		if (priority >= driver->priority) {
			driver->binary_updated = true;
			return B_OK;
		}

		// TODO: test for changes here and/or via node monitoring and reload
		//	the driver if necessary
		if (driver->image < B_OK)
			return driver->image;

		return B_OK;
	}

	// we don't know this driver, create a new entry for it

	driver = (legacy_driver *)malloc(sizeof(legacy_driver));
	if (driver == NULL)
		return B_NO_MEMORY;

	driver->path = strdup(path);
	if (driver->path == NULL) {
		free(driver);
		return B_NO_MEMORY;
	}

	driver->name = get_leaf(driver->path);
	driver->device = stat.st_dev;
	driver->node = stat.st_ino;
	driver->image = image;
	driver->last_modified = stat.st_mtim;
	driver->devices_used = 0;
	driver->binary_updated = false;
	driver->priority = priority;

	driver->api_version = 1;
	driver->find_device = NULL;
	driver->publish_devices = NULL;
	driver->uninit_driver = NULL;
	driver->uninit_hardware = NULL;
	new(&driver->devices) DeviceList;

	hash_insert(sDriverHash, driver);
	if (stat.st_dev > 0)
		change_driver_watcher(stat.st_dev, stat.st_ino, true);

	// Even if loading the driver fails - its entry will stay with us
	// so that we don't have to go through it again
	return load_driver(driver);
}


/*!	This is no longer part of the public kernel API, so we just export the
	symbol
*/
extern "C" status_t load_driver_symbols(const char *driverName);
status_t
load_driver_symbols(const char *driverName)
{
	// This is done globally for the whole kernel via the settings file.
	// We don't have to do anything here.

	return B_OK;
}


static status_t
reload_driver(legacy_driver *driver)
{
	dprintf("devfs: reload driver \"%s\" (%ld, %lld)\n", driver->name,
		driver->device, driver->node);

	unload_driver(driver);

	struct stat stat;
	if (::stat(driver->path, &stat) == 0
		&& (stat.st_dev != driver->device || stat.st_ino != driver->node)) {
		// The driver file has been changed, so we need to update its listener
		change_driver_watcher(driver->device, driver->node, false);

		driver->device = stat.st_dev;
		driver->node = stat.st_ino;

		change_driver_watcher(driver->device, driver->node, true);
	}

	status_t status = load_driver(driver);
	if (status != B_OK)
		unpublish_driver(driver);

	return status;
}


static void
handle_driver_events(void */*_fs*/, int /*iteration*/)
{
	if (atomic_and(&sDriverEventsPending, 0) == 0)
		return;

	// something happened, let's see what it was

	while (true) {
		MutexLocker eventLocker(sDriverEventsLock);

		driver_event* event = sDriverEvents.RemoveHead();
		if (event == NULL)
			break;

		eventLocker.Unlock();
		TRACE(("driver event %p, type %d\n", event, event->type));

		switch (event->type) {
			case kAddDriver:
			{
				// Add new drivers
				RecursiveLocker locker(sLock);
				TRACE(("  add driver %p\n", event->path));

				legacy_driver* driver = (legacy_driver*)hash_lookup(sDriverHash,
					get_leaf(event->path));
				if (driver == NULL)
					legacy_driver_add(event->path);
				else if (get_priority(event->path) >= driver->priority)
					driver->binary_updated = true;
				break;
			}

			case kRemoveDriver:
			{
				// Mark removed drivers as updated
				RecursiveLocker locker(sLock);
				TRACE(("  remove driver %p\n", event->path));

				legacy_driver* driver = (legacy_driver*)hash_lookup(sDriverHash,
					get_leaf(event->path));
				if (driver != NULL
					&& get_priority(event->path) >= driver->priority)
					driver->binary_updated = true;
				break;
			}

			case kAddWatcher:
				TRACE(("  add watcher %ld:%lld\n", event->node.device,
					event->node.node));
				add_node_listener(event->node.device, event->node.node,
					B_WATCH_STAT | B_WATCH_NAME, sDriverWatcher);
				break;

			case kRemoveWatcher:
				TRACE(("  remove watcher %ld:%lld\n", event->node.device,
					event->node.node));
				remove_node_listener(event->node.device, event->node.node,
					sDriverWatcher);
				break;
		}

		delete event;
	}

	// Reload updated drivers

	RecursiveLocker locker(sLock);

	hash_iterator iterator;
	hash_open(sDriverHash, &iterator);
	legacy_driver *driver;
	while (true) {
		driver = (legacy_driver *)hash_next(sDriverHash, &iterator);
		if (driver == NULL)
			break;
		if (!driver->binary_updated || driver->devices_used != 0)
			continue;

		// try to reload the driver
		reload_driver(driver);
	}

	hash_close(sDriverHash, &iterator, false);
	locker.Unlock();
}


//	#pragma mark - DriverWatcher


DriverWatcher::DriverWatcher()
{
}


DriverWatcher::~DriverWatcher()
{
}


void
DriverWatcher::EventOccurred(NotificationService& service,
	const KMessage* event)
{
	int32 opcode = event->GetInt32("opcode", -1);
	if (opcode != B_STAT_CHANGED
		|| (event->GetInt32("fields", 0) & B_STAT_MODIFICATION_TIME) == 0)
		return;

	RecursiveLocker locker(sLock);

	legacy_driver* driver = find_driver(event->GetInt32("device", -1),
		event->GetInt64("node", 0));
	if (driver == NULL)
		return;

	driver->binary_updated = true;

	if (driver->devices_used == 0) {
		// trigger a reload of the driver
		atomic_add(&sDriverEventsPending, 1);
	} else {
		// driver is in use right now
		dprintf("devfs: changed driver \"%s\" is still in use\n", driver->name);
	}
}


static void
dump_driver(legacy_driver* driver)
{
	kprintf("DEVFS DRIVER: %p\n", driver);
	kprintf(" name:           %s\n", driver->name);
	kprintf(" path:           %s\n", driver->path);
	kprintf(" image:          %ld\n", driver->image);
	kprintf(" device:         %ld\n", driver->device);
	kprintf(" node:           %Ld\n", driver->node);
	kprintf(" last modified:  %ld.%lu\n", driver->last_modified.tv_sec,
		driver->last_modified.tv_nsec);
	kprintf(" devs used:      %ld\n", driver->devices_used);
	kprintf(" devs published: %ld\n", driver->devices.Count());
	kprintf(" binary updated: %d\n", driver->binary_updated);
	kprintf(" priority:       %ld\n", driver->priority);
	kprintf(" api version:    %ld\n", driver->api_version);
	kprintf(" hooks:          find_device %p, publish_devices %p\n"
		"                 uninit_driver %p, uninit_hardware %p\n",
		driver->find_device, driver->publish_devices, driver->uninit_driver,
		driver->uninit_hardware);
}


static int
dump_device(int argc, char** argv)
{
	if (argc < 2 || !strcmp(argv[1], "--help")) {
		kprintf("usage: %s [device]\n", argv[0]);
		return 0;
	}

	LegacyDevice* device = (LegacyDevice*)parse_expression(argv[1]);

	kprintf("LEGACY DEVICE: %p\n", device);
	kprintf(" path:     %s\n", device->Path());
	kprintf(" hooks:    %p\n", device->Hooks());
	device_hooks* hooks = device->Hooks();
	kprintf("  close()     %p\n", hooks->close);
	kprintf("  free()      %p\n", hooks->free);
	kprintf("  control()   %p\n", hooks->control);
	kprintf("  read()      %p\n", hooks->read);
	kprintf("  write()     %p\n", hooks->write);
	kprintf("  select()    %p\n", hooks->select);
	kprintf("  deselect()  %p\n", hooks->deselect);
	dump_driver(device->Driver());

	return 0;
}


static int
dump_driver(int argc, char** argv)
{
	if (argc < 2) {
		// print list of all drivers
		kprintf("address    image used publ.   pri name\n");
		hash_iterator iterator;
		hash_open(sDriverHash, &iterator);
		while (true) {
			legacy_driver* driver = (legacy_driver*)hash_next(sDriverHash,
				&iterator);
			if (driver == NULL)
				break;

			kprintf("%p  %5ld %3ld %5ld %c %3ld %s\n", driver,
				driver->image < 0 ? -1 : driver->image,
				driver->devices_used, driver->devices.Count(),
				driver->binary_updated ? 'U' : ' ', driver->priority,
				driver->name);
		}

		hash_close(sDriverHash, &iterator, false);
		return 0;
	}

	if (!strcmp(argv[1], "--help")) {
		kprintf("usage: %s [name]\n", argv[0]);
		return 0;
	}

	legacy_driver* driver = (legacy_driver*)hash_lookup(sDriverHash, argv[1]);
	if (driver == NULL) {
		kprintf("Driver named \"%s\" not found.\n", argv[1]);
		return 0;
	}

	dump_driver(driver);
	return 0;
}


//	#pragma mark -


DirectoryIterator::DirectoryIterator(const char* path, const char* subPath,
		bool recursive)
	:
	fDirectory(NULL),
	fBasePath(NULL),
	fCurrentName(NULL)
{
	SetTo(path, subPath, recursive);
}


DirectoryIterator::~DirectoryIterator()
{
	Unset();
}


void
DirectoryIterator::SetTo(const char* path, const char* subPath, bool recursive)
{
	Unset();
	fRecursive = recursive;

	if (path == NULL) {
		// add default paths
		const directory_which whichPath[] = {
			B_USER_ADDONS_DIRECTORY,
			B_COMMON_ADDONS_DIRECTORY,
			B_BEOS_ADDONS_DIRECTORY
		};
		KPath pathBuffer;

		bool disableUserAddOns = get_safemode_boolean(
			B_SAFEMODE_DISABLE_USER_ADD_ONS, false);

		for (uint32 i = 0; i < sizeof(whichPath) / sizeof(whichPath[0]); i++) {
			if (i < 2 && disableUserAddOns)
				continue;

			if (find_directory(whichPath[i], gBootDevice, true,
					pathBuffer.LockBuffer(), pathBuffer.BufferSize()) == B_OK) {
				pathBuffer.UnlockBuffer();
				pathBuffer.Append("kernel");
				AddPath(pathBuffer.Path(), subPath);
			} else
				pathBuffer.UnlockBuffer();
		}
	} else
		AddPath(path, subPath);
}


status_t
DirectoryIterator::GetNext(KPath& path, struct stat& stat)
{
next_directory:
	while (fDirectory == NULL) {
		delete fBasePath;
		fBasePath = NULL;

		if (!fPaths.Pop(&fBasePath))
			return B_ENTRY_NOT_FOUND;

		fDirectory = opendir(fBasePath->Path());
	}

next_entry:
	struct dirent* dirent = readdir(fDirectory);
	if (dirent == NULL) {
		// get over to next directory on the stack
		closedir(fDirectory);
		fDirectory = NULL;

		goto next_directory;
	}

	if (!strcmp(dirent->d_name, "..") || !strcmp(dirent->d_name, "."))
		goto next_entry;

	fCurrentName = dirent->d_name;

	path.SetTo(fBasePath->Path());
	path.Append(fCurrentName);

	if (::stat(path.Path(), &stat) != 0)
		goto next_entry;

	if (S_ISDIR(stat.st_mode) && fRecursive) {
		KPath *nextPath = new(nothrow) KPath(path);
		if (!nextPath)
			return B_NO_MEMORY;
		if (fPaths.Push(nextPath) != B_OK)
			return B_NO_MEMORY;

		goto next_entry;
	}

	return B_OK;
}


void
DirectoryIterator::Unset()
{
	if (fDirectory != NULL) {
		closedir(fDirectory);
		fDirectory = NULL;
	}

	delete fBasePath;
	fBasePath = NULL;

	KPath *path;
	while (fPaths.Pop(&path))
		delete path;
}


void
DirectoryIterator::AddPath(const char* basePath, const char* subPath)
{
	KPath *path = new(nothrow) KPath(basePath);
	if (!path)
		panic("out of memory");
	if (subPath != NULL)
		path->Append(subPath);

	fPaths.Push(path);
}


//	#pragma mark -


DirectoryWatcher::DirectoryWatcher()
{
}


DirectoryWatcher::~DirectoryWatcher()
{
}


void
DirectoryWatcher::EventOccurred(NotificationService& service,
	const KMessage* event)
{
	int32 opcode = event->GetInt32("opcode", -1);
	dev_t device = event->GetInt32("device", -1);
	ino_t directory = event->GetInt64("directory", -1);
	const char *name = event->GetString("name", NULL);

	if (opcode == B_ENTRY_MOVED) {
		// Determine whether it's a move within, out of, or into one
		// of our watched directories.
		ino_t from = event->GetInt64("from directory", -1);
		ino_t to = event->GetInt64("to directory", -1);
		if (sDirectoryNodeHash.Lookup(&from) == NULL) {
			directory = to;
			opcode = B_ENTRY_CREATED;
		} else if (sDirectoryNodeHash.Lookup(&to) == NULL) {
			directory = from;
			opcode = B_ENTRY_REMOVED;
		} else {
			// Move within, don't do anything for now
			// TODO: adjust driver priority if necessary
			return;
		}
	}

	KPath path(B_PATH_NAME_LENGTH + 1);
	if (path.InitCheck() != B_OK || vfs_entry_ref_to_path(device, directory,
			name, path.LockBuffer(), path.BufferSize()) != B_OK)
		return;

	path.UnlockBuffer();

	dprintf("driver \"%s\" %s\n", path.Leaf(),
		opcode == B_ENTRY_CREATED ? "added" : "removed");

	driver_event* driverEvent = new(std::nothrow) driver_event(
		opcode == B_ENTRY_CREATED ? kAddDriver : kRemoveDriver);
	if (driverEvent == NULL)
		return;

	strlcpy(driverEvent->path, path.Path(), sizeof(driverEvent->path));

	MutexLocker _(sDriverEventsLock);
	sDriverEvents.Add(driverEvent);
	atomic_add(&sDriverEventsPending, 1);
}


//	#pragma mark -


static void
start_watching(const char *base, const char *sub)
{
	KPath path(base);
	path.Append(sub);

	// TODO: create missing directories?
	struct stat stat;
	if (::stat(path.Path(), &stat) != 0)
		return;

	add_node_listener(stat.st_dev, stat.st_ino, B_WATCH_DIRECTORY,
		sDirectoryWatcher);

	directory_node_entry *entry = new(std::nothrow) directory_node_entry;
	if (entry != NULL) {
		entry->node = stat.st_ino;
		sDirectoryNodeHash.Insert(entry);
	}
}


static struct driver_entry*
new_driver_entry(const char* path, dev_t device, ino_t node)
{
	driver_entry* entry = (driver_entry*)malloc(sizeof(driver_entry));
	if (entry == NULL)
		return NULL;

	entry->path = strdup(path);
	if (entry->path == NULL) {
		free(entry);
		return NULL;
	}

	entry->device = device;
	entry->node = node;
	entry->busses = 0;
	return entry;
}


/*!	Iterates over the given list and tries to load all drivers in that list.
	The list is emptied and freed during the traversal.
*/
static status_t
try_drivers(DriverEntryList& list)
{
	while (true) {
		driver_entry* entry = list.RemoveHead();
		if (entry == NULL)
			break;

		image_id image = load_kernel_add_on(entry->path);
		if (image >= 0) {
			// check if it's an old-style driver
			if (legacy_driver_add(entry->path) == B_OK) {
				// we have a driver
				dprintf("loaded driver %s\n", entry->path);
			}

			unload_kernel_add_on(image);
		}

		free(entry->path);
		free(entry);
	}

	return B_OK;
}


static status_t
probe_for_drivers(const char *type)
{
	TRACE(("probe_for_drivers(type = %s)\n", type));

	if (gBootDevice < 0)
		return B_OK;

	DriverEntryList drivers;

	// build list of potential drivers for that type

	DirectoryIterator iterator(NULL, type, false);
	struct stat stat;
	KPath path;

	while (iterator.GetNext(path, stat) == B_OK) {
		if (S_ISDIR(stat.st_mode)) {
			add_node_listener(stat.st_dev, stat.st_ino, B_WATCH_DIRECTORY,
				sDirectoryWatcher);

			directory_node_entry *entry
				= new(std::nothrow) directory_node_entry;
			if (entry != NULL) {
				entry->node = stat.st_ino;
				sDirectoryNodeHash.Insert(entry);
			}

			// We need to make sure that drivers in ie. "audio/raw/" can
			// be found as well - therefore, we must make sure that "audio"
			// exists on /dev.

			size_t length = strlen("drivers/dev");
			if (strncmp(type, "drivers/dev", length))
				continue;

			path.SetTo(type);
			path.Append(iterator.CurrentName());
			devfs_publish_directory(path.Path() + length + 1);
			continue;
		}

		driver_entry *entry = new_driver_entry(path.Path(), stat.st_dev,
			stat.st_ino);
		if (entry == NULL)
			return B_NO_MEMORY;

		TRACE(("found potential driver: %s\n", path.Path()));
		drivers.Add(entry);
	}

	if (drivers.IsEmpty())
		return B_OK;

	// ToDo: do something with the remaining drivers... :)
	try_drivers(drivers);
	return B_OK;
}


//	#pragma mark - LegacyDevice


LegacyDevice::LegacyDevice(legacy_driver* driver, const char* path,
		device_hooks* hooks)
	:
	fDriver(driver),
	fRepublished(true),
	fRemovedFromParent(false)
{
	fDeviceModule = (device_module_info*)malloc(sizeof(device_module_info));
	if (fDeviceModule != NULL)
		memset(fDeviceModule, 0, sizeof(device_module_info));

	fDeviceData = this;
	fPath = strdup(path);

	SetHooks(hooks);
}


LegacyDevice::~LegacyDevice()
{
	free(fDeviceModule);
	free((char*)fPath);
}


status_t
LegacyDevice::InitCheck() const
{
	return fDeviceModule != NULL && fPath != NULL ? B_OK : B_NO_MEMORY;
}


status_t
LegacyDevice::InitDevice()
{
	RecursiveLocker _(sLock);

	if (fInitialized++ > 0)
		return B_OK;

	if (fDriver != NULL && fDriver->devices_used == 0
		&& (fDriver->image < 0 || fDriver->binary_updated)) {
		status_t status = reload_driver(fDriver);
		if (status < B_OK)
			return status;
	}

	if (fDriver != NULL)
		fDriver->devices_used++;

	return B_OK;
}


void
LegacyDevice::UninitDevice()
{
	RecursiveLocker _(sLock);

	if (fInitialized-- > 1)
		return;

	if (fDriver != NULL) {
		if (--fDriver->devices_used == 0 && fDriver->devices.IsEmpty())
			unload_driver(fDriver);
		fDriver = NULL;
	}
}


void
LegacyDevice::Removed()
{
	RecursiveLocker _(sLock);

	if (!fRemovedFromParent && fDriver != NULL)
		fDriver->devices.Remove(this);

	delete this;
}


void
LegacyDevice::SetHooks(device_hooks* hooks)
{
	// TODO: setup compatibility layer!
	fHooks = hooks;

	fDeviceModule->close = hooks->close;
	fDeviceModule->free = hooks->free;
	fDeviceModule->control = hooks->control;
	fDeviceModule->read = hooks->read;
	fDeviceModule->write = hooks->write;

	if (fDriver == NULL || fDriver->api_version >= 2) {
		// According to Be newsletter, vol II, issue 36,
		// version 2 added readv/writev, which we don't support, but also
		// select/deselect.
		if (hooks->select != NULL) {
			// Note we set the module's select to a non-null value to indicate
			// that we have select. HasSelect() will therefore return the
			// correct answer. As Select() is virtual our compatibility
			// version below is going to be called though, that redirects to
			// the proper select hook, so it is ok to set it to an invalid
			// address here.
			fDeviceModule->select = (status_t (*)(void*, uint8, selectsync*))~0;
		}

		fDeviceModule->deselect = hooks->deselect;
	}
}


status_t
LegacyDevice::Open(const char* path, int openMode, void** _cookie)
{
	return Hooks()->open(path, openMode, _cookie);
}


status_t
LegacyDevice::Select(void* cookie, uint8 event, selectsync* sync)
{
	return Hooks()->select(cookie, event, 0, sync);
}


//	#pragma mark - kernel private API


extern "C" void
legacy_driver_add_preloaded(kernel_args* args)
{
	// NOTE: This function does not exit in case of error, since it
	// needs to unload the images then. Also the return code of
	// the path operations is kept separate from the add_driver()
	// success, so that even if add_driver() fails for one driver, it
	// is still tried for the other drivers.
	// NOTE: The initialization success of the path objects is implicitely
	// checked by the immediately following functions.
	KPath basePath;
	status_t status = find_directory(B_BEOS_ADDONS_DIRECTORY,
		gBootDevice, false, basePath.LockBuffer(), basePath.BufferSize());
	if (status != B_OK) {
		dprintf("legacy_driver_add_preloaded: find_directory() failed: "
			"%s\n", strerror(status));
	}
	basePath.UnlockBuffer();
	if (status == B_OK)
		status = basePath.Append("kernel");
	if (status != B_OK) {
		dprintf("legacy_driver_add_preloaded: constructing base driver "
			"path failed: %s\n", strerror(status));
		return;
	}

	struct preloaded_image* image;
	for (image = args->preloaded_images; image != NULL; image = image->next) {
		if (image->is_module || image->id < 0)
			continue;

		KPath imagePath(basePath);
		status = imagePath.Append(image->name);

		// try to add the driver
		TRACE(("legacy_driver_add_preloaded: adding driver %s\n",
			imagePath.Path()));

		if (status == B_OK)
			status = add_driver(imagePath.Path(), image->id);
		if (status != B_OK) {
			dprintf("legacy_driver_add_preloaded: Failed to add \"%s\": %s\n",
				image->name, strerror(status));
			unload_kernel_add_on(image->id);
		}
	}
}


extern "C" status_t
legacy_driver_add(const char* path)
{
	return add_driver(path, -1);
}


extern "C" status_t
legacy_driver_publish(const char *path, device_hooks *hooks)
{
	// we don't have a driver, just publish the hooks
	LegacyDevice* device = new(std::nothrow) LegacyDevice(NULL, path, hooks);
	if (device == NULL)
		return B_NO_MEMORY;

	status_t status = device->InitCheck();
	if (status == B_OK)
		status = devfs_publish_device(path, device);

	if (status != B_OK)
		delete device;

	return status;
}


extern "C" status_t
legacy_driver_rescan(const char* driverName)
{
	RecursiveLocker locker(sLock);

	legacy_driver* driver = (legacy_driver*)hash_lookup(sDriverHash,
		driverName);
	if (driver == NULL)
		return B_ENTRY_NOT_FOUND;

	// Republish the driver's entries
	return republish_driver(driver);
}


extern "C" status_t
legacy_driver_probe(const char* subPath)
{
	TRACE(("legacy_driver_probe(type = %s)\n", subPath));

	char devicePath[64];
	snprintf(devicePath, sizeof(devicePath), "drivers/dev%s%s",
		subPath[0] ? "/" : "", subPath);

	if (!sWatching && gBootDevice > 0) {
		// We're probing the actual boot volume for the first time,
		// let's watch its driver directories for changes
		const directory_which whichPath[] = {
			B_USER_ADDONS_DIRECTORY,
			B_COMMON_ADDONS_DIRECTORY,
			B_BEOS_ADDONS_DIRECTORY
		};
		KPath path;

		new(&sDirectoryWatcher) DirectoryWatcher;

		bool disableUserAddOns = get_safemode_boolean(
			B_SAFEMODE_DISABLE_USER_ADD_ONS, false);

		for (uint32 i = 0; i < sizeof(whichPath) / sizeof(whichPath[0]); i++) {
			if (i < 2 && disableUserAddOns)
				continue;

			if (find_directory(whichPath[i], gBootDevice, true,
					path.LockBuffer(), path.BufferSize()) == B_OK) {
				path.UnlockBuffer();
				path.Append("kernel/drivers");

				start_watching(path.Path(), "dev");
				start_watching(path.Path(), "bin");
			} else
				path.UnlockBuffer();
		}

		sWatching = true;
	}

	return probe_for_drivers(devicePath);
}


extern "C" status_t
legacy_driver_init(void)
{
	legacy_driver dummyDriver;
	sDriverHash = hash_init(DRIVER_HASH_SIZE,
		offset_of_member(dummyDriver, next), &driver_entry_compare,
		&driver_entry_hash);
	if (sDriverHash == NULL)
		return B_NO_MEMORY;

	recursive_lock_init(&sLock, "legacy driver");

	new(&sDriverWatcher) DriverWatcher;
	new(&sDriverEvents) DriverEventList;

	register_kernel_daemon(&handle_driver_events, NULL, 10);
		// once every second

	add_debugger_command("legacy_driver", &dump_driver,
		"info about a legacy driver entry");
	add_debugger_command("legacy_device", &dump_device,
		"info about a legacy device");

	return B_OK;
}
