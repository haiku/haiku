// KDiskDeviceManager.cpp

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
#include "KDiskDeviceManager.h"
#include "KDiskDeviceUtils.h"
#include "KDiskSystem.h"
#include "KFileDiskDevice.h"
#include "KFileSystem.h"
#include "KPartition.h"
#include "KPartitioningSystem.h"

// debugging
//#define DBG(x)
#define DBG(x) x
#define OUT printf

// TODO: Remove when not longer needed.
using BPrivate::DiskDevice::KDiskDeviceJob;
using BPrivate::DiskDevice::KDiskDeviceJobQueue;

// directories for partitioning and file system modules
static const char *kPartitioningSystemPrefix	= "partitioning_systems";
static const char *kFileSystemPrefix			= "file_systems";

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


// constructor
KDiskDeviceManager::KDiskDeviceManager()
	: fLock("disk device manager"),
	  fDevices(new(nothrow) DeviceMap),
	  fPartitions(new(nothrow) PartitionMap),
	  fDiskSystems(new(nothrow) DiskSystemMap),
	  fObsoletePartitions(new(nothrow) PartitionSet)
{
	if (InitCheck() != B_OK)
		return;
	// add partitioning systems
	if (void *list = open_module_list(kPartitioningSystemPrefix)) {
		char moduleName[B_PATH_NAME_LENGTH];
		for (size_t bufferSize = sizeof(moduleName);
			 read_next_module_name(list, moduleName, &bufferSize) == B_OK;
			 bufferSize = sizeof(moduleName)) {
DBG(OUT("partitioning system: %s\n", moduleName));
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
DBG(OUT("file system: %s\n", moduleName));
			_AddFileSystem(moduleName);
		}
		close_module_list(list);
	}
DBG(OUT("number of disk systems: %ld\n", CountDiskSystems()));
	// TODO: Watch the disk systems and the relevant directories.
}

// destructor
KDiskDeviceManager::~KDiskDeviceManager()
{
	// remove all devices
	for (int32 cookie = 0; KDiskDevice *device = NextDevice(&cookie);) {
		PartitionRegistrar _(device);
		_RemoveDevice(device);
	}
	// some sanity checks
	if (fPartitions->Count() > 0) {
		DBG(OUT("WARNING: There are still %ld unremoved partitions!\n",
				fPartitions->Count()));
	}
	if (fObsoletePartitions->Count() > 0) {
		DBG(OUT("WARNING: There are still %ld obsolete partitions!\n",
				fObsoletePartitions->Count()));
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
}

// InitCheck
status_t
KDiskDeviceManager::InitCheck() const
{
	if (!fPartitions || !fDevices || !fDiskSystems || !fObsoletePartitions)
		return B_NO_MEMORY;
	return (fLock.Sem() >= 0 ? B_OK : fLock.Sem());
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
KDiskDeviceManager::FindDevice(partition_id id)
{
	if (KPartition *partition = FindPartition(id))
		return partition->Device();
	return NULL;
}

// FindPartition
KPartition *
KDiskDeviceManager::FindPartition(const char *path, bool noShadow)
{
// TODO: Optimize!
	for (PartitionMap::Iterator it = fPartitions->Begin();
		 it != fPartitions->End();
		 ++it) {
		KPartition *partition = it->Value();
		char partitionPath[B_PATH_NAME_LENGTH];
		if (partition->GetPath(partitionPath) == B_OK
			&& !strcmp(path, partitionPath)) {
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
KDiskDeviceManager::RegisterDevice(partition_id id)
{
	if (ManagerLocker locker = this) {
		if (KDiskDevice *device = FindDevice(id)) {
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

// CreateFileDevice
status_t
KDiskDeviceManager::CreateFileDevice(const char *filePath,
									 partition_id *deviceID)
{
	if (!filePath)
		return B_BAD_VALUE;
	status_t error = B_ERROR;
	KFileDiskDevice *device = NULL;
	if (ManagerLocker locker = this) {
		// check, if the device does already exist
		if (FindFileDevice(filePath))
			return B_FILE_EXISTS;
		// allocate a KFileDiskDevice
		device = new(nothrow) KFileDiskDevice;
		if (!device)
			return B_NO_MEMORY;
		// initialize and add the device
		error = device->SetTo(filePath);
		if (error == B_OK && !_AddDevice(device))
			error = B_NO_MEMORY;
		// set result / cleanup und failure
		if (error != B_OK) {
			delete device;
			return error;
		}
		if (deviceID)
			*deviceID = device->ID();
		device->Register();
	}
	// scan device
	if (error == B_OK && device) {
		_ScanDevice(device);
		device->Unregister();
	}
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
	for (int32 cookie = 0;
		 KDiskSystem *diskSystem = NextDiskSystem(&cookie); ) {
		if (!strcmp(name, diskSystem->Name()))
			return diskSystem;
	}
	return NULL;
}

// DiskSystemWithID
KDiskSystem *
KDiskDeviceManager::DiskSystemWithID(disk_system_id id)
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
KDiskDeviceManager::LoadDiskSystem(disk_system_id id)
{
	KDiskSystem *diskSystem = NULL;
	if (ManagerLocker locker = this) {
		diskSystem = DiskSystemWithID(id);
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
	if (ManagerLocker locker = this)
		error = _Scan("/dev/disk");
	// scan the devices for partitions
	// TODO: This is only provisional.
	if (error == B_OK) {
		int32 cookie = 0;
		while (KDiskDevice *device = RegisterNextDevice(&cookie)) {
			error = _ScanDevice(device);
			device->Unregister();
		}
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

// _Scan
status_t
KDiskDeviceManager::_Scan(const char *path)
{
	status_t error = B_OK;
	struct stat st;
	if (lstat(path, &st) < 0)
		return errno;
	if (S_ISDIR(st.st_mode)) {
		// a directory: iterate through its contents
		DIR *dir = opendir(path);
		if (!dir)
			return errno;
		while (dirent *entry = readdir(dir)) {
			// skip "." and ".."
			if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
				continue;
			if (strlen(path) + strlen(entry->d_name) + 1 >= B_PATH_NAME_LENGTH)
				continue;
			char entryPath[B_PATH_NAME_LENGTH];
			sprintf(entryPath, "%s/%s", path, entry->d_name);
			_Scan(entryPath);
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

// _ScanDevice
status_t
KDiskDeviceManager::_ScanDevice(KDiskDevice *device)
{
	status_t error = B_OK;
	if (device->WriteLock()) {
		// scan the device
		if (device->HasMedia()) {
			error = _ScanPartition(device);
			if (error != B_OK) {
				// ...
				error = B_OK;
			}
		}
		device->WriteUnlock();
	} else
		error = B_ERROR;
	return error;
}

// _ScanPartition
status_t
KDiskDeviceManager::_ScanPartition(KPartition *partition)
{
	// the partition's device must be write-locked
	if (!partition)
		return B_BAD_VALUE;
char partitionPath[B_PATH_NAME_LENGTH];
partition->GetPath(partitionPath);
DBG(OUT("KDiskDeviceManager::_ScanPartition(%s)\n", partitionPath));
	// publish the partition
	status_t error = partition->PublishDevice();
	if (error != B_OK)
		return error;
	// find the disk system that returns the best priority for this partition
	float bestPriority = -1;
	KDiskSystem *bestDiskSystem = NULL;
	void *bestCookie = NULL;
	int32 itCookie = 0;
	while (KDiskSystem *diskSystem = LoadNextDiskSystem(&itCookie)) {
DBG(OUT("  trying: %s\n", diskSystem->Name()));
		void *cookie = NULL;
		float priority = diskSystem->Identify(partition, &cookie);
DBG(OUT("  returned: %f\n", priority));
		if (priority >= 0 && priority > bestPriority) {
			// new best disk system
			if (bestDiskSystem) {
				bestDiskSystem->FreeIdentifyCookie(partition, bestCookie);
				bestDiskSystem->Unload();
			}
			bestPriority = priority;
			bestDiskSystem = diskSystem;
			bestCookie = cookie;
		} else {
			// disk system doesn't identify the partition or worse than our
			// current favorite
			diskSystem->FreeIdentifyCookie(partition, cookie);
			diskSystem->Unload();
		}
	}
	// now, if we have found a disk system, let it scan the partition
	if (bestDiskSystem) {
DBG(OUT("  scanning with: %s\n", bestDiskSystem->Name()));
		error = bestDiskSystem->Scan(partition, bestCookie);
		if (error == B_OK) {
			partition->SetDiskSystem(bestDiskSystem);
			for (int32 i = 0; KPartition *child = partition->ChildAt(i); i++)
				_ScanPartition(child);
		} else {
			// TODO: Handle the error.
DBG(OUT("  scanning failed: %s\n", strerror(error)));
		}
		// now we can safely unload the disk system -- it has been loaded by
		// the partition(s) and thus will not really be unloaded
		bestDiskSystem->Unload();
	} else {
		// contents not recognized
		// nothing to be done -- partitions are created as unrecognized
	}
	return error;
}


// singleton instance
KDiskDeviceManager *KDiskDeviceManager::fDefaultManager = NULL;

