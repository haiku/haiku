// KDiskDeviceManager.cpp

#include <stdlib.h>
#include <string.h>

#include "KDiskDevice.h"
#include "KDiskDeviceManager.h"
#include "KDiskDeviceUtils.h"
#include "KPartition.h"

// TODO: Remove when not longer needed.
using BPrivate::DiskDevice::KDiskDeviceJob;
using BPrivate::DiskDevice::KDiskDeviceJobQueue;
using BPrivate::DiskDevice::KDiskSystem;

// constructor
KDiskDeviceManager::KDiskDeviceManager()
	: fLock("disk device manager"),
	  fDevices(20),
	  fPartitions(100)
{
}

// destructor
KDiskDeviceManager::~KDiskDeviceManager()
{
}

// InitCheck
status_t
KDiskDeviceManager::InitCheck() const
{
	return B_OK;
}

// CreateDefault
status_t
KDiskDeviceManager::CreateDefault()
{
	status_t error = B_OK;
	if (!fDefaultManager) {
		fDefaultManager = new(nothrow) KDiskDeviceManager;
		if (fDefaultManager) {
			error = fDefaultManager->InitCheck();
			if (error != B_OK)
				DeleteDefault();
		} else
			error = B_NO_MEMORY;
	}
	return (fDefaultManager ? B_OK : B_NO_MEMORY);
}

// DeleteDefault
void
KDiskDeviceManager::DeleteDefault()
{
	if (fDefaultManager) {
		delete fDefaultManager;
		fDefaultManager = NULL;
	}
}

// Default
KDiskDeviceManager *
KDiskDeviceManager::Default()
{
	return fDefaultManager;
}

// Lock
bool
KDiskDeviceManager::Lock()
{
	return fLock.Lock();
}

// Unlock
void
KDiskDeviceManager::Unlock()
{
	fLock.Unlock();
}

// RegisterDevice
KDiskDevice *
KDiskDeviceManager::RegisterDevice(const char *path, bool noShadow)
{
// TODO: Handle shadows correctly!
	if (ManagerLocker locker = this) {
		for (int32 i = 0; KDiskDevice *device = fDevices.ItemAt(i); i++) {
			if (device->Path() && !strcmp(path, device->Path())) {
				device->Register();
				return device;
			}
		}
	}
	return NULL;
}

// RegisterDevice
KDiskDevice *
KDiskDeviceManager::RegisterDevice(partition_id id, bool noShadow)
{
// TODO: Handle shadows correctly!
	if (ManagerLocker locker = this) {
		if (KPartition *partition = _FindPartition(id)) {
			KDiskDevice *device = partition->Device();
			device->Register();
			return device;
		}
	}
	return NULL;
}

// RegisterPartition
KPartition *
KDiskDeviceManager::RegisterPartition(const char *path, bool noShadow)
{
// TODO: Handle shadows correctly!
// TODO: Optimize!
	if (ManagerLocker locker = this) {
		for (int32 i = 0; KPartition *partition = fPartitions.ItemAt(i); i++) {
			char partitionPath[B_PATH_NAME_LENGTH];
			if (partition->GetPath(partitionPath) == B_OK
				&& !strcmp(path, partitionPath)) {
				partition->Register();
				return partition;
			}
		}
	}
	return NULL;
}

// RegisterPartition
KPartition *
KDiskDeviceManager::RegisterPartition(partition_id id, bool noShadow)
{
// TODO: Handle shadows correctly!
	if (ManagerLocker locker = this) {
		if (KPartition *partition = _FindPartition(id)) {
			partition->Register();
			return partition;
		}
	}
	return NULL;
}

// CountDiskDevices
int32
KDiskDeviceManager::CountDiskDevices()
{
	if (ManagerLocker locker = this)
		return fDevices.CountItems();
	return 0;
}

// DiskDeviceAt
KDiskDevice *
KDiskDeviceManager::DiskDeviceAt(int32 index)
{
	if (ManagerLocker locker = this)
		return fDevices.ItemAt(index);
	return 0;
}

// PartitionAdded
bool
KDiskDeviceManager::PartitionAdded(KPartition *partition)
{
	return (partition && fPartitions.AddItem(partition));
}

// PartitionRemoved
bool
KDiskDeviceManager::PartitionRemoved(KPartition *partition)
{
	return (partition && fPartitions.RemoveItem(partition));
}

// JobWithID
KDiskDeviceJob *
KDiskDeviceManager::JobWithID(disk_job_id id)
{
	// not implemented
	return NULL;
}

// CountJobs
int32
KDiskDeviceManager::CountJobs()
{
	// not implemented
	return 0;
}

// JobAt
KDiskDeviceJob *
KDiskDeviceManager::JobAt(int32 index)
{
	// not implemented
	return NULL;
}

// AddJobQueue
bool
KDiskDeviceManager::AddJobQueue(KDiskDeviceJobQueue *jobQueue)
{
	// not implemented
	return false;
}

// CountJobQueues
int32
KDiskDeviceManager::CountJobQueues()
{
	// not implemented
	return 0;
}

// JobQueueAt
KDiskDeviceJobQueue *
KDiskDeviceManager::JobQueueAt(int32 index)
{
	// not implemented
	return NULL;
}

// DiskSystemWithName
KDiskSystem *
KDiskDeviceManager::DiskSystemWithName(const char *name)
{
	// not implemented
	return NULL;
}

// DiskSystemWithID
KDiskSystem *
KDiskDeviceManager::DiskSystemWithID(disk_system_id id)
{
	// not implemented
	return NULL;
}

// CountDiskSystems
int32
KDiskDeviceManager::CountDiskSystems()
{
	// not implemented
	return 0;
}

// DiskSystemAt
KDiskSystem *
KDiskDeviceManager::DiskSystemAt(int32 index)
{
	// not implemented
	return NULL;
}

// _FindPartition
KPartition *
KDiskDeviceManager::_FindPartition(partition_id id) const
{
	for (int32 i = 0; KPartition *partition = fPartitions.ItemAt(i); i++) {
		if (partition->ID() == id)
			return partition;
	}
}

// singleton instance
KDiskDeviceManager *KDiskDeviceManager::fDefaultManager = NULL;

