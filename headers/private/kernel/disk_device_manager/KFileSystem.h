// KFileSystem.h
//
// KFileSystem implements the KDiskSystem interface for file systems.
// It works with the FS API.

#ifndef _K_FILE_DISK_DEVICE_SYSTEM_H
#define _K_FILE_DISK_DEVICE_SYSTEM_H

#include "KDiskSystem.h"

namespace BPrivate {
namespace DiskDevice {

class KFileSystem {
	KFileSystem(const char *name);
	virtual ~KFileSystem();

	virtual bool IsFileSystem() const;

	virtual status_t Load();		// load/unload -- can be nested
	virtual status_t Unload();		//
	virtual bool IsLoaded() const;

	// Scanning

	virtual float Identify(KPartition *partition, void **cookie);
	virtual status_t Scan(KPartition *partition, void *cookie);
	virtual void FreeIdentifyCookie(KPartition *partition, void *cookie);
	virtual void FreeContentCookie(KPartition *partition);

	// Querying

	virtual bool SupportsDefragmenting(KPartition *partition,
									   bool *whileMounted);
	virtual bool SupportsRepairing(KPartition *partition, bool checkOnly,
								   bool *whileMounted);
	virtual bool SupportsResizing(KPartition *partition, bool *whileMounted);
	virtual bool SupportsMoving(KPartition *partition, bool *whileMounted);
	virtual bool SupportsParentSystem(KDiskSystem *system);

	virtual bool ValidateResize(KPartition *partition, off_t *size);
	virtual bool ValidateMove(KPartition *partition, off_t *start);
	virtual bool ValidateInitialize(KPartition *partition,
									const char *parameters);
	virtual bool ValidateSetContentParameters(KPartition *child,
											  const char *parameters);

	// Writing

	virtual status_t Defragment(KPartition *partition, KDiskDeviceJob *job);
	virtual status_t Repair(KPartition *partition, bool checkOnly,
							KDiskDeviceJob *job);
	virtual status_t Resize(KPartition *partition, off_t size,
							KDiskDeviceJob *job);
	virtual status_t Move(KPartition *partition, off_t offset,
						  KDiskDeviceJob *job);
	virtual status_t Initialize(KPartition *partition, const char *parameters,
								KDiskDeviceJob *job);
	virtual status_t SetContentParameters(KPartition *partition,
										  const char *parameters,
										  KDiskDeviceJob *job);

};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KFileSystem;

#endif	// _K_FILE_DISK_DEVICE_SYSTEM_H
