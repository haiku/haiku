// KDiskDeviceManager.cpp

#include <module.h>
#include <stdlib.h>
#include <string.h>

#include "KDiskDevice.h"
#include "KDiskDeviceManager.h"
#include "KDiskDeviceUtils.h"
#include "KDiskSystem.h"
#include "KFileSystem.h"
#include "KPartition.h"
#include "KPartitioningSystem.h"

// TODO: Remove when not longer needed.
using BPrivate::DiskDevice::KDiskDeviceJob;
using BPrivate::DiskDevice::KDiskDeviceJobQueue;

// directories for partitioning and file system modules
static const char *kPartitioningSystemPrefix	= "partitioning_systems";
static const char *kFileSystemPrefix			= "file_systems";

// constructor
KDiskDeviceManager::KDiskDeviceManager()
	: fLock("disk device manager"),
	  fDevices(20),
	  fPartitions(100),
	  fDiskSystems(20)
{
	// add partitioning systems
	if (void *list = open_module_list(kPartitioningSystemPrefix)) {
		char moduleName[B_PATH_NAME_LENGTH];
		for (size_t bufferSize = sizeof(moduleName);
			 read_next_module_name(list, moduleName, &bufferSize) == B_OK;
			 bufferSize = sizeof(moduleName)) {
			_AddPartitioningSystem(moduleName);
		}
		close_module_list(list);
	}
	// add file systems
	if (void *list = open_module_list(kFileSystemPrefix)) {
		char moduleName[B_PATH_NAME_LENGTH];
		for (size_t bufferSize = sizeof(moduleName);
			 read_next_module_name(list, moduleName, &bufferSize) == B_OK;
			 bufferSize = sizeof(moduleName)) {
			_AddFileSystem(moduleName);
		}
		close_module_list(list);
	}
	// TODO: Watch the disk systems and the relevant directories.
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

// FindDevice
KDiskDevice *
KDiskDeviceManager::FindDevice(const char *path, bool noShadow)
{
// TODO: Handle shadows correctly!
	for (int32 i = 0; KDiskDevice *device = fDevices.ItemAt(i); i++) {
		if (device->Path() && !strcmp(path, device->Path()))
			return device;
	}
	return NULL;
}

// FindDevice
KDiskDevice *
KDiskDeviceManager::FindDevice(partition_id id, bool noShadow)
{
// TODO: Handle shadows correctly!
	if (KPartition *partition = FindPartition(id))
		return partition->Device();
	return NULL;
}

// FindPartition
KPartition *
KDiskDeviceManager::FindPartition(const char *path, bool noShadow)
{
// TODO: Handle shadows correctly!
// TODO: Optimize!
	for (int32 i = 0; KPartition *partition = fPartitions.ItemAt(i); i++) {
		char partitionPath[B_PATH_NAME_LENGTH];
		if (partition->GetPath(partitionPath) == B_OK
			&& !strcmp(path, partitionPath)) {
			return partition;
		}
	}
	return NULL;
}

// FindPartition
KPartition *
KDiskDeviceManager::FindPartition(partition_id id, bool noShadow)
{
// TODO: Handle shadows correctly!
	for (int32 i = 0; KPartition *partition = fPartitions.ItemAt(i); i++) {
		if (partition->ID() == id)
			return partition;
	}
	return NULL;
}

// RegisterDevice
KDiskDevice *
KDiskDeviceManager::RegisterDevice(const char *path, bool noShadow)
{
// TODO: Handle shadows correctly!
	if (ManagerLocker locker = this) {
		if (KDiskDevice *device = FindDevice(path, noShadow)) {
			device->Register();
			return device;
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
		if (KDiskDevice *device = FindDevice(id, noShadow)) {
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
	if (ManagerLocker locker = this) {
		if (KPartition *partition = FindPartition(path, noShadow)) {
			partition->Register();
			return partition;
		}
	}
	return NULL;
}

// RegisterPartition
KPartition *
KDiskDeviceManager::RegisterPartition(partition_id id, bool noShadow)
{
	if (ManagerLocker locker = this) {
		if (KPartition *partition = FindPartition(id, noShadow)) {
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
	for (int32 i = 0; KDiskSystem *diskSystem = fDiskSystems.ItemAt(i); i++) {
		if (!strcmp(name, diskSystem->Name()))
			return diskSystem;
	}
	return NULL;
}

// DiskSystemWithID
KDiskSystem *
KDiskDeviceManager::DiskSystemWithID(disk_system_id id)
{
	for (int32 i = 0; KDiskSystem *diskSystem = fDiskSystems.ItemAt(i); i++) {
		if (diskSystem->ID() == id)
			return diskSystem;
	}
	return NULL;
}

// CountDiskSystems
int32
KDiskDeviceManager::CountDiskSystems()
{
	return fDiskSystems.CountItems();
}

// DiskSystemAt
KDiskSystem *
KDiskDeviceManager::DiskSystemAt(int32 index)
{
	return fDiskSystems.ItemAt(index);
}

// _AddPartitioningSystem
status_t
KDiskDeviceManager::_AddPartitioningSystem(const char *name)
{
	if (!name)
		return B_BAD_VALUE;
	KDiskSystem *diskSystem = new(nothrow) KPartitioningSystem(name);
	if (!diskSystem)
		return B_NO_MEMORY;
	return _AddDiskSystem(diskSystem);
}

// _AddFileSystem
status_t
KDiskDeviceManager::_AddFileSystem(const char *name)
{
// TODO: Uncomment when KFileSystem is implemented.
/*
	if (!name)
		return B_BAD_VALUE;
	KDiskSystem *diskSystem = new(nothrow) KFileSystem(name);
	if (!diskSystem)
		return B_NO_MEMORY;
	return _AddDiskSystem(diskSystem);
*/
	return B_ERROR;
}

// _AddDiskSystem
status_t
KDiskDeviceManager::_AddDiskSystem(KDiskSystem *diskSystem)
{
	if (!diskSystem)
		return B_BAD_VALUE;
	status_t error = diskSystem->Init();
	if (error == B_OK && !fDiskSystems.AddItem(diskSystem))
		error = B_NO_MEMORY;
	if (error != B_OK)
		delete diskSystem;
	return error;
}


// singleton instance
KDiskDeviceManager *KDiskDeviceManager::fDefaultManager = NULL;

