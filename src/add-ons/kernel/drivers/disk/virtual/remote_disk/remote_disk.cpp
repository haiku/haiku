/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <string.h>

#include <KernelExport.h>
#include <Drivers.h>

#include <lock.h>
#include <util/AutoLock.h>
#include <util/kernel_cpp.h>

#include "RemoteDisk.h"


//#define TRACE_REMOTE_DISK
#ifdef TRACE_REMOTE_DISK
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) do {} while (false)
#endif


const bigtime_t kInitRetryDelay	= 10 * 1000000LL;	// 10 s

enum {
	MAX_REMOTE_DISKS	= 1
};


struct RemoteDiskDevice : recursive_lock {
	RemoteDisk*		remoteDisk;
	bigtime_t		lastInitRetryTime;

	RemoteDiskDevice()
		:
		remoteDisk(NULL),
		lastInitRetryTime(-1)
	{
	}

	~RemoteDiskDevice()
	{
		delete remoteDisk;
		Uninit();
	}

	status_t Init()
	{
		recursive_lock_init(this, "remote disk device");
		return B_OK;
	}

	void Uninit()
	{
		recursive_lock_destroy(this);
	}

	status_t LazyInitDisk()
	{
		if (remoteDisk)
			return B_OK;

		// don't try to init, if the last attempt wasn't long enough ago
		if (lastInitRetryTime >= 0
			&& system_time() < lastInitRetryTime + kInitRetryDelay) {
			return B_ERROR;
		}

		// create the object
		remoteDisk = new(nothrow) RemoteDisk;
		if (!remoteDisk) {
			lastInitRetryTime = system_time();
			return B_NO_MEMORY;
		}

		// find a server
		TRACE(("remote_disk: FindAnyRemoteDisk()\n"));
		status_t error = remoteDisk->FindAnyRemoteDisk();
		if (error != B_OK) {
			delete remoteDisk;
			remoteDisk = NULL;
			lastInitRetryTime = system_time();
			return B_NO_MEMORY;
		}

		return B_OK;
	}

	void GetGeometry(device_geometry* geometry, bool bios)
	{
		// TODO: Respect "bios" argument!
		geometry->bytes_per_sector = REMOTE_DISK_BLOCK_SIZE;
		geometry->sectors_per_track = 1;
		geometry->cylinder_count = remoteDisk->Size() / REMOTE_DISK_BLOCK_SIZE;
		geometry->head_count = 1;
		geometry->device_type = B_DISK;
		geometry->removable = true;
		geometry->read_only = remoteDisk->IsReadOnly();
		geometry->write_once = false;
	}
};

typedef RecursiveLocker DeviceLocker;


static const char* kPublishedNames[] = {
	"disk/virtual/remote_disk/0/raw",
//	"misc/remote_disk_control",
	NULL
};

static RemoteDiskDevice* sDevices;


// #pragma mark - internal functions


// device_for_name
static RemoteDiskDevice*
device_for_name(const char* name)
{
	for (int i = 0; i < MAX_REMOTE_DISKS; i++) {
		if (strcmp(name, kPublishedNames[i]) == 0)
			return sDevices + i;
	}
	return NULL;
}


// #pragma mark - data device hooks


static status_t
remote_disk_open(const char* name, uint32 flags, void** cookie)
{
	RemoteDiskDevice* device = device_for_name(name);
	TRACE(("remote_disk_open(\"%s\") -> %p\n", name, device));
	if (!device)
		return B_BAD_VALUE;

	DeviceLocker locker(device);
	status_t error = device->LazyInitDisk();
	if (error != B_OK)
		return error;

	*cookie = device;

	return B_OK;
}


static status_t
remote_disk_close(void* cookie)
{
	TRACE(("remote_disk_close(%p)\n", cookie));

	// nothing to do
	return B_OK;
}


static status_t
remote_disk_read(void* cookie, off_t position, void* buffer, size_t* numBytes)
{
	TRACE(("remote_disk_read(%p, %lld, %p, %lu)\n", cookie, position, buffer,
		*numBytes));

	RemoteDiskDevice* device = (RemoteDiskDevice*)cookie;
	DeviceLocker locker(device);

	ssize_t bytesRead = device->remoteDisk->ReadAt(position, buffer, *numBytes);
	if (bytesRead < 0) {
		*numBytes = 0;
		TRACE(("remote_disk_read() failed: %s\n", strerror(bytesRead)));
		return bytesRead;
	}

	*numBytes = bytesRead;
	TRACE(("remote_disk_read() done: %ld\n", bytesRead));
	return B_OK;
}


static status_t
remote_disk_write(void* cookie, off_t position, const void* buffer,
	size_t* numBytes)
{
	TRACE(("remote_disk_write(%p, %lld, %p, %lu)\n", cookie, position, buffer,
		*numBytes));

	RemoteDiskDevice* device = (RemoteDiskDevice*)cookie;
	DeviceLocker locker(device);

	ssize_t bytesWritten = device->remoteDisk->WriteAt(position, buffer,
		*numBytes);
	if (bytesWritten < 0) {
		*numBytes = 0;
		TRACE(("remote_disk_write() failed: %s\n", strerror(bytesRead)));
		return bytesWritten;
	}

	*numBytes = bytesWritten;
	TRACE(("remote_disk_written() done: %ld\n", bytesWritten));
	return B_OK;
}


static status_t
remote_disk_control(void* cookie, uint32 op, void* arg, size_t len)
{
	TRACE(("remote_disk_control(%p, %lu, %p, %lu)\n", cookie, op, arg, len));

	RemoteDiskDevice* device = (RemoteDiskDevice*)cookie;
	DeviceLocker locker(device);

	// used data device
	switch (op) {
		case B_GET_DEVICE_SIZE:
			TRACE(("remote_disk: B_GET_DEVICE_SIZE\n"));
			*(size_t*)arg = device->remoteDisk->Size();
			return B_OK;

		case B_SET_NONBLOCKING_IO:
			TRACE(("remote_disk: B_SET_NONBLOCKING_IO\n"));
			return B_OK;

		case B_SET_BLOCKING_IO:
			TRACE(("remote_disk: B_SET_BLOCKING_IO\n"));
			return B_OK;

		case B_GET_READ_STATUS:
			TRACE(("remote_disk: B_GET_READ_STATUS\n"));
			*(bool*)arg = true;
			return B_OK;

		case B_GET_WRITE_STATUS:
			TRACE(("remote_disk: B_GET_WRITE_STATUS\n"));
			*(bool*)arg = true;
			return B_OK;

		case B_GET_ICON:
		{
			TRACE(("remote_disk: B_GET_ICON\n"));
			return B_BAD_VALUE;
		}

		case B_GET_GEOMETRY:
			TRACE(("remote_disk: B_GET_GEOMETRY\n"));
			device->GetGeometry((device_geometry*)arg, false);
			return B_OK;

		case B_GET_BIOS_GEOMETRY:
		{
			TRACE(("remote_disk: B_GET_BIOS_GEOMETRY\n"));
			device->GetGeometry((device_geometry*)arg, true);
			return B_OK;
		}

		case B_GET_MEDIA_STATUS:
			TRACE(("remote_disk: B_GET_MEDIA_STATUS\n"));
			*(status_t*)arg = B_NO_ERROR;
			return B_OK;

		case B_SET_UNINTERRUPTABLE_IO:
			TRACE(("remote_disk: B_SET_UNINTERRUPTABLE_IO\n"));
			return B_OK;

		case B_SET_INTERRUPTABLE_IO:
			TRACE(("remote_disk: B_SET_INTERRUPTABLE_IO\n"));
			return B_OK;

		case B_FLUSH_DRIVE_CACHE:
			TRACE(("remote_disk: B_FLUSH_DRIVE_CACHE\n"));
			return B_OK;

		case B_GET_BIOS_DRIVE_ID:
			TRACE(("remote_disk: B_GET_BIOS_DRIVE_ID\n"));
			*(uint8*)arg = 0xF8;
			return B_OK;

		case B_GET_DRIVER_FOR_DEVICE:
		case B_SET_DEVICE_SIZE:
		case B_SET_PARTITION:
		case B_FORMAT_DEVICE:
		case B_EJECT_DEVICE:
		case B_LOAD_MEDIA:
		case B_GET_NEXT_OPEN_DEVICE:
			TRACE(("remote_disk: another ioctl: %lx (%lu)\n", op, op));
			return B_BAD_VALUE;

		default:
			TRACE(("remote_disk: unknown ioctl: %lx (%lu)\n", op, op));
			return B_BAD_VALUE;
	}
}


static status_t
remote_disk_free(void* cookie)
{
	TRACE(("remote_disk_free(%p)\n", cookie));

	// nothing to do
	return B_OK;
}


static device_hooks sDataDeviceHooks = {
	remote_disk_open,
	remote_disk_close,
	remote_disk_free,
	remote_disk_control,
	remote_disk_read,
	remote_disk_write
};


// #pragma mark - public API


int32 api_version = B_CUR_DRIVER_API_VERSION;


status_t
init_hardware(void)
{
	TRACE(("remote_disk: init_hardware()\n"));

	return B_OK;
}


status_t
init_driver(void)
{
	TRACE(("remote_disk: init_driver()\n"));

	sDevices = new(nothrow) RemoteDiskDevice[MAX_REMOTE_DISKS];
	if (!sDevices)
		return B_NO_MEMORY;

	status_t error = B_OK;
	for (int i = 0; error == B_OK && i < MAX_REMOTE_DISKS; i++)
		error = sDevices[i].Init();

	if (error != B_OK) {
		delete[] sDevices;
		sDevices = NULL;
		return error;
	}

	return B_OK;
}


void
uninit_driver(void)
{
	TRACE(("remote_disk: uninit_driver()\n"));

	delete[] sDevices;
}


const char**
publish_devices(void)
{
	TRACE(("remote_disk: publish_devices()\n"));
	return kPublishedNames;
}


device_hooks*
find_device(const char* name)
{
	TRACE(("remote_disk: find_device(%s)\n", name));
	return &sDataDeviceHooks;
}

