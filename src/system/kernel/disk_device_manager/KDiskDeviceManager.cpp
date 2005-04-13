/* 
** Copyright 2003-2004, Ingo Weinhold, bonefish@cs.tu-berlin.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <KernelExport.h>
#include <util/kernel_cpp.h>

#include <dirent.h>
#include <errno.h>
#include <module.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <VectorMap.h>
#include <VectorSet.h>

#include "KDiskDevice.h"
#include "KDiskDeviceJob.h"
#include "KDiskDeviceJobFactory.h"
#include "KDiskDeviceJobQueue.h"
#include "KDiskDeviceManager.h"
#include "KDiskDeviceUtils.h"
#include "KDiskSystem.h"
#include "KFileDiskDevice.h"
#include "KFileSystem.h"
#include "KPartition.h"
#include "KPartitioningSystem.h"
#include "KPartitionVisitor.h"
#include "KPath.h"
#include "KShadowPartition.h"

// debugging
//#define DBG(x)
#define DBG(x) x
#define OUT dprintf


// directories for partitioning and file system modules
static const char *kPartitioningSystemPrefix = "partitioning_systems";
static const char *kFileSystemPrefix = "file_systems";
static const char *kFileSystemDiskDeviceSuffix = "/disk_device/v1";


// singleton instance
KDiskDeviceManager *KDiskDeviceManager::sDefaultManager = NULL;


// GetPartitionID
struct GetPartitionID {
	inline partition_id operator()(const KPartition *partition) const
	{
		return partition->ID();
	}
};

// GetDiskSystemID
struct GetDiskSystemID {
	inline disk_system_id operator()(const KDiskSystem *system) const
	{
		return system->ID();
	}
};

// GetJobID
struct GetJobID {
	inline disk_job_id operator()(const KDiskDeviceJob *job) const
	{
		return job->ID();
	}
};

// PartitionMap
struct KDiskDeviceManager::PartitionMap : VectorMap<partition_id, KPartition*,
	VectorMapEntryStrategy::ImplicitKey<partition_id, KPartition*,
		GetPartitionID> > {
};
	
// DeviceMap
struct KDiskDeviceManager::DeviceMap : VectorMap<partition_id, KDiskDevice*,
	VectorMapEntryStrategy::ImplicitKey<partition_id, KDiskDevice*,
		GetPartitionID> > {
};
	
// DiskSystemMap
struct KDiskDeviceManager::DiskSystemMap : VectorMap<disk_system_id,
	KDiskSystem*,
	VectorMapEntryStrategy::ImplicitKey<disk_system_id, KDiskSystem*,
		GetDiskSystemID> > {
};
	
// PartitionSet
struct KDiskDeviceManager::PartitionSet : VectorSet<KPartition*> {
};

// JobMap
struct KDiskDeviceManager::JobMap : VectorMap<disk_job_id, KDiskDeviceJob*,
	VectorMapEntryStrategy::ImplicitKey<disk_job_id, KDiskDeviceJob*,
		GetJobID> > {
};

// JobQueueVector
struct KDiskDeviceManager::JobQueueVector : Vector<KDiskDeviceJobQueue*> {};
	

static bool
is_active_job_status(uint32 status)
{
	return (status == B_DISK_DEVICE_JOB_SCHEDULED
			|| status == B_DISK_DEVICE_JOB_IN_PROGRESS);
}


static bool
is_fs_disk_device(const char *module)
{
	size_t prefixLength = strlen(kFileSystemPrefix);
	if (strncmp(module, kFileSystemPrefix, prefixLength))
		return false;

	size_t suffixLength = strlen(kFileSystemDiskDeviceSuffix);
	size_t length = strlen(module);

	if (length <= suffixLength + prefixLength)
		return false;

	return !strcmp(module + length - suffixLength, kFileSystemDiskDeviceSuffix);
}


//	#pragma mark -


KDiskDeviceManager::KDiskDeviceManager()
	: fLock("disk device manager"),
	  fDevices(new(nothrow) DeviceMap),
	  fPartitions(new(nothrow) PartitionMap),
	  fDiskSystems(new(nothrow) DiskSystemMap),
	  fObsoletePartitions(new(nothrow) PartitionSet),
	  fJobs(new(nothrow) JobMap),
	  fJobQueues(new(nothrow) JobQueueVector),
	  fJobFactory(new(nothrow) KDiskDeviceJobFactory)
{
	if (InitCheck() != B_OK)
		return;

	// We are in early boot mode, so open_module_list() won't find what
	// we're looking for; we need to use get_next_loaded_module_name()
	// instead.

	uint32 cookie = 0;
	size_t partitioningPrefixLength = strlen(kPartitioningSystemPrefix);

	while (true) {
		KPath name;
		if (name.InitCheck() != B_OK)
			break;
		size_t nameLength = name.BufferSize();
		if (get_next_loaded_module_name(&cookie, name.LockBuffer(),
			&nameLength) != B_OK) {
			break;
		}
		name.UnlockBuffer();

		if (!strncmp(name.Path(), kPartitioningSystemPrefix,
				partitioningPrefixLength)) {
			DBG(OUT("partitioning system: %s\n", name.Path()));
			_AddPartitioningSystem(name.Path());
		} else if (is_fs_disk_device(name.Path())) {
			DBG(OUT("file system: %s\n", name.Path()));
			_AddFileSystem(name.Path());
		}
	}

	DBG(OUT("number of disk systems: %ld\n", CountDiskSystems()));
	// TODO: Watch the disk systems and the relevant directories.
}


KDiskDeviceManager::~KDiskDeviceManager()
{
	// TODO: terminate and remove all jobs
	// remove all devices
	for (int32 cookie = 0; KDiskDevice *device = NextDevice(&cookie);) {
		PartitionRegistrar _(device);
		_RemoveDevice(device);
	}
	// some sanity checks
	if (fPartitions->Count() > 0) {
		DBG(OUT("WARNING: There are still %ld unremoved partitions!\n",
				fPartitions->Count()));
		for (PartitionMap::Iterator it = fPartitions->Begin();
			 it != fPartitions->End(); ++it) {
			DBG(OUT("         partition: %ld\n", it->Value()->ID()));
		}
	}
	if (fObsoletePartitions->Count() > 0) {
		DBG(OUT("WARNING: There are still %ld obsolete partitions!\n",
				fObsoletePartitions->Count()));
		for (PartitionSet::Iterator it = fObsoletePartitions->Begin();
			 it != fObsoletePartitions->End(); ++it) {
			DBG(OUT("         partition: %ld\n", (*it)->ID()));
		}
	}
	// remove all disk systems
	for (int32 cookie = 0;
		 KDiskSystem *diskSystem = NextDiskSystem(&cookie); ) {
		fDiskSystems->Remove(diskSystem->ID());
		if (diskSystem->IsLoaded()) {
			DBG(OUT("WARNING: Disk system `%s' (%ld) is still loaded!\n",
					diskSystem->Name(), diskSystem->ID()));
		} else
			delete diskSystem;
	}

	// delete the containers
	delete fPartitions;
	delete fDevices;
	delete fDiskSystems;
	delete fObsoletePartitions;
	delete fJobs;
	delete fJobQueues;
	delete fJobFactory;
}

// InitCheck
status_t
KDiskDeviceManager::InitCheck() const
{
	if (!fPartitions || !fDevices || !fDiskSystems || !fObsoletePartitions
		|| !fJobs || !fJobQueues || !fJobFactory) {
		return B_NO_MEMORY;
	}
	return (fLock.Sem() >= 0 ? B_OK : fLock.Sem());
}


/** This creates the system's default DiskDeviceManager.
 *	The creation is not thread-safe, and shouldn't be done
 *	more than once.
 */

status_t
KDiskDeviceManager::CreateDefault()
{
	if (sDefaultManager != NULL)
		return B_OK;

	sDefaultManager = new(nothrow) KDiskDeviceManager;
	if (sDefaultManager == NULL)
		return B_NO_MEMORY;

	return sDefaultManager->InitCheck();
}


/**	This deletes the default DiskDeviceManager. The
 *	deletion is not thread-safe either, you should
 *	make sure that it's called only once.
 */

void
KDiskDeviceManager::DeleteDefault()
{
	delete sDefaultManager;
	sDefaultManager = NULL;
}

// Default
KDiskDeviceManager *
KDiskDeviceManager::Default()
{
	return sDefaultManager;
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
KDiskDeviceManager::FindDevice(const char *path)
{
	for (int32 cookie = 0; KDiskDevice *device = NextDevice(&cookie); ) {
		if (device->Path() && !strcmp(path, device->Path()))
			return device;
	}
	return NULL;
}

// FindDevice
KDiskDevice *
KDiskDeviceManager::FindDevice(partition_id id, bool deviceOnly)
{
	if (KPartition *partition = FindPartition(id)) {
		KDiskDevice *device = partition->Device();
		if (!deviceOnly || id == device->ID())
			return device;
	}
	return NULL;
}

// FindPartition
KPartition *
KDiskDeviceManager::FindPartition(const char *path, bool noShadow)
{
// TODO: Optimize!
	KPath partitionPath;
	if (partitionPath.InitCheck() != B_OK)
		return NULL;
	for (PartitionMap::Iterator it = fPartitions->Begin();
		 it != fPartitions->End();
		 ++it) {
		KPartition *partition = it->Value();
		if (partition->GetPath(&partitionPath) == B_OK
			&& partitionPath == path) {
			if (noShadow && partition->IsShadowPartition())
				return partition->PhysicalPartition();
			return partition;
		}
	}
	return NULL;
}

// FindPartition
KPartition *
KDiskDeviceManager::FindPartition(partition_id id, bool noShadow)
{
	PartitionMap::Iterator it = fPartitions->Find(id);
	if (it != fPartitions->End()) {
		if (noShadow && it->Value()->IsShadowPartition())
			return it->Value()->PhysicalPartition();
		return it->Value();
	}
	return NULL;
}

// FindFileDevice
KFileDiskDevice *
KDiskDeviceManager::FindFileDevice(const char *filePath)
{
	for (int32 cookie = 0; KDiskDevice *device = NextDevice(&cookie); ) {
		KFileDiskDevice *fileDevice = dynamic_cast<KFileDiskDevice*>(device);
		if (fileDevice && fileDevice->FilePath()
			&& !strcmp(filePath, fileDevice->FilePath())) {
			return fileDevice;
		}
	}
	return NULL;
}

// RegisterDevice
KDiskDevice *
KDiskDeviceManager::RegisterDevice(const char *path)
{
	if (ManagerLocker locker = this) {
		if (KDiskDevice *device = FindDevice(path)) {
			device->Register();
			return device;
		}
	}
	return NULL;
}

// RegisterDevice
KDiskDevice *
KDiskDeviceManager::RegisterDevice(partition_id id, bool deviceOnly)
{
	if (ManagerLocker locker = this) {
		if (KDiskDevice *device = FindDevice(id, deviceOnly)) {
			device->Register();
			return device;
		}
	}
	return NULL;
}

// RegisterNextDevice
KDiskDevice *
KDiskDeviceManager::RegisterNextDevice(int32 *cookie)
{
	if (!cookie)
		return NULL;
	if (ManagerLocker locker = this) {
		if (KDiskDevice *device = NextDevice(cookie)) {
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

// RegisterFileDevice
KFileDiskDevice *
KDiskDeviceManager::RegisterFileDevice(const char *filePath)
{
	if (ManagerLocker locker = this) {
		if (KFileDiskDevice *device = FindFileDevice(filePath)) {
			device->Register();
			return device;
		}
	}
	return NULL;
}

// ReadLockDevice
KDiskDevice *
KDiskDeviceManager::ReadLockDevice(partition_id id, bool deviceOnly)
{
	// register device
	KDiskDevice *device = RegisterDevice(id, deviceOnly);
	if (!device)
		return NULL;
	// lock device
	if (device->ReadLock())
		return device;
	device->Unregister();
	return NULL;
}

// WriteLockDevice
KDiskDevice *
KDiskDeviceManager::WriteLockDevice(partition_id id, bool deviceOnly)
{
	// register device
	KDiskDevice *device = RegisterDevice(id, deviceOnly);
	if (!device)
		return NULL;
	// lock device
	if (device->WriteLock())
		return device;
	device->Unregister();
	return NULL;
}

// ReadLockPartition
KPartition *
KDiskDeviceManager::ReadLockPartition(partition_id id)
{
	// register partition
	KPartition *partition = RegisterPartition(id);
	if (!partition)
		return NULL;
	// get and register the device
	KDiskDevice *device = NULL;
	if (ManagerLocker locker = this) {
		device = partition->Device();
		if (device)
			device->Register();
	}
	// lock the device
	if (device->ReadLock()) {
		// final check, if the partition still belongs to the device
		if (partition->Device() == device)
			return partition;
		device->ReadUnlock();
	}
	// cleanup on failure
	if (device)
		device->Unregister();
	partition->Unregister();
	return NULL;
}

// WriteLockPartition
KPartition *
KDiskDeviceManager::WriteLockPartition(partition_id id)
{
	// register partition
	KPartition *partition = RegisterPartition(id);
	if (!partition)
		return NULL;
	// get and register the device
	KDiskDevice *device = NULL;
	if (ManagerLocker locker = this) {
		device = partition->Device();
		if (device)
			device->Register();
	}
	// lock the device
	if (device->WriteLock()) {
		// final check, if the partition still belongs to the device
		if (partition->Device() == device)
			return partition;
		device->WriteUnlock();
	}
	// cleanup on failure
	if (device)
		device->Unregister();
	partition->Unregister();
	return NULL;
}

// CreateFileDevice
partition_id
KDiskDeviceManager::CreateFileDevice(const char *filePath, bool *newlyCreated)
{
	if (!filePath)
		return B_BAD_VALUE;

	// normalize the file path
	KPath normalizedFilePath;
	status_t error = normalizedFilePath.SetTo(filePath, true);
	if (error != B_OK)
		return error;
	filePath = normalizedFilePath.Path();

	KFileDiskDevice *device = NULL;
	if (ManagerLocker locker = this) {
		// check, if the device does already exist
		if ((device = FindFileDevice(filePath))) {
			if (newlyCreated)
				*newlyCreated = false;
			return device->ID();
		}

		// allocate a KFileDiskDevice
		device = new(nothrow) KFileDiskDevice;
		if (!device)
			return B_NO_MEMORY;

		// initialize and add the device
		error = device->SetTo(filePath);
		// Note: Here we are allowed to lock a device although already having
		// the manager locked, since it is not yet added to the manager.
		DeviceWriteLocker deviceLocker(device);
		if (error == B_OK && !deviceLocker.IsLocked())
			error = B_ERROR;
		if (error == B_OK && !_AddDevice(device))
			error = B_NO_MEMORY;

		// scan device
		if (error == B_OK) {
			_ScanPartition(device);

			if (newlyCreated)
				*newlyCreated = true;
			return device->ID();
		}

		// cleanup on failure
		delete device;
	} else
		error = B_ERROR;
	return error;
}

// DeleteFileDevice
status_t
KDiskDeviceManager::DeleteFileDevice(const char *filePath)
{
	if (KFileDiskDevice *device = RegisterFileDevice(filePath)) {
		PartitionRegistrar _(device, true);
		if (DeviceWriteLocker locker = device) {
			if (_RemoveDevice(device))
				return B_OK;
		}
	}
	return B_ERROR;
}

// DeleteFileDevice
status_t
KDiskDeviceManager::DeleteFileDevice(partition_id id)
{
	if (KDiskDevice *device = RegisterDevice(id)) {
		PartitionRegistrar _(device, true);
		if (!dynamic_cast<KFileDiskDevice*>(device) || id != device->ID())
			return B_ENTRY_NOT_FOUND;
		if (DeviceWriteLocker locker = device) {
			if (_RemoveDevice(device))
				return B_OK;
		}
	}
	return B_ERROR;
}

// CountDevices
int32
KDiskDeviceManager::CountDevices()
{
	return fDevices->Count();
}

// NextDevice
KDiskDevice *
KDiskDeviceManager::NextDevice(int32 *cookie)
{
	if (!cookie)
		return NULL;
	DeviceMap::Iterator it = fDevices->FindClose(*cookie, false);
	if (it != fDevices->End()) {
		KDiskDevice *device = it->Value();
		*cookie = device->ID() + 1;
		return device;
	}
	return NULL;
}

// PartitionAdded
bool
KDiskDeviceManager::PartitionAdded(KPartition *partition)
{
	return (partition && fPartitions->Put(partition->ID(), partition) == B_OK);
}

// PartitionRemoved
bool
KDiskDeviceManager::PartitionRemoved(KPartition *partition)
{
	if (partition && partition->PrepareForRemoval()
		&& fPartitions->Remove(partition->ID())) {
		// If adding the partition to the obsolete list fails (due to lack
		// of memory), we can't do anything about it. We will leak memory then.
		fObsoletePartitions->Insert(partition);
		partition->MarkObsolete();
		return true;
	}
	return false;
}

// DeletePartition
bool
KDiskDeviceManager::DeletePartition(KPartition *partition)
{
	if (partition && partition->IsObsolete()
		&& partition->CountReferences() == 0
		&& partition->PrepareForDeletion()
		&& fObsoletePartitions->Remove(partition)) {
		delete partition;
		return true;
	}
	return false;
}

// FindJob
KDiskDeviceJob *
KDiskDeviceManager::FindJob(disk_job_id id)
{
	JobMap::Iterator it = fJobs->Find(id);
	if (it == fJobs->End())
		return NULL;
	return it->Value();
}

// CountJobs
int32
KDiskDeviceManager::CountJobs()
{
	return fJobs->Count();
}

// NextJob
KDiskDeviceJob *
KDiskDeviceManager::NextJob(int32 *cookie)
{
	if (!cookie)
		return NULL;
	JobMap::Iterator it = fJobs->FindClose(*cookie, false);
	if (it != fJobs->End()) {
		KDiskDeviceJob *job = it->Value();
		*cookie = job->ID() + 1;
		return job;
	}
	return NULL;
}

// AddJobQueue
/*!
	The device must be write locked, the manager must be locked.
*/
status_t
KDiskDeviceManager::AddJobQueue(KDiskDeviceJobQueue *jobQueue)
{
	// check the parameter
	if (!jobQueue)
		return B_BAD_VALUE;
	if (jobQueue->InitCheck() != B_OK)
		return jobQueue->InitCheck();
	// add the queue
	status_t error = fJobQueues->PushBack(jobQueue);
	if (error != B_OK)
		return error;
	// add the queue's jobs
	int32 count = jobQueue->CountJobs();
	for (int32 i = 0; i < count; i++) {
		KDiskDeviceJob *job = jobQueue->JobAt(i);
		error = fJobs->Put(job->ID(), job);
		if (error != B_OK) {
			_RemoveJobQueue(jobQueue);
			return error;
		}
	}
	// mark the jobs scheduled
	for (int32 i = 0; KDiskDeviceJob *job = jobQueue->JobAt(i); i++)
		_UpdateJobStatus(job, B_DISK_DEVICE_JOB_SCHEDULED, false);
	_UpdateBusyPartitions(jobQueue->Device());
	// start the execution of the queue
	error = jobQueue->Execute();
	if (error != B_OK) {
		// resuming the execution failed -- mark all jobs failed and 
		// remove the queue
		for (int32 i = 0; KDiskDeviceJob *job = jobQueue->JobAt(i); i++)
			_UpdateJobStatus(job, B_DISK_DEVICE_JOB_FAILED, false);
		_RemoveJobQueue(jobQueue);
		_UpdateBusyPartitions(jobQueue->Device());
	}
	return error;
}

// RemoveJobQueue
status_t
KDiskDeviceManager::RemoveJobQueue(KDiskDeviceJobQueue *jobQueue)
{
	if (!jobQueue)
		return B_BAD_VALUE;
	if (jobQueue->InitCheck() != B_OK)
		return jobQueue->InitCheck();
	if (jobQueue->IsExecuting())
		return B_BAD_VALUE;
	return (_RemoveJobQueue(jobQueue) ? B_OK : B_ENTRY_NOT_FOUND);
}

// DeleteJobQueue
status_t
KDiskDeviceManager::DeleteJobQueue(KDiskDeviceJobQueue *jobQueue)
{
	status_t error = RemoveJobQueue(jobQueue);
	if (error == B_OK)
		delete jobQueue;
	return error;
}

// CountJobQueues
int32
KDiskDeviceManager::CountJobQueues()
{
	return fJobQueues->Count();
}

// NextJobQueue
KDiskDeviceJobQueue *
KDiskDeviceManager::NextJobQueue(int32 *cookie)
{
	if (!cookie || *cookie < 0 || *cookie >= CountJobQueues())
		return NULL;
	return fJobQueues->ElementAt((*cookie)++);
}

// JobFactory
KDiskDeviceJobFactory *
KDiskDeviceManager::JobFactory() const
{
	return fJobFactory;
}

// UpdateBusyPartitions
status_t
KDiskDeviceManager::UpdateBusyPartitions(KDiskDevice *device)
{
	if (!device)
		return B_BAD_VALUE;
	if (DeviceWriteLocker deviceLocker = device) {
		if (ManagerLocker locker = this)
			return _UpdateBusyPartitions(device);
	}
	return B_ERROR;
}

// UpdateJobStatus
status_t
KDiskDeviceManager::UpdateJobStatus(KDiskDeviceJob *job, uint32 status,
									bool updateBusyPartitions)
{
	// check parameters
	if (!job)
		return B_BAD_VALUE;
	KDiskDeviceJobQueue *jobQueue = job->JobQueue();
	KDiskDevice *device = (jobQueue ? jobQueue->Device() : NULL);
	if (!device)
		return B_BAD_VALUE;
	// lock device and manager
	if (DeviceWriteLocker deviceLocker = device) {
		if (ManagerLocker locker = this)
			return _UpdateJobStatus(job, status, updateBusyPartitions);
	}
	return B_ERROR;
}

// FindDiskSystem
KDiskSystem *
KDiskDeviceManager::FindDiskSystem(const char *name)
{
	for (int32 cookie = 0;
		 KDiskSystem *diskSystem = NextDiskSystem(&cookie); ) {
		if (!strcmp(name, diskSystem->Name()))
			return diskSystem;
	}
	return NULL;
}

// FindDiskSystem
KDiskSystem *
KDiskDeviceManager::FindDiskSystem(disk_system_id id)
{
	DiskSystemMap::Iterator it = fDiskSystems->Find(id);
	if (it != fDiskSystems->End())
		return it->Value();
	return NULL;
}

// CountDiskSystems
int32
KDiskDeviceManager::CountDiskSystems()
{
	return fDiskSystems->Count();
}

// NextDiskSystem
KDiskSystem *
KDiskDeviceManager::NextDiskSystem(int32 *cookie)
{
	if (!cookie)
		return NULL;
	DiskSystemMap::Iterator it = fDiskSystems->FindClose(*cookie, false);
	if (it != fDiskSystems->End()) {
		KDiskSystem *diskSystem = it->Value();
		*cookie = diskSystem->ID() + 1;
		return diskSystem;
	}
	return NULL;
}

// LoadDiskSystem
KDiskSystem *
KDiskDeviceManager::LoadDiskSystem(const char *name)
{
	KDiskSystem *diskSystem = NULL;
	if (ManagerLocker locker = this) {
		diskSystem = FindDiskSystem(name);
		if (diskSystem && diskSystem->Load() != B_OK)
			diskSystem = NULL;
	}
	return diskSystem;
}

// LoadDiskSystem
KDiskSystem *
KDiskDeviceManager::LoadDiskSystem(disk_system_id id)
{
	KDiskSystem *diskSystem = NULL;
	if (ManagerLocker locker = this) {
		diskSystem = FindDiskSystem(id);
		if (diskSystem && diskSystem->Load() != B_OK)
			diskSystem = NULL;
	}
	return diskSystem;
}

// LoadNextDiskSystem
KDiskSystem *
KDiskDeviceManager::LoadNextDiskSystem(int32 *cookie)
{
	if (!cookie)
		return NULL;
	if (ManagerLocker locker = this) {
		if (KDiskSystem *diskSystem = NextDiskSystem(cookie)) {
			if (diskSystem->Load() == B_OK) {
				*cookie = diskSystem->ID() + 1;
				return diskSystem;
			}
		}
	}
	return NULL;
}

// InitialDeviceScan
status_t
KDiskDeviceManager::InitialDeviceScan()
{
	status_t error = B_ERROR;
	// scan for devices
	if (ManagerLocker locker = this) {
		error = _Scan("/dev/disk");
		if (error != B_OK)
			return error;
	}
	// scan the devices for partitions
	int32 cookie = 0;
	while (KDiskDevice *device = RegisterNextDevice(&cookie)) {
		PartitionRegistrar _(device, true);
		if (DeviceWriteLocker deviceLocker = device) {
			if (ManagerLocker locker = this) {
				error = _ScanPartition(device);
				if (error != B_OK)
					break;
			} else
				return B_ERROR;
		} else
			return B_ERROR;
	}
	return error;
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
	if (!name)
		return B_BAD_VALUE;

	KDiskSystem *diskSystem = new(nothrow) KFileSystem(name);
	if (!diskSystem)
		return B_NO_MEMORY;

	return _AddDiskSystem(diskSystem);
}

// _AddDiskSystem
status_t
KDiskDeviceManager::_AddDiskSystem(KDiskSystem *diskSystem)
{
	if (!diskSystem)
		return B_BAD_VALUE;
DBG(OUT("KDiskDeviceManager::_AddDiskSystem(%s)\n", diskSystem->Name()));
	status_t error = diskSystem->Init();
if (error != B_OK)
DBG(OUT("  initialization failed: %s\n", strerror(error)));
	if (error == B_OK)
		error = fDiskSystems->Put(diskSystem->ID(), diskSystem);
	if (error != B_OK)
		delete diskSystem;
DBG(OUT("KDiskDeviceManager::_AddDiskSystem() done: %s\n", strerror(error)));
	return error;
}

// _AddDevice
bool
KDiskDeviceManager::_AddDevice(KDiskDevice *device)
{
	if (!device || !PartitionAdded(device))
		return false;
	if (fDevices->Put(device->ID(), device) == B_OK)
		return true;
	PartitionRemoved(device);
	return false;
}

// _RemoveDevice
bool
KDiskDeviceManager::_RemoveDevice(KDiskDevice *device)
{
	return (device && fDevices->Remove(device->ID())
			&& PartitionRemoved(device));
}

// _RemoveJobQueue
bool
KDiskDeviceManager::_RemoveJobQueue(KDiskDeviceJobQueue *jobQueue)
{
	if (!jobQueue)
		return false;
	// find the job queue
	JobQueueVector::Iterator it = fJobQueues->Find(jobQueue);
	if (it == fJobQueues->End())
		return false;
	// remove the queue's jobs
	int32 count = jobQueue->CountJobs();
	for (int32 i = 0; i < count; i++) {
		KDiskDeviceJob *job = jobQueue->JobAt(i);
		fJobs->Remove(job->ID());
	}
	fJobQueues->Erase(it);
	return true;
}

// _UpdateBusyPartitions
/*!
	The device must be write locked, the manager must be locked.
*/
status_t
KDiskDeviceManager::_UpdateBusyPartitions(KDiskDevice *device)
{
	if (!device)
		return B_BAD_VALUE;
	// mark all partitions un-busy
	struct UnmarkBusyVisitor : KPartitionVisitor {
		virtual bool VisitPre(KPartition *partition)
		{
			partition->ClearFlags(B_PARTITION_BUSY
								  | B_PARTITION_DESCENDANT_BUSY);
			return false;
		}
	} visitor;
	device->VisitEachDescendant(&visitor);
	// Iterate through all job queues and all jobs scheduled or in
	// progress and mark their scope busy.
	for (int32 cookie = 0;
		 KDiskDeviceJobQueue *jobQueue = NextJobQueue(&cookie); ) {
		if (jobQueue->Device() != device)
			continue;
		for (int32 i = jobQueue->ActiveJobIndex();
			 KDiskDeviceJob *job = jobQueue->JobAt(i); i++) {
			if (job->Status() != B_DISK_DEVICE_JOB_IN_PROGRESS
				&& job->Status() != B_DISK_DEVICE_JOB_SCHEDULED) {
				continue;
			}
			KPartition *partition = FindPartition(job->ScopeID());
			if (!partition || partition->Device() != device)
				continue;
			partition->AddFlags(B_PARTITION_BUSY);
		}
	}
	// mark all anscestors of busy partitions descendant busy and all
	// descendants busy
	struct MarkBusyVisitor : KPartitionVisitor {
		virtual bool VisitPre(KPartition *partition)
		{
			// parent busy => child busy
			if (partition->Parent() && partition->Parent()->IsBusy())
				partition->AddFlags(B_PARTITION_BUSY);
			return false;
		}

		virtual bool VisitPost(KPartition *partition)
		{
			// child [descendant] busy => parent descendant busy
			if ((partition->IsBusy() || partition->IsDescendantBusy())
				&& partition->Parent()) {
				partition->Parent()->AddFlags(B_PARTITION_DESCENDANT_BUSY);
			}
			return false;
		}
	} visitor2;
	device->VisitEachDescendant(&visitor2);
	return B_OK;
}

// _UpdateJobStatus
status_t
KDiskDeviceManager::_UpdateJobStatus(KDiskDeviceJob *job, uint32 status,
									 bool updateBusyPartitions)
{
	// check parameters
	if (!job)
		return B_BAD_VALUE;
	KDiskDeviceJobQueue *jobQueue = job->JobQueue();
	KDiskDevice *device = (jobQueue ? jobQueue->Device() : NULL);
	if (!device)
		return B_BAD_VALUE;
	if (job->Status() == status)
		return B_OK;
	// check migration of a schedule/in progress to a terminal state
	// or vice versa
	updateBusyPartitions &= (is_active_job_status(job->Status())
							 != is_active_job_status(status));
	// set new state and update the partitions' busy flags
	job->SetStatus(status);
	if (updateBusyPartitions)
		return _UpdateBusyPartitions(device);
	// TODO: notifications
	return B_OK;
}

// _Scan
status_t
KDiskDeviceManager::_Scan(const char *path)
{
DBG(OUT("KDiskDeviceManager::_Scan(%s)\n", path));
	status_t error = B_OK;
	struct stat st;
	if (lstat(path, &st) < 0) {
		return errno;
	}
	if (S_ISDIR(st.st_mode)) {
		// a directory: iterate through its contents
		DIR *dir = opendir(path);
		if (!dir)
			return errno;
		while (dirent *entry = readdir(dir)) {
			// skip "." and ".."
			if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
				continue;
			KPath entryPath;
			if (entryPath.SetPath(path) != B_OK
				|| entryPath.Append(entry->d_name) != B_OK) {
				continue;
			}
			_Scan(entryPath.Path());
		}
		closedir(dir);
	} else {
		// not a directory
		// check, if it is named "raw"
		int32 len = strlen(path);
		int32 leafLen = strlen("/raw");
		if (len <= leafLen || strcmp(path + len - leafLen, "/raw"))
			return B_ERROR;
DBG(OUT("  found device: %s\n", path));
		// create a KDiskDevice for it
		KDiskDevice *device = new(nothrow) KDiskDevice;
		if (!device)
			return B_NO_MEMORY;
		// init the KDiskDevice
		status_t error = device->SetTo(path);
		// add the device
		if (error == B_OK && !_AddDevice(device))
			error = B_NO_MEMORY;
		// cleanup on error
		if (error != B_OK)
			delete device;
	}
	return error;
}

// _ScanPartition
/*!
	The device must be write locked, the manager must be locked.
*/
status_t
KDiskDeviceManager::_ScanPartition(KPartition *partition)
{
	if (!partition)
		return B_BAD_VALUE;
	// create a new job queue for the device
	KDiskDeviceJobQueue *jobQueue = new(nothrow) KDiskDeviceJobQueue;
	if (!jobQueue)
		return B_NO_MEMORY;
	jobQueue->SetDevice(partition->Device());
	// create a job for scanning the device and add it to the job queue
	KDiskDeviceJob *job = fJobFactory->CreateScanPartitionJob(partition->ID());
	if (!job) {
		delete jobQueue;
		return B_NO_MEMORY;
	}
	if (!jobQueue->AddJob(job)) {
		delete jobQueue;
		delete job;
		return B_NO_MEMORY;
	}
	// add the job queue
	status_t error = AddJobQueue(jobQueue);
	if (error != B_OK)
		delete jobQueue;
	return error;
}

