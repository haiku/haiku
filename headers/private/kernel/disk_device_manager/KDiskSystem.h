// KDiskSystem.h

#ifndef _K_DISK_DEVICE_SYSTEM_H
#define _K_DISK_DEVICE_SYSTEM_H

#include "disk_device_manager.h"

namespace BPrivate {
namespace DiskDevice {

class KDiskDeviceJob;
class KPartition;

class KDiskSystem {
public:
	KDiskSystem(const char *name);
	virtual ~KDiskSystem();

	virtual status_t Init();

	void SetID(disk_system_id id);
	disk_system_id ID() const;
	const char *Name() const;
	virtual const char *PrettyName();

	virtual bool IsFileSystem() const;
	bool IsPartitioningSystem() const;

	// manager will be locked
	status_t Load();		// load/unload -- can be nested
	void Unload();			//
	bool IsLoaded() const;

	// Scanning
	// Device must be write locked.

	virtual float Identify(KPartition *partition, void **cookie);
	virtual status_t Scan(KPartition *partition, void *cookie);
	virtual void FreeIdentifyCookie(KPartition *partition, void *cookie);
	virtual void FreeCookie(KPartition *partition);
	virtual void FreeContentCookie(KPartition *partition);

	// Querying
	// Device must be read locked.

	virtual bool SupportsDefragmenting(KPartition *partition,
									   bool *whileMounted);
	virtual bool SupportsRepairing(KPartition *partition, bool checkOnly,
								   bool *whileMounted);
	virtual bool SupportsResizing(KPartition *partition, bool *whileMounted);
	virtual bool SupportsResizingChild(KPartition *child);
	virtual bool SupportsMoving(KPartition *partition, bool *whileMounted);
	virtual bool SupportsMovingChild(KPartition *child);
	virtual bool SupportsParentSystem(KDiskSystem *system);
	virtual bool SupportsChildSystem(KDiskSystem *system);

	virtual bool ValidateResize(KPartition *partition, off_t *size);
	virtual bool ValidateMove(KPartition *partition, off_t *start);
	virtual bool ValidateResizeChild(KPartition *partition, off_t *size);
	virtual bool ValidateMoveChild(KPartition *partition, off_t *start);
	virtual bool ValidateCreateChild(KPartition *partition, off_t *start,
									 off_t *size, const char *parameters);
	virtual bool ValidateInitialize(KPartition *partition,
									const char *parameters);
	virtual bool ValidateSetParameters(KPartition *partition,
									   const char *parameters);
	virtual bool ValidateSetContentParameters(KPartition *child,
											  const char *parameters);
	virtual int32 CountPartitionableSpaces(KPartition *partition);
	virtual bool GetPartitionableSpaces(KPartition *partition,
										partitionable_space_data *spaces,
										int32 count,
										int32 *actualCount = NULL);

	// Writing
	// Device should not be locked.

	virtual status_t Defragment(KPartition *partition, KDiskDeviceJob *job);
	virtual status_t Repair(KPartition *partition, bool checkOnly,
							KDiskDeviceJob *job);
	virtual status_t Resize(KPartition *partition, off_t size,
							KDiskDeviceJob *job);
	virtual status_t ResizeChild(KPartition *child, off_t size,
								 KDiskDeviceJob *job);
	virtual status_t Move(KPartition *partition, off_t offset,
						  KDiskDeviceJob *job);
	virtual status_t MoveChild(KPartition *child, off_t offset,
							   KDiskDeviceJob *job);
	virtual status_t CreateChild(KPartition *partition, off_t offset,
								 off_t size, const char *parameters,
								 KDiskDeviceJob *job,
								 KPartition **child = NULL,
								 partition_id childID = -1);
	virtual status_t DeleteChild(KPartition *child, KDiskDeviceJob *job);
	virtual status_t Initialize(KPartition *partition, const char *parameters,
								KDiskDeviceJob *job);
	virtual status_t SetParameters(KPartition *partition,
								   const char *parameters,
								   KDiskDeviceJob *job);
	virtual status_t SetContentParameters(KPartition *partition,
										  const char *parameters,
										  KDiskDeviceJob *job);
// The KPartition* parameters for the writing methods are a bit `volatile',
// since the device will not be locked, when they are called. The KPartition
// is registered though, so that it is at least guaranteed that the object
// won't go away.

protected:
	virtual status_t LoadModule();
	virtual void UnloadModule();

	status_t SetPrettyName(const char *name);

private:
	disk_system_id	fID;
	char			*fName;
	char			*fPrettyName;
	int32			fLoadCounter;
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KDiskSystem;

#endif	// _K_DISK_DEVICE_SYSTEM_H
