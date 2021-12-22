// ----------------------------------------------------------------------
//  This software is part of the Haiku distribution and is covered
//  by the MIT License.
//
//  File Name:		virtualdrive.c
//
//	Description:	Driver that emulates virtual drives.
//
//	Author:			Marcus Overhagen <Marcus@Overhagen.de>
//					Ingo Weinhold <bonefish@users.sf.net>
//					Axel Doerfler <axeld@pinc-software.de>
// ----------------------------------------------------------------------

#include <fcntl.h>
#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <KernelExport.h>
#include <Drivers.h>
#include <Errors.h>

#include "lock.h"
#include "virtualdrive.h"
#include "virtualdrive_icon.h"

/*
[2:07] <geist> when you open the file in the driver, use stat() to see if it's a file. if it is, call ioctl 10000 on the underlying file
[2:07] <geist> that's the disable-cache ioctl
[2:08] <geist> bfs is probably doing the same algorithm, and seeing that you are a device and not a file, and so it doesn't call 10000 on you
[2:08] <marcus_o> thanks, I will try calling it
[2:08] <geist> and dont bother using dosfs as a host fs, it wont work
[2:09] <geist> bfs is the only fs that's reasonably safe being reentered like that, but only if the underlying one is opened with the cache disabled on that file
[2:09] <geist> that ioctl is used on the swap file as well
[2:10] <marcus_o> I'm currently allocating memory in the driver's write() function hook
[2:10] <geist> cant do that
*/

//#define TRACE(x) dprintf x
#define TRACE(x) ;
#define MB (1024LL * 1024LL)

static int dev_index_for_path(const char *path);

/* -----
	null-terminated array of device names supported by this driver
----- */

static const char *sVirtualDriveName[] = {
	VIRTUAL_DRIVE_DIRECTORY_REL "/0",
	VIRTUAL_DRIVE_DIRECTORY_REL "/1",
	VIRTUAL_DRIVE_DIRECTORY_REL "/2",
	VIRTUAL_DRIVE_DIRECTORY_REL "/3",
	VIRTUAL_DRIVE_DIRECTORY_REL "/4",
	VIRTUAL_DRIVE_DIRECTORY_REL "/5",
	VIRTUAL_DRIVE_DIRECTORY_REL "/6",
	VIRTUAL_DRIVE_DIRECTORY_REL "/7",
	VIRTUAL_DRIVE_DIRECTORY_REL "/8",
	VIRTUAL_DRIVE_DIRECTORY_REL "/9",
	VIRTUAL_DRIVE_CONTROL_DEVICE_REL,
	NULL
};

int32 api_version = B_CUR_DRIVER_API_VERSION;
extern device_hooks sVirtualDriveHooks;

lock driverlock;

typedef struct device_info {
	int32			open_count;
	int				fd;
	off_t			size;
	bool			unused;
	bool			registered;
	char			file[B_PATH_NAME_LENGTH];
	const char		*device_path;
	device_geometry	geometry;
} device_info;

#define kDeviceCount		11
#define kDataDeviceCount	(kDeviceCount - 1)
#define kControlDevice		(kDeviceCount - 1)
struct device_info			gDeviceInfos[kDeviceCount];

static int32		gRegistrationCount	= 0;
static int			gControlDeviceFD	= -1;

static thread_id	gLockOwner			= -1;
static int32		gLockOwnerNesting	= 0;


// lock_driver
void
lock_driver()
{
	thread_id thread = find_thread(NULL);
	if (gLockOwner != thread) {
		LOCK(driverlock);
		gLockOwner = thread;
	}
	gLockOwnerNesting++;
}


// unlock_driver
void
unlock_driver()
{
	thread_id thread = find_thread(NULL);
	if (gLockOwner == thread && --gLockOwnerNesting == 0) {
		gLockOwner = -1;
		UNLOCK(driverlock);
	}
}


// is_valid_device_index
static inline
bool
is_valid_device_index(int32 index)
{
	return (index >= 0 && index < kDeviceCount);
}


// is_valid_data_device_index
static inline
bool
is_valid_data_device_index(int32 index)
{
	return (is_valid_device_index(index) && index != kControlDevice);
}


// dev_index_for_path
static
int
dev_index_for_path(const char *path)
{
	int i;
	for (i = 0; i < kDeviceCount; i++) {
		if (!strcmp(path, gDeviceInfos[i].device_path))
			return i;
	}
	return -1;
}


// clear_device_info
static
void
clear_device_info(int32 index)
{
	TRACE(("virtualdrive: clear_device_info(%ld)\n", index));

	device_info &info = gDeviceInfos[index];
	info.open_count = 0;
	info.fd = -1;
	info.size = 0;
	info.unused = (index != kDeviceCount - 1);
	info.registered = !info.unused;
	info.file[0] = '\0';
	info.device_path = sVirtualDriveName[index];
	info.geometry.read_only = true;
}


// init_device_info
static
status_t
init_device_info(int32 index, virtual_drive_info *initInfo)
{
	if (!is_valid_data_device_index(index) || !initInfo)
		return B_BAD_VALUE;

	device_info &info = gDeviceInfos[index];
	if (!info.unused)
		return B_BAD_VALUE;

	bool readOnly = (initInfo->use_geometry && initInfo->geometry.read_only);

	// open the file
	int fd = open(initInfo->file_name, (readOnly ? O_RDONLY : O_RDWR));
	if (fd < 0)
		return errno;

	status_t error = B_OK;

	// get the file size
	off_t fileSize = 0;
	struct stat st;
	if (fstat(fd, &st) == 0)
		fileSize = st.st_size;
	else
		error = errno;

	// If we shall use the supplied geometry, we enlarge the file, if
	// necessary. Otherwise we fill in the geometry according to the size of the file.
	off_t size = 0;
	if (error == B_OK) {
		if (initInfo->use_geometry) {
			// use the supplied geometry
			info.geometry = initInfo->geometry;
			size = (off_t)info.geometry.bytes_per_sector
				* info.geometry.sectors_per_track
				* info.geometry.cylinder_count
				* info.geometry.head_count;
			if (size > fileSize) {
				if (!readOnly) {
					if (ftruncate(fd, size) != 0)
						error = errno;
				} else
					error = B_NOT_ALLOWED;
			}
		} else {
			// fill in the geometry
			// default to 512 bytes block size
			uint32 blockSize = 512;
			// Optimally we have only 1 block per sector and only one head.
			// Since we have only a uint32 for the cylinder count, this won't work
			// for files > 2TB. So, we set the head count to the minimally possible
			// value.
			off_t blocks = fileSize / blockSize;
			uint32 heads = (blocks + ULONG_MAX - 1) / ULONG_MAX;
			if (heads == 0)
				heads = 1;
			info.geometry.bytes_per_sector = blockSize;
		    info.geometry.sectors_per_track = 1;
		    info.geometry.cylinder_count = blocks / heads;
		    info.geometry.head_count = heads;
		    info.geometry.device_type = B_DISK;	// TODO: Add a new constant.
		    info.geometry.removable = false;
		    info.geometry.read_only = false;
		    info.geometry.write_once = false;
			size = (off_t)info.geometry.bytes_per_sector
				* info.geometry.sectors_per_track
				* info.geometry.cylinder_count
				* info.geometry.head_count;
		}
	}

	if (error == B_OK) {
		// Disable caching for underlying file! (else this driver will deadlock)
		// We probably cannot resize the file once the cache has been disabled!

		// This applies to BeOS only:
		// Work around a bug in BFS: the file is not synced before the cache is
		// turned off, and thus causing possible inconsistencies.
		// Unfortunately, this only solves one half of the issue; there is
		// no way to remove the blocks in the cache, so changes made to the
		// image have the chance to get lost.
		fsync(fd);

		// This is a special reserved ioctl() opcode not defined anywhere in
		// the Be headers.
		if (ioctl(fd, 10000) != 0) {
			dprintf("virtualdrive: disable caching ioctl failed\n");
			return errno;
		}
	}

	// fill in the rest of the device_info structure
	if (error == B_OK) {
		// open_count doesn't have to be changed here (virtualdrive_open() will do that for us)
		info.fd = fd;
		info.size = size;
		info.unused = false;
		info.registered = true;
		strcpy(info.file, initInfo->file_name);
		info.device_path = sVirtualDriveName[index];
	} else {
		// cleanup on error
		close(fd);
		if (info.open_count == 0)
			clear_device_info(index);
	}
	return error;
}


// uninit_device_info
static
status_t
uninit_device_info(int32 index)
{
	if (!is_valid_data_device_index(index))
		return B_BAD_VALUE;

	device_info &info = gDeviceInfos[index];
	if (info.unused)
		return B_BAD_VALUE;

	close(info.fd);
	clear_device_info(index);
	return B_OK;
}


//	#pragma mark -
//	public driver API


status_t
init_hardware(void)
{
	TRACE(("virtualdrive: init_hardware\n"));
	return B_OK;
}


status_t
init_driver(void)
{
	TRACE(("virtualdrive: init\n"));

	new_lock(&driverlock, "virtualdrive lock");

	// init the device infos
	for (int32 i = 0; i < kDeviceCount; i++)
		clear_device_info(i);

	return B_OK;
}


void
uninit_driver(void)
{
	TRACE(("virtualdrive: uninit\n"));
	free_lock(&driverlock);
}


const char **
publish_devices(void)
{
	TRACE(("virtualdrive: publish_devices\n"));
	return sVirtualDriveName;
}


device_hooks *
find_device(const char* name)
{
	TRACE(("virtualdrive: find_device(%s)\n", name));
	return &sVirtualDriveHooks;
}


//	#pragma mark -
//	the device hooks


static status_t
virtualdrive_open(const char *name, uint32 flags, void **cookie)
{
	TRACE(("virtualdrive: open %s\n",name));

	*cookie = (void *)-1;

	lock_driver();

	int32 devIndex = dev_index_for_path(name);

	TRACE(("virtualdrive: devIndex %ld!\n", devIndex));

	if (!is_valid_device_index(devIndex)) {
		TRACE(("virtualdrive: wrong index!\n"));
		unlock_driver();
		return B_ERROR;
	}

	if (gDeviceInfos[devIndex].unused) {
		TRACE(("virtualdrive: device is unused!\n"));
		unlock_driver();
		return B_ERROR;
	}

	if (!gDeviceInfos[devIndex].registered) {
		TRACE(("virtualdrive: device has been unregistered!\n"));
		unlock_driver();
		return B_ERROR;
	}

	// store index in cookie
	*cookie = (void *)devIndex;

	gDeviceInfos[devIndex].open_count++;

	unlock_driver();
	return B_OK;
}


static status_t
virtualdrive_close(void *cookie)
{
	int32 devIndex = (int)cookie;

	TRACE(("virtualdrive: close() devIndex = %ld\n", devIndex));
	if (!is_valid_data_device_index(devIndex))
		return B_OK;

	lock_driver();

	gDeviceInfos[devIndex].open_count--;
	if (gDeviceInfos[devIndex].open_count == 0 && !gDeviceInfos[devIndex].registered) {
		// The last FD is closed and the device has been unregistered. Free its info.
		uninit_device_info(devIndex);
	}

	unlock_driver();

	return B_OK;
}


static status_t
virtualdrive_read(void *cookie, off_t position, void *buffer, size_t *numBytes)
{
	TRACE(("virtualdrive: read pos = 0x%08Lx, bytes = 0x%08lx\n", position, *numBytes));

	// check parameters
	int devIndex = (int)cookie;
	if (devIndex == kControlDevice) {
		TRACE(("virtualdrive: reading from control device not allowed\n"));
		return B_NOT_ALLOWED;
	}
	if (position < 0)
		return B_BAD_VALUE;

	lock_driver();
	device_info &info = gDeviceInfos[devIndex];
	// adjust position and numBytes according to the file size
	if (position > info.size)
		position = info.size;
	if (position + *numBytes > info.size)
		*numBytes = info.size - position;
	// read
	status_t error = B_OK;
	ssize_t bytesRead = read_pos(info.fd, position, buffer, *numBytes);
	if (bytesRead < 0)
		error = errno;
	else
		*numBytes = bytesRead;
	unlock_driver();
	return error;
}


static status_t
virtualdrive_write(void *cookie, off_t position, const void *buffer, size_t *numBytes)
{
	TRACE(("virtualdrive: write pos = 0x%08Lx, bytes = 0x%08lx\n", position, *numBytes));

	// check parameters
	int devIndex = (int)cookie;
	if (devIndex == kControlDevice) {
		TRACE(("virtualdrive: writing to control device not allowed\n"));
		return B_NOT_ALLOWED;
	}
	if (position < 0)
		return B_BAD_VALUE;

	lock_driver();
	device_info &info = gDeviceInfos[devIndex];
	// adjust position and numBytes according to the file size
	if (position > info.size)
		position = info.size;
	if (position + *numBytes > info.size)
		*numBytes = info.size - position;
	// read
	status_t error = B_OK;
	ssize_t bytesRead = write_pos(info.fd, position, buffer, *numBytes);
	if (bytesRead < 0)
		error = errno;
	else
		*numBytes = bytesRead;
	unlock_driver();
	return error;
}


static status_t
virtualdrive_control(void *cookie, uint32 op, void *arg, size_t len)
{
	TRACE(("virtualdrive: ioctl\n"));

	int devIndex = (int)cookie;
	device_info &info = gDeviceInfos[devIndex];

	if (devIndex == kControlDevice || info.unused) {
		// control device or unused data device
		switch (op) {
			case B_GET_DEVICE_SIZE:
			case B_SET_NONBLOCKING_IO:
			case B_SET_BLOCKING_IO:
			case B_GET_READ_STATUS:
			case B_GET_WRITE_STATUS:		
			case B_GET_ICON:
			case B_GET_GEOMETRY:
			case B_GET_BIOS_GEOMETRY:
			case B_GET_MEDIA_STATUS:
			case B_SET_UNINTERRUPTABLE_IO:
			case B_SET_INTERRUPTABLE_IO:
			case B_FLUSH_DRIVE_CACHE:
			case B_GET_BIOS_DRIVE_ID:
			case B_GET_DRIVER_FOR_DEVICE:
			case B_SET_DEVICE_SIZE:
			case B_SET_PARTITION:
			case B_FORMAT_DEVICE:
			case B_EJECT_DEVICE:
			case B_LOAD_MEDIA:
			case B_GET_NEXT_OPEN_DEVICE:
				TRACE(("virtualdrive: another ioctl: %lx (%lu)\n", op, op));
				return B_BAD_VALUE;

			case VIRTUAL_DRIVE_REGISTER_FILE:
			{
				TRACE(("virtualdrive: VIRTUAL_DRIVE_REGISTER_FILE\n"));

				virtual_drive_info *driveInfo = (virtual_drive_info *)arg;
				if (devIndex != kControlDevice || driveInfo == NULL
					|| driveInfo->magic != VIRTUAL_DRIVE_MAGIC
					|| driveInfo->drive_info_size != sizeof(virtual_drive_info))
					return B_BAD_VALUE;

				status_t error = B_ERROR;
				int32 i;

				lock_driver();				

				// first, look if we already have opened that file and see
				// if it's available to us which happens when it has been
				// halted but is still in use by other components
				for (i = 0; i < kDataDeviceCount; i++) {
					if (!gDeviceInfos[i].unused
						&& gDeviceInfos[i].fd == -1
						&& !gDeviceInfos[i].registered
						&& !strcmp(gDeviceInfos[i].file, driveInfo->file_name)) {
						// mark device as unused, so that init_device_info() will succeed
						gDeviceInfos[i].unused = true;
						error = B_OK;
						break;
					}
				}

				if (error != B_OK) {
					// find an unused data device
					for (i = 0; i < kDataDeviceCount; i++) {
						if (gDeviceInfos[i].unused) {
							error = B_OK;
							break;
						}
					}
				}

				if (error == B_OK) {
					// we found a device slot, let's initialize it
					error = init_device_info(i, driveInfo);
					if (error == B_OK) {
						// return the device path
						strcpy(driveInfo->device_name, "/dev/");
						strcat(driveInfo->device_name, gDeviceInfos[i].device_path);

						// on the first registration we need to open the
						// control device to stay loaded
						if (gRegistrationCount++ == 0) {
							char path[B_PATH_NAME_LENGTH];
							strcpy(path, "/dev/");
							strcat(path, info.device_path);
							gControlDeviceFD = open(path, O_RDONLY);
						}
					}
				}

				unlock_driver();
				return error;
			}
			case VIRTUAL_DRIVE_UNREGISTER_FILE:
			case VIRTUAL_DRIVE_GET_INFO:
				TRACE(("virtualdrive: VIRTUAL_DRIVE_UNREGISTER_FILE/"
					  "VIRTUAL_DRIVE_GET_INFO on control device\n"));
				// these are called on used data files only!
				return B_BAD_VALUE;

			default:
				TRACE(("virtualdrive: unknown ioctl: %lx (%lu)\n", op, op));
				return B_BAD_VALUE;
		}
	} else {
		// used data device
		switch (op) {
			case B_GET_DEVICE_SIZE:
				TRACE(("virtualdrive: B_GET_DEVICE_SIZE\n"));
				*(size_t*)arg = info.size;
				return B_OK;

			case B_SET_NONBLOCKING_IO:
				TRACE(("virtualdrive: B_SET_NONBLOCKING_IO\n"));
				return B_OK;

			case B_SET_BLOCKING_IO:
				TRACE(("virtualdrive: B_SET_BLOCKING_IO\n"));
				return B_OK;

			case B_GET_READ_STATUS:
				TRACE(("virtualdrive: B_GET_READ_STATUS\n"));
				*(bool*)arg = true;
				return B_OK;

			case B_GET_WRITE_STATUS:		
				TRACE(("virtualdrive: B_GET_WRITE_STATUS\n"));
				*(bool*)arg = true;
				return B_OK;

			case B_GET_ICON:
			{
				TRACE(("virtualdrive: B_GET_ICON\n"));
				device_icon *icon = (device_icon *)arg;

				if (icon->icon_size == kPrimaryImageWidth) {
					memcpy(icon->icon_data, kPrimaryImageBits, kPrimaryImageWidth * kPrimaryImageHeight);
				} else if (icon->icon_size == kSecondaryImageWidth) {
					memcpy(icon->icon_data, kSecondaryImageBits, kSecondaryImageWidth * kSecondaryImageHeight);
				} else
					return B_ERROR;

				return B_OK;
			}

			case B_GET_GEOMETRY:
				TRACE(("virtualdrive: B_GET_GEOMETRY\n"));
				*(device_geometry *)arg = info.geometry;
				return B_OK;

			case B_GET_BIOS_GEOMETRY:
			{
				TRACE(("virtualdrive: B_GET_BIOS_GEOMETRY\n"));
				device_geometry *dg = (device_geometry *)arg;
				dg->bytes_per_sector = 512;
				dg->sectors_per_track = info.size / (512 * 1024);
				dg->cylinder_count = 1024;
				dg->head_count = 1;
				dg->device_type = info.geometry.device_type;
				dg->removable = info.geometry.removable;
				dg->read_only = info.geometry.read_only;
				dg->write_once = info.geometry.write_once;
				return B_OK;
			}

			case B_GET_MEDIA_STATUS:
				TRACE(("virtualdrive: B_GET_MEDIA_STATUS\n"));
				*(status_t*)arg = B_NO_ERROR;
				return B_OK;

			case B_SET_UNINTERRUPTABLE_IO:
				TRACE(("virtualdrive: B_SET_UNINTERRUPTABLE_IO\n"));
				return B_OK;

			case B_SET_INTERRUPTABLE_IO:
				TRACE(("virtualdrive: B_SET_INTERRUPTABLE_IO\n"));
				return B_OK;

			case B_FLUSH_DRIVE_CACHE:
				TRACE(("virtualdrive: B_FLUSH_DRIVE_CACHE\n"));
				return B_OK;

			case B_GET_BIOS_DRIVE_ID:
				TRACE(("virtualdrive: B_GET_BIOS_DRIVE_ID\n"));
				*(uint8*)arg = 0xF8;
				return B_OK;

			case B_GET_DRIVER_FOR_DEVICE:
			case B_SET_DEVICE_SIZE:
			case B_SET_PARTITION:
			case B_FORMAT_DEVICE:
			case B_EJECT_DEVICE:
			case B_LOAD_MEDIA:
			case B_GET_NEXT_OPEN_DEVICE:
				TRACE(("virtualdrive: another ioctl: %lx (%lu)\n", op, op));
				return B_BAD_VALUE;

			case VIRTUAL_DRIVE_REGISTER_FILE:
				TRACE(("virtualdrive: VIRTUAL_DRIVE_REGISTER_FILE (data)\n"));
				return B_BAD_VALUE;
			case VIRTUAL_DRIVE_UNREGISTER_FILE:
			{
				TRACE(("virtualdrive: VIRTUAL_DRIVE_UNREGISTER_FILE\n"));
				lock_driver();

				bool immediately = (bool)arg;
				bool wasRegistered = info.registered;

				info.registered = false;

				// on the last unregistration we need to close the
				// control device
				if (wasRegistered && --gRegistrationCount == 0) {
					close(gControlDeviceFD);
					gControlDeviceFD = -1;
				}

				// if we "immediately" is true, we will stop our service immediately
				// and close the underlying file, open it for other uses
				if (immediately) {
					TRACE(("virtualdrive: close file descriptor\n"));
					// we cannot use uninit_device_info() here, since that does
					// a little too much and would open the device for other
					// uses.
					close(info.fd);
					info.fd = -1;
				}

				unlock_driver();
				return B_OK;
			}
			case VIRTUAL_DRIVE_GET_INFO:
			{
				TRACE(("virtualdrive: VIRTUAL_DRIVE_GET_INFO\n"));

				virtual_drive_info *driveInfo = (virtual_drive_info *)arg;
				if (driveInfo == NULL
					|| driveInfo->magic != VIRTUAL_DRIVE_MAGIC
					|| driveInfo->drive_info_size != sizeof(virtual_drive_info))
					return B_BAD_VALUE;

				strcpy(driveInfo->file_name, info.file);
				strcpy(driveInfo->device_name, "/dev/");
				strcat(driveInfo->device_name, info.device_path);
				driveInfo->geometry = info.geometry;
				driveInfo->use_geometry = true;
				driveInfo->halted = info.fd == -1;
				return B_OK;
			}

			default:
				TRACE(("virtualdrive: unknown ioctl: %lx (%lu)\n", op, op));
				return B_BAD_VALUE;
		}
	}

}


static status_t
virtualdrive_free(void *cookie)
{
	TRACE(("virtualdrive: free cookie()\n"));
	return B_OK;
}


/* -----
	function pointers for the device hooks entry points
----- */

device_hooks sVirtualDriveHooks = {
	virtualdrive_open, 			/* -> open entry point */
	virtualdrive_close, 		/* -> close entry point */
	virtualdrive_free,			/* -> free cookie */
	virtualdrive_control, 		/* -> control entry point */
	virtualdrive_read,			/* -> read entry point */
	virtualdrive_write			/* -> write entry point */
};

