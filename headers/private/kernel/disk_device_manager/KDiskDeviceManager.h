// KDiskDeviceManager.h

#ifndef _K_DISK_DEVICE_MANAGER_H
#define _K_DISK_DEVICE_MANAGER_H

#include "disk_device_manager.h"

namespace BPrivate {
namespace DiskDevice {

class KDiskDevice;
class KDiskDeviceJob;
class KDiskDeviceJobQueue;
class KDiskSystem;
class KPartition;

class KDiskDeviceManager {
	KDiskDeviceManager();
	~KDiskDeviceManager();

	// Singleton Creation, Deletion, and Access

	static status_t CreateDefault();
	static void DeleteDefault();
	static KDiskDeviceManager *Default();

	// Locking

	bool Lock();
	void Unlock();

	// Disk Device / Partition Management

	KDiskDevice *RegisterDevice(const char *path, bool noShadow = true);
	KDiskDevice *RegisterDevice(partition_id id, bool noShadow = true);
	KPartition *RegisterPartition(const char *path, bool noShadow = true);
	KPartition *RegisterPartition(partition_id id, bool noShadow = true);

	// manager must be locked
	int32 CountDiskDevices() const;
	KDiskDevice *DiskDeviceAt(int32 index) const;

	void PartitionAdded(KPartition *partition);		// implementation internal
	void PartitionRemoved(KPartition *partition);	//

	// Jobs

	// manager must be locked
	KDiskDeviceJob *JobWithID(disk_job_id id) const;
	int32 CountJobs() const;
	KDiskDeviceJob *JobAt(int32 index) const;

	// manager must be locked
	bool AddJobQueue(KDiskDeviceJobQueue *jobQueue) const;
	int32 CountJobQueues() const;
	KDiskDeviceJobQueue *JobQueueAt(int32 index) const;

	// Disk Systems

	// manager must be locked
	KDiskSystem *DiskSystemWithName(const char *name);
	KDiskSystem *DiskSystemWithID(disk_system_id id);
	int32 CountDiskSystems() const;
	KDiskSystem *DiskSystemAt(int32 index) const;

	// Watching

	// TODO: Watching service for the kernel. The userland watching is handled
	// by the registrar.
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KDiskDeviceManager;

#endif	// _K_DISK_DEVICE_MANAGER_H
