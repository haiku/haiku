// disk_device_manager.cpp

#include <stdio.h>

#include "disk_device_manager.h"
#include "KDiskDevice.h"
#include "KDiskDeviceJob.h"
#include "KDiskDeviceJobQueue.h"
#include "KDiskDeviceManager.h"
#include "KDiskDeviceUtils.h"
#include "KDiskSystem.h"
#include "KPartition.h"

// debugging
//#define DBG(x)
#define DBG(x) x
#define OUT printf

// write_lock_disk_device
disk_device_data *
write_lock_disk_device(partition_id partitionID)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	if (KDiskDevice *device = manager->RegisterDevice(partitionID)) {
		if (device->WriteLock())
			return device->DeviceData();
		// Only unregister, when the locking fails. The guarantees, that the
		// lock owner also has a reference.
		device->Unregister();
	}
	return NULL;
}

// write_unlock_disk_device
void
write_unlock_disk_device(partition_id partitionID)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	if (KDiskDevice *device = manager->RegisterDevice(partitionID)) {
		bool isLocked = device->IsWriteLocked();
		if (isLocked) {
			device->WriteUnlock();
			device->Unregister();
		}
		device->Unregister();
	}
}

// read_lock_disk_device
disk_device_data *
read_lock_disk_device(partition_id partitionID)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	if (KDiskDevice *device = manager->RegisterDevice(partitionID)) {
		if (device->ReadLock())
			return device->DeviceData();
		// Only unregister, when the locking fails. The guarantees, that the
		// lock owner also has a reference.
		device->Unregister();
	}
	return NULL;
}

// read_unlock_disk_device
void
read_unlock_disk_device(partition_id partitionID)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	if (KDiskDevice *device = manager->RegisterDevice(partitionID)) {
		bool isLocked = device->IsReadLocked(false);
		if (isLocked) {
			device->ReadUnlock();
			device->Unregister();
		}
		device->Unregister();
	}
}

// find_disk_device
int32
find_disk_device(const char *path)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	partition_id id = -1;
	if (KDiskDevice *device = manager->RegisterDevice(path)) {
		id = device->ID();
		device->Unregister();
	}
	return id;
}

// find_partition
int32
find_partition(const char *path)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	partition_id id = -1;
	if (KPartition *partition = manager->RegisterPartition(path)) {
		id = partition->ID();
		partition->Unregister();
	}
	return id;
}

// get_disk_device
disk_device_data *
get_disk_device(partition_id partitionID)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	KDiskDevice *device = manager->FindDevice(partitionID, false);
	return (device ? device->DeviceData() : NULL);
}

// get_partition
partition_data *
get_partition(partition_id partitionID)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	KPartition *partition = manager->FindPartition(partitionID);
	return (partition ? partition->PartitionData() : NULL);
}

// get_parent_partition
partition_data *
get_parent_partition(partition_id partitionID)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	KPartition *partition = manager->FindPartition(partitionID);
	if (partition && partition->Parent())
		return partition->Parent()->PartitionData();
	return NULL;
}

// get_child_partition
partition_data *
get_child_partition(partition_id partitionID, int32 index)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	if (KPartition *partition = manager->FindPartition(partitionID)) {
		if (KPartition *child = partition->ChildAt(index))
			return child->PartitionData();
	}
	return NULL;
}

// create_child_partition
partition_data *
create_child_partition(partition_id partitionID, int32 index,
					   partition_id childID)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	if (KPartition *partition = manager->FindPartition(partitionID)) {
		KPartition *child = NULL;
		if (partition->CreateChild(childID, index, &child) == B_OK)
			return child->PartitionData();
else
DBG(OUT("  creating child (%ld, %ld) failed\n", partitionID, index));
	}
else
DBG(OUT("  partition %ld not found\n", partitionID));
	return NULL;
}

// delete_partition
bool
delete_partition(partition_id partitionID)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	if (KPartition *partition = manager->FindPartition(partitionID)) {
		if (KPartition *parent = partition->Parent())
			return parent->RemoveChild(partition);
	}
	return false;
}

// partition_modified
void
partition_modified(partition_id partitionID)
{
	// not implemented
}

// find_disk_system
disk_system_id
find_disk_system(const char *name)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	if (ManagerLocker locker = manager) {
		if (KDiskSystem *diskSystem = manager->FindDiskSystem(name))
			return diskSystem->ID();
	}
	return -1;
}

// update_disk_device_job_progress
bool
update_disk_device_job_progress(disk_job_id jobID, float progress)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	if (ManagerLocker locker = manager) {
		if (KDiskDeviceJob *job = manager->FindJob(jobID)) {
			job->UpdateProgress(progress);
			return true;
		}
	}
	return false;
}

// update_disk_device_job_extra_progress
bool
update_disk_device_job_extra_progress(disk_job_id jobID, const char *info)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	if (ManagerLocker locker = manager) {
		if (KDiskDeviceJob *job = manager->FindJob(jobID)) {
			job->UpdateExtraProgress(info);
			return true;
		}
	}
	return false;
}

// set_disk_device_job_error_message
bool
set_disk_device_job_error_message(disk_job_id jobID, const char *message)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	if (ManagerLocker locker = manager) {
		if (KDiskDeviceJob *job = manager->FindJob(jobID)) {
			job->SetErrorMessage(message);
			return true;
		}
	}
	return false;
}

// update_disk_device_job_interrupt_properties
uint32
update_disk_device_job_interrupt_properties(disk_job_id jobID,
											uint32 interruptProperties)
{
	bool paused = false;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	do {
		sem_id pauseSemaphore = -1;
		if (ManagerLocker locker = manager) {
			// get the job and the respective job queue
			if (KDiskDeviceJob *job = manager->FindJob(jobID)) {
				if (KDiskDeviceJobQueue *jobQueue = job->JobQueue()) {
					// terminate if canceled.
					if (jobQueue->IsCanceled()) {
						if (jobQueue->ShallReverse())
							return B_DISK_DEVICE_JOB_REVERSE;
						return B_DISK_DEVICE_JOB_CANCEL;
					}
					// set the new interrupt properties only when not
					// requested to pause
					if (jobQueue->IsPauseRequested())
						pauseSemaphore = jobQueue->ReadyToPause();
					else
						job->SetInterruptProperties(interruptProperties);
				}
			}
		}
		// pause, if requested; redo the loop then
		paused = (pauseSemaphore >= 0);
		if (paused) {
			acquire_sem(pauseSemaphore);
			pauseSemaphore = -1;
		}
	} while (paused);
	return B_DISK_DEVICE_JOB_CONTINUE;
}

