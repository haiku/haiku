// KDiskDeviceManager.h

#ifndef _K_DISK_DEVICE_MANAGER_H
#define _K_DISK_DEVICE_MANAGER_H

#include "disk_device_manager.h"
#include "List.h"
#include "Locker.h"

namespace BPrivate {
namespace DiskDevice {

class KDiskDevice;
class KDiskDeviceJob;
class KDiskDeviceJobQueue;
class KDiskSystem;
class KPartition;

class KDiskDeviceManager {
public:
	KDiskDeviceManager();
	~KDiskDeviceManager();

	status_t InitCheck() const;

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
	int32 CountDiskDevices();
	KDiskDevice *DiskDeviceAt(int32 index);

	bool PartitionAdded(KPartition *partition);		// implementation internal
	bool PartitionRemoved(KPartition *partition);	//

	// Jobs

	// manager must be locked
	KDiskDeviceJob *JobWithID(disk_job_id id);
	int32 CountJobs();
	KDiskDeviceJob *JobAt(int32 index);

	// manager must be locked
	bool AddJobQueue(KDiskDeviceJobQueue *jobQueue);
	int32 CountJobQueues();
	KDiskDeviceJobQueue *JobQueueAt(int32 index);

	// Disk Systems

	// manager must be locked
	KDiskSystem *DiskSystemWithName(const char *name);
	KDiskSystem *DiskSystemWithID(disk_system_id id);
	int32 CountDiskSystems();
	KDiskSystem *DiskSystemAt(int32 index);

	// Watching

	// TODO: Watching service for the kernel. The userland watching is handled
	// by the registrar.

private:
	KPartition *_FindPartition(partition_id id) const;

	BLocker						fLock;
	List<KDiskDevice*>			fDevices;		// TODO: Optimize!
	List<KPartition*>			fPartitions;	//

	static KDiskDeviceManager	*fDefaultManager;
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KDiskDeviceManager;

#endif	// _K_DISK_DEVICE_MANAGER_H
