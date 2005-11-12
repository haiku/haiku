// KDiskDeviceUtils.h

#ifndef _K_DISK_DEVICE_UTILS_H
#define _K_DISK_DEVICE_UTILS_H

#include <stdlib.h>
#include <string.h>

#include <SupportDefs.h>

#include <util/AutoLock.h>

// instantiations
#include <KDiskDevice.h>
#include <KDiskDeviceManager.h>
#include <KDiskSystem.h>
#include <KPartition.h>

namespace BPrivate {
namespace DiskDevice {

// helper functions

// min
template<typename T>
static inline
const T&
min(const T &a, const T &b)
{
	if (a < b)
		return a;
	return b;
}

// max
template<typename T>
static inline
const T&
max(const T &a, const T &b)
{
	if (a < b)
		return b;
	return a;
}

// set_string
static inline
status_t
set_string(char *&location, const char *newValue)
{
	// unset old value
	if (location) {
		free(location);
		location = NULL;
	}
	// set new value
	status_t error = B_OK;
	if (newValue) {
		location = strdup(newValue);
		if (!location)
			error = B_NO_MEMORY;
	}
	return error;
}

// compare_string
/*!	\brief \c NULL aware strcmp().

	\c NULL is considered the least of all strings. \c NULL equals \c NULL.

	\param str1 First string.
	\param str2 Second string.
	\return A value less than 0, if \a str1 is less than \a str2,
			0, if they are equal, or a value greater than 0, if
			\a str1 is greater \a str2.
*/
static inline
int
compare_string(const char *str1, const char *str2)
{
	if (str1 == NULL) {
		if (str2 == NULL)
			return 0;
		return 1;
	} else if (str2 == NULL)
		return -1;
	return strcmp(str1, str2);
}


// locking

// instantiations
class KDiskDevice;
class KDiskDeviceManager;
class KDiskSystem;
class KPartition;
//typedef struct disk_device_data disk_device_data;

typedef AutoLocker<KDiskDevice, AutoLockerReadLocking<KDiskDevice> >
		DeviceReadLocker;
typedef AutoLocker<KDiskDevice, AutoLockerWriteLocking<KDiskDevice> >
		DeviceWriteLocker;
typedef AutoLocker<KDiskDeviceManager > ManagerLocker;

// AutoLockerPartitionRegistration
template<const int dummy = 0>
class AutoLockerPartitionRegistration {
public:
	inline bool Lock(KPartition *partition)
	{
		partition->Register();
		return true;
	}

	inline void Unlock(KPartition *partition)
	{
		partition->Unregister();
	}
};

typedef AutoLocker<KPartition, AutoLockerPartitionRegistration<> >
	PartitionRegistrar;

// AutoLockerDiskSystemLoading
template<const int dummy = 0>
class AutoLockerDiskSystemLoading {
public:
	inline bool Lock(KDiskSystem *diskSystem)
	{
		return (diskSystem->Load() == B_OK);
	}

	inline void Unlock(KDiskSystem *diskSystem)
	{
		diskSystem->Unload();
	}
};

typedef AutoLocker<KDiskSystem, AutoLockerDiskSystemLoading<> >
	DiskSystemLoader;

/*
// AutoLockerDeviceStructReadLocker
template<const int dummy = 0>
class AutoLockerDeviceStructReadLocker {
public:
	inline bool Lock(disk_device_data *device)
	{
		return read_lock_disk_device(device->id);
	}

	inline void Unlock(disk_device_data *device)
	{
		read_unlock_disk_device(device->id);
	}
};

typedef AutoLocker<disk_device_data, AutoLockerDeviceStructReadLocker<> >
	DeviceStructReadLocker;

// AutoLockerDeviceStructWriteLocker
template<const int dummy = 0>
class AutoLockerDeviceStructWriteLocker {
public:
	inline bool Lock(disk_device_data *device)
	{
		return write_lock_disk_device(device->id);
	}

	inline void Unlock(disk_device_data *device)
	{
		write_unlock_disk_device(device->id);
	}
};

typedef AutoLocker<disk_device_data, AutoLockerDeviceStructWriteLocker<> >
	DeviceStructWriteLocker;
*/

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::set_string;
using BPrivate::DiskDevice::DeviceReadLocker;
using BPrivate::DiskDevice::DeviceWriteLocker;
using BPrivate::DiskDevice::ManagerLocker;
using BPrivate::DiskDevice::PartitionRegistrar;
using BPrivate::DiskDevice::DiskSystemLoader;
//using BPrivate::DiskDevice::DeviceStructReadLocker;
//using BPrivate::DiskDevice::DeviceStructReadLocker;

#endif	// _K_DISK_DEVICE_H
