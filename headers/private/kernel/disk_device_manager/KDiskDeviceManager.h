/*
 * Copyright 2004-2006, Haiku, Inc. All rights reserved.
 * Copyright 2003-2004, Ingo Weinhold, bonefish@cs.tu-berlin.de. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _K_DISK_DEVICE_MANAGER_H
#define _K_DISK_DEVICE_MANAGER_H


#include "disk_device_manager.h"
#include "Locker.h"


namespace BPrivate {
namespace DiskDevice {

class KDiskDevice;
class KDiskDeviceJob;
class KDiskDeviceJobFactory;
class KDiskDeviceJobQueue;
class KDiskSystem;
class KFileDiskDevice;
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

	// manager must be locked
	KDiskDevice *FindDevice(const char *path);
	KDiskDevice *FindDevice(partition_id id, bool deviceOnly = true);
	KPartition *FindPartition(const char *path, bool noShadow = false);
	KPartition *FindPartition(partition_id id, bool noShadow = false);
	KFileDiskDevice *FindFileDevice(const char *filePath);

	KDiskDevice *RegisterDevice(const char *path);
	KDiskDevice *RegisterDevice(partition_id id, bool deviceOnly = true);
	KDiskDevice *RegisterNextDevice(int32 *cookie);
	KPartition *RegisterPartition(const char *path, bool noShadow = false);
	KPartition *RegisterPartition(partition_id id, bool noShadow = false);
	KFileDiskDevice *RegisterFileDevice(const char *filePath);

	KDiskDevice *ReadLockDevice(partition_id id, bool deviceOnly = true);
	KDiskDevice *WriteLockDevice(partition_id id, bool deviceOnly = true);
		// The device is also registered and must be unregistered by the
		// caller.
	KPartition *ReadLockPartition(partition_id id);
	KPartition *WriteLockPartition(partition_id id);
		// Both the device and the partition is also registered and must be
		// unregistered by the caller.

	partition_id CreateFileDevice(const char *filePath,
		bool *newlyCreated = NULL, bool async = true);
	status_t DeleteFileDevice(const char *filePath);
	status_t DeleteFileDevice(partition_id id);

	// manager must be locked
	int32 CountDevices();
	KDiskDevice *NextDevice(int32 *cookie);

	bool PartitionAdded(KPartition *partition);		// implementation internal
	bool PartitionRemoved(KPartition *partition);	//
	bool DeletePartition(KPartition *partition);	//

	// Jobs

	// manager must be locked
	KDiskDeviceJob *FindJob(disk_job_id id);
	int32 CountJobs();
	KDiskDeviceJob *NextJob(int32 *cookie);

	// manager must be locked
	status_t AddJobQueue(KDiskDeviceJobQueue *jobQueue);
		// the device must be write locked
	status_t RemoveJobQueue(KDiskDeviceJobQueue *jobQueue);
	status_t DeleteJobQueue(KDiskDeviceJobQueue *jobQueue);
		// called when the execution is done
	int32 CountJobQueues();
	KDiskDeviceJobQueue *NextJobQueue(int32 *cookie);

	KDiskDeviceJobFactory *JobFactory() const;

	// manager must *not* be locked
	status_t UpdateBusyPartitions(KDiskDevice *device);
	status_t UpdateJobStatus(KDiskDeviceJob *job, uint32 status,
							 bool updateBusyPartitions);

	// Disk Systems

	// manager must be locked
	KDiskSystem *FindDiskSystem(const char *name);
	KDiskSystem *FindDiskSystem(disk_system_id id);
	int32 CountDiskSystems();
	KDiskSystem *NextDiskSystem(int32 *cookie);

	KDiskSystem *LoadDiskSystem(const char *name);
	KDiskSystem *LoadDiskSystem(disk_system_id id);
	KDiskSystem *LoadNextDiskSystem(int32 *cookie);

	// Watching

	// TODO: Watching service for the kernel. The userland watching is handled
	// by the registrar.

	status_t InitialDeviceScan();

private:
	status_t _AddPartitioningSystem(const char *name);
	status_t _AddFileSystem(const char *name);
	status_t _AddDiskSystem(KDiskSystem *diskSystem);

	bool _AddDevice(KDiskDevice *device);
	bool _RemoveDevice(KDiskDevice *device);

	bool _RemoveJobQueue(KDiskDeviceJobQueue *jobQueue);

	status_t _UpdateBusyPartitions(KDiskDevice *device);
	status_t _UpdateJobStatus(KDiskDeviceJob *job, uint32 status,
							  bool updateBusyPartitions);

	status_t _Scan(const char *path);
	status_t _ScanPartition(KPartition *partition, bool async);
		// the manager must be locked and the device write locked

	struct DeviceMap;
	struct DiskSystemMap;
	struct JobMap;
	struct JobQueueVector;
	struct PartitionMap;
	struct PartitionSet;

	BLocker						fLock;
	DeviceMap					*fDevices;
	PartitionMap				*fPartitions;
	DiskSystemMap				*fDiskSystems;
	PartitionSet				*fObsoletePartitions;
	JobMap						*fJobs;
	JobQueueVector				*fJobQueues;
	KDiskDeviceJobFactory		*fJobFactory;

	static KDiskDeviceManager	*sDefaultManager;
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KDiskDeviceManager;

#endif	// _K_DISK_DEVICE_MANAGER_H
