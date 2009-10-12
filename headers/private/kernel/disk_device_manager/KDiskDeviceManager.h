/*
 * Copyright 2004-2009, Haiku, Inc. All rights reserved.
 * Copyright 2003-2004, Ingo Weinhold, bonefish@cs.tu-berlin.de. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _K_DISK_DEVICE_MANAGER_H
#define _K_DISK_DEVICE_MANAGER_H


#include <disk_device_manager.h>
#include <Locker.h>
#include <Notifications.h>


namespace BPrivate {
namespace DiskDevice {

class KDiskDevice;
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

	DefaultUserNotificationService& Notifications();
	void Notify(const KMessage& event, uint32 eventMask);

	// manager must be locked
	KDiskDevice *FindDevice(const char *path);
	KDiskDevice *FindDevice(partition_id id, bool deviceOnly = true);
	KPartition *FindPartition(const char *path);
	KPartition *FindPartition(partition_id id);
	KFileDiskDevice *FindFileDevice(const char *filePath);

	KDiskDevice *RegisterDevice(const char *path);
	KDiskDevice *RegisterDevice(partition_id id, bool deviceOnly = true);
	KDiskDevice *RegisterNextDevice(int32 *cookie);
	KPartition *RegisterPartition(const char *path);
	KPartition *RegisterPartition(partition_id id);
	KFileDiskDevice *RegisterFileDevice(const char *filePath);

	KDiskDevice *ReadLockDevice(partition_id id, bool deviceOnly = true);
	KDiskDevice *WriteLockDevice(partition_id id, bool deviceOnly = true);
		// The device is also registered and must be unregistered by the
		// caller.
	KPartition *ReadLockPartition(partition_id id);
	KPartition *WriteLockPartition(partition_id id);
		// Both the device and the partition is also registered and must be
		// unregistered by the caller.

	status_t ScanPartition(KPartition* partition);

	partition_id CreateDevice(const char *path, bool *newlyCreated = NULL);
	status_t DeleteDevice(const char *path);

	partition_id CreateFileDevice(const char* filePath,
		bool* newlyCreated = NULL);
	status_t DeleteFileDevice(const char *filePath);
	status_t DeleteFileDevice(partition_id id);

	// manager must be locked
	int32 CountDevices();
	KDiskDevice *NextDevice(int32 *cookie);

	bool PartitionAdded(KPartition *partition);		// implementation internal
	bool PartitionRemoved(KPartition *partition);	//
	bool DeletePartition(KPartition *partition);	//

	// Disk Systems

	// manager must be locked
	KDiskSystem *FindDiskSystem(const char *name, bool byPrettyName = false);
	KDiskSystem *FindDiskSystem(disk_system_id id);
	int32 CountDiskSystems();
	KDiskSystem *NextDiskSystem(int32 *cookie);

	KDiskSystem *LoadDiskSystem(const char *name, bool byPrettyName = false);
	KDiskSystem *LoadDiskSystem(disk_system_id id);
	KDiskSystem *LoadNextDiskSystem(int32 *cookie);

	status_t InitialDeviceScan();
	status_t RescanDiskSystems();
	status_t StartMonitoring();

private:
	struct DeviceMap;
	struct DiskSystemMap;
	struct PartitionMap;
	struct PartitionSet;
	class DiskSystemWatcher;
	class DeviceWatcher;
	class DiskNotifications;

	static status_t _CheckMediaStatusDaemon(void* self);
	status_t _CheckMediaStatus();

	status_t _RescanDiskSystems(DiskSystemMap& addedSystems, bool fileSystems);

	status_t _AddPartitioningSystem(const char *name);
	status_t _AddFileSystem(const char *name);
	status_t _AddDiskSystem(KDiskSystem *diskSystem);

	bool _AddDevice(KDiskDevice *device);
	bool _RemoveDevice(KDiskDevice *device);

	status_t _Scan(const char *path);
	status_t _ScanPartition(KPartition *partition, bool async,
		DiskSystemMap* restrictScan = NULL);
		// the manager must be locked and the device write locked
	status_t _ScanPartition(KPartition *partition,
		DiskSystemMap* restrictScan);

	status_t _AddRemoveMonitoring(const char *path, bool add);

	void _NotifyDeviceEvent(KDiskDevice* device, int32 event, uint32 mask);

	recursive_lock				fLock;
	DeviceMap					*fDevices;
	PartitionMap				*fPartitions;
	DiskSystemMap				*fDiskSystems;
	PartitionSet				*fObsoletePartitions;
	thread_id					fMediaChecker;
	volatile bool				fTerminating;
	DiskSystemWatcher			*fDiskSystemWatcher;
	DeviceWatcher				*fDeviceWatcher;
	DiskNotifications*			fNotifications;

	static KDiskDeviceManager	*sDefaultManager;
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KDiskDeviceManager;

#endif	// _K_DISK_DEVICE_MANAGER_H
