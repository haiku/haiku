// KDiskDeviceUtils.h

#ifndef _K_DISK_DEVICE_UTILS_H
#define _K_DISK_DEVICE_UTILS_H

#include <stdlib.h>
#include <string.h>

#include <SupportDefs.h>

namespace BPrivate {
namespace DiskDevice {

// helper functions

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


// locking

// AutoLockerStandardLocking
template<typename Lockable>
class AutoLockerStandardLocking {
public:
	inline bool Lock(Lockable *lockable)
	{
		return lockable->Lock();
	}

	inline void Unlock(Lockable *lockable)
	{
		lockable->Unlock();
	}
};

// AutoLockerReadLocking
template<typename Lockable>
class AutoLockerReadLocking {
public:
	inline bool Lock(Lockable *lockable)
	{
		return lockable->ReadLock();
	}

	inline void Unlock(Lockable *lockable)
	{
		lockable->ReadUnlock();
	}
};

// AutoLockerWriteLocking
template<typename Lockable>
class AutoLockerWriteLocking {
public:
	inline bool Lock(Lockable *lockable)
	{
		return lockable->WriteLock();
	}

	inline void Unlock(Lockable *lockable)
	{
		lockable->WriteUnlock();
	}
};

// AutoLocker
template<typename Lockable,
		 typename Locking = AutoLockerStandardLocking<Lockable> >
class AutoLocker {
private:
	typedef AutoLocker<Lockable, Locking>	ThisClass;
public:
	inline AutoLocker(Lockable *lockable, bool alreadyLocked = false)
		: fLockable(lockable),
		  fLocked(fLockable && alreadyLocked)
	{
		if (!fLocked)
			_Lock();
	}

	inline AutoLocker(Lockable &lockable, bool alreadyLocked = false)
		: fLockable(&lockable),
		  fLocked(fLockable && alreadyLocked)
	{
		if (!fLocked)
			_Lock();
	}

	inline ~AutoLocker()
	{
		_Unlock();
	}

	inline AutoLocker<Lockable, Locking> &operator=(Lockable *lockable)
	{
		_Unlock();
		fLockable = lockable;
		_Lock();
	}

	inline AutoLocker<Lockable, Locking> &operator=(Lockable &lockable)
	{
		_Unlock();
		fLockable = &lockable;
		_Lock();
	}

	inline bool IsLocked() const	{ return fLocked; }

	inline operator bool() const	{ return fLocked; }

private:
	inline void _Lock()
	{
		if (fLockable)
			fLocked = fLocking.Lock(fLockable);
	}

	inline void _Unlock()
	{
		if (fLockable && fLocked) {
			fLocking.Unlock(fLockable);
			fLocked = false;
		}
	}

private:
	Lockable	*fLockable;
	bool		fLocked;
	Locking		fLocking;
};

// instantiations
class KDiskDevice;
class KDiskDeviceManager;
class KDiskSystem;
class KPartition;

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

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::set_string;
using BPrivate::DiskDevice::AutoLocker;
using BPrivate::DiskDevice::DeviceReadLocker;
using BPrivate::DiskDevice::DeviceWriteLocker;
using BPrivate::DiskDevice::ManagerLocker;
using BPrivate::DiskDevice::PartitionRegistrar;
using BPrivate::DiskDevice::DiskSystemLoader;

#endif	// _K_DISK_DEVICE_H
