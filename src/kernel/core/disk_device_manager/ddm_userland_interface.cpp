// ddm_userland_interface.cpp

#include <KDiskDevice.h>
#include <KDiskDeviceManager.h>
#include <KDiskDeviceUtils.h>
#include <KFileDiskDevice.h>
#include <KDiskSystem.h>

#include "ddm_userland_interface.h"
#include "UserDataWriter.h"

// get_current_team
static
team_id
get_current_team()
{
	// TODO: There must be a straighter way in kernelland.
	thread_info info;
	get_thread_info(find_thread(NULL), &info);
	return info.team;
}

// check_shadow_partition
static
bool
check_shadow_partition(const KPartition *partition)
{
	return (partition && partition->Device() && partition->IsShadowPartition()
			&& partition->Device()->ShadowOwner() == get_current_team());
}

// _kern_get_next_disk_device_id
partition_id
_kern_get_next_disk_device_id(int32 *cookie, size_t *neededSize)
{
	if (!cookie)
		return B_BAD_VALUE;
	partition_id id = B_ENTRY_NOT_FOUND;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the next device
	if (KDiskDevice *device = manager->RegisterNextDevice(cookie)) {
		PartitionRegistrar _(device, true);
		id = device->ID();
		if (neededSize) {
			if (DeviceReadLocker locker = device) {
				// get the needed size
				UserDataWriter writer;
				device->WriteUserData(writer, false);
				*neededSize = writer.AllocatedSize();
			} else
				return B_ERROR;
		}
	}
	return id;
}

// _kern_find_disk_device
partition_id
_kern_find_disk_device(const char *filename, size_t *neededSize)
{
	if (!filename)
		return B_BAD_VALUE;
	partition_id id = B_ENTRY_NOT_FOUND;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// find the device
	if (KDiskDevice *device = manager->RegisterDevice(filename)) {
		PartitionRegistrar _(device, true);
		id = device->ID();
		if (neededSize) {
			if (DeviceReadLocker locker = device) {
				// get the needed size
				UserDataWriter writer;
				device->WriteUserData(writer, false);
				*neededSize = writer.AllocatedSize();
			} else
				return B_ERROR;
		}
	}
	return id;
}

// _kern_find_partition
partition_id
_kern_find_partition(const char *filename, size_t *neededSize)
{
	if (!filename)
		return B_BAD_VALUE;
	partition_id id = B_ENTRY_NOT_FOUND;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// find the partition
	if (KPartition *partition = manager->RegisterPartition(filename)) {
		PartitionRegistrar _(partition, true);
		id = partition->ID();
		if (neededSize) {
			// get and lock the partition's device
			KDiskDevice *device = manager->RegisterDevice(partition->ID());
			if (!device)
				return B_ENTRY_NOT_FOUND;
			PartitionRegistrar _2(device, true);
			if (DeviceReadLocker locker = device) {
				// get the needed size
				UserDataWriter writer;
				device->WriteUserData(writer, false);
				*neededSize = writer.AllocatedSize();
			} else
				return B_ERROR;
		}
	}
	return id;
}

// _kern_get_disk_device_data
status_t
_kern_get_disk_device_data(partition_id id, bool deviceOnly, bool shadow,
						   user_disk_device_data *buffer, size_t bufferSize,
						   size_t *neededSize)
{
	if (!buffer && bufferSize > 0)
		return B_BAD_VALUE;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the device
	if (KDiskDevice *device = manager->RegisterDevice(id, deviceOnly)) {
		PartitionRegistrar _(device, true);
		if (DeviceReadLocker locker = device) {
			// write the device data into the buffer
			UserDataWriter writer(buffer, bufferSize);
			device->WriteUserData(writer, shadow);
			if (neededSize)
				*neededSize = writer.AllocatedSize();
			if (writer.AllocatedSize() <= bufferSize)
				return B_OK;
			return B_BUFFER_OVERFLOW;
		}
	}
	return B_ENTRY_NOT_FOUND;
}

// _kern_register_file_device
partition_id
_kern_register_file_device(const char *filename)
{
	if (!filename)
		return B_BAD_VALUE;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	if (ManagerLocker locker = manager) {
		if (KFileDiskDevice *device = manager->FindFileDevice(filename))
			return device->ID();
		return manager->CreateFileDevice(filename);
	}
	return B_ERROR;
}

// _kern_unregister_file_device
status_t
_kern_unregister_file_device(partition_id deviceID, const char *filename)
{
	if (deviceID < 0 && !filename)
		return B_BAD_VALUE;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	if (deviceID >= 0)
		return manager->DeleteFileDevice(deviceID);
	return manager->DeleteFileDevice(filename);
}

// _kern_get_disk_system_info
status_t
_kern_get_disk_system_info(disk_system_id id, user_disk_system_info *info)
{
	if (!info)
		return B_BAD_VALUE;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	if (ManagerLocker locker = manager) {
		if (KDiskSystem *diskSystem = manager->FindDiskSystem(id)) {
			DiskSystemLoader _(diskSystem, true);
			diskSystem->GetInfo(info);
			return B_OK;
		}
	}
	return B_ENTRY_NOT_FOUND;
}

// _kern_get_next_disk_system_info
status_t
_kern_get_next_disk_system_info(int32 *cookie, user_disk_system_info *info)
{
	if (!cookie || !info)
		return B_BAD_VALUE;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	if (ManagerLocker locker = manager) {
		if (KDiskSystem *diskSystem = manager->NextDiskSystem(cookie)) {
			DiskSystemLoader _(diskSystem, true);
			diskSystem->GetInfo(info);
			return B_OK;
		}
	}
	return B_ENTRY_NOT_FOUND;
}

// _kern_find_disk_system
status_t
_kern_find_disk_system(const char *name, user_disk_system_info *info)
{
	if (!name || !info)
		return B_BAD_VALUE;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	if (ManagerLocker locker = manager) {
		if (KDiskSystem *diskSystem = manager->FindDiskSystem(name)) {
			DiskSystemLoader _(diskSystem, true);
			diskSystem->GetInfo(info);
			return B_OK;
		}
	}
	return B_ENTRY_NOT_FOUND;
}

// _kern_supports_defragmenting_partition
bool
_kern_supports_defragmenting_partition(disk_system_id diskSystemID,
									   partition_id partitionID,
									   bool *whileMounted)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->ReadLockPartition(partitionID);
	if (!partition)
		return false;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceReadLocker locker(partition->Device(), true);
	if (!check_shadow_partition(partition))
		return false;
	// get the disk system
	KDiskSystem *diskSystem = partition->DiskSystem();
	if (!diskSystem || diskSystem->ID() != diskSystemID)
		return false;
	// get the info
	return diskSystem->SupportsDefragmenting(partition, whileMounted);
}

// _kern_supports_repairing_partition
bool
_kern_supports_repairing_partition(disk_system_id diskSystemID,
								   partition_id partitionID, bool checkOnly,
								   bool *whileMounted)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->ReadLockPartition(partitionID);
	if (!partition)
		return false;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceReadLocker locker(partition->Device(), true);
	if (!check_shadow_partition(partition))
		return false;
	// get the disk system
	KDiskSystem *diskSystem = partition->DiskSystem();
	if (!diskSystem || diskSystem->ID() != diskSystemID)
		return false;
	// get the info
	return diskSystem->SupportsRepairing(partition, checkOnly, whileMounted);
}

// _kern_supports_resizing_partition
bool
_kern_supports_resizing_partition(disk_system_id diskSystemID,
								  partition_id partitionID, bool *whileMounted)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->ReadLockPartition(partitionID);
	if (!partition)
		return false;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceReadLocker locker(partition->Device(), true);
	if (!check_shadow_partition(partition))
		return false;
	// get the disk system
	KDiskSystem *diskSystem = partition->DiskSystem();
	if (!diskSystem || diskSystem->ID() != diskSystemID)
		return false;
	// get the info
	return diskSystem->SupportsResizing(partition, whileMounted);
}

// _kern_supports_resizing_child_partition
bool
_kern_supports_resizing_child_partition(disk_system_id diskSystemID,
										partition_id partitionID)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->ReadLockPartition(partitionID);
	if (!partition)
		return false;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceReadLocker locker(partition->Device(), true);
	if (!check_shadow_partition(partition))
		return false;
	// get the disk system
	if (!partition->Parent())
		return false;
	KDiskSystem *diskSystem = partition->Parent()->DiskSystem();
	if (!diskSystem || diskSystem->ID() != diskSystemID)
		return false;
	// get the info
	return diskSystem->SupportsResizingChild(partition);
}

// _kern_supports_moving_partition
bool
_kern_supports_moving_partition(disk_system_id diskSystemID,
								partition_id partitionID, bool *whileMounted)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->ReadLockPartition(partitionID);
	if (!partition)
		return false;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceReadLocker locker(partition->Device(), true);
	if (!check_shadow_partition(partition))
		return false;
	// get the disk system
	KDiskSystem *diskSystem = partition->DiskSystem();
	if (!diskSystem || diskSystem->ID() != diskSystemID)
		return false;
	// get the info
	return diskSystem->SupportsMoving(partition, whileMounted);
}

// _kern_supports_moving_child_partition
bool
_kern_supports_moving_child_partition(disk_system_id diskSystemID,
									  partition_id partitionID)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->ReadLockPartition(partitionID);
	if (!partition)
		return false;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceReadLocker locker(partition->Device(), true);
	if (!check_shadow_partition(partition))
		return false;
	// get the disk system
	if (!partition->Parent())
		return false;
	KDiskSystem *diskSystem = partition->Parent()->DiskSystem();
	if (!diskSystem || diskSystem->ID() != diskSystemID)
		return false;
	// get the info
	return diskSystem->SupportsMovingChild(partition);
}

// _kern_supports_setting_partition_name
bool
_kern_supports_setting_partition_name(disk_system_id diskSystemID,
									  partition_id partitionID)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->ReadLockPartition(partitionID);
	if (!partition)
		return false;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceReadLocker locker(partition->Device(), true);
	if (!check_shadow_partition(partition))
		return false;
	// get the disk system
	if (!partition->Parent())
		return false;
	KDiskSystem *diskSystem = partition->Parent()->DiskSystem();
	if (!diskSystem || diskSystem->ID() != diskSystemID)
		return false;
	// get the info
	return diskSystem->SupportsSettingName(partition);
}

// _kern_supports_setting_partition_content_name
bool
_kern_supports_setting_partition_content_name(disk_system_id diskSystemID,
											  partition_id partitionID,
											  bool *whileMounted)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->ReadLockPartition(partitionID);
	if (!partition)
		return false;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceReadLocker locker(partition->Device(), true);
	if (!check_shadow_partition(partition))
		return false;
	// get the disk system
	KDiskSystem *diskSystem = partition->DiskSystem();
	if (!diskSystem || diskSystem->ID() != diskSystemID)
		return false;
	// get the info
	return diskSystem->SupportsSettingContentName(partition, whileMounted);
}

// _kern_supports_setting_partition_type
bool
_kern_supports_setting_partition_type(disk_system_id diskSystemID,
									  partition_id partitionID)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->ReadLockPartition(partitionID);
	if (!partition)
		return false;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceReadLocker locker(partition->Device(), true);
	if (!check_shadow_partition(partition))
		return false;
	// get the disk system
	if (!partition->Parent())
		return false;
	KDiskSystem *diskSystem = partition->Parent()->DiskSystem();
	if (!diskSystem || diskSystem->ID() != diskSystemID)
		return false;
	// get the info
	return diskSystem->SupportsSettingType(partition);
}

// _kern_supports_creating_child_partition
bool
_kern_supports_creating_child_partition(disk_system_id diskSystemID,
										partition_id partitionID)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->ReadLockPartition(partitionID);
	if (!partition)
		return false;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceReadLocker locker(partition->Device(), true);
	if (!check_shadow_partition(partition))
		return false;
	// get the disk system
	KDiskSystem *diskSystem = partition->DiskSystem();
	if (!diskSystem || diskSystem->ID() != diskSystemID)
		return false;
	// get the info
	return diskSystem->SupportsCreatingChild(partition);
}

// _kern_supports_deleting_child_partition
bool
_kern_supports_deleting_child_partition(disk_system_id diskSystemID,
										partition_id partitionID)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->ReadLockPartition(partitionID);
	if (!partition)
		return false;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceReadLocker locker(partition->Device(), true);
	if (!check_shadow_partition(partition))
		return false;
	// get the disk system
	if (!partition->Parent())
		return false;
	KDiskSystem *diskSystem = partition->Parent()->DiskSystem();
	if (!diskSystem || diskSystem->ID() != diskSystemID)
		return false;
	// get the info
	return diskSystem->SupportsDeletingChild(partition);
}

// _kern_supports_initializing_partition
bool
_kern_supports_initializing_partition(disk_system_id diskSystemID,
									  partition_id partitionID)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->ReadLockPartition(partitionID);
	if (!partition)
		return false;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceReadLocker locker(partition->Device(), true);
	if (!check_shadow_partition(partition))
		return false;
	// get the disk system
	KDiskSystem *diskSystem = manager->LoadDiskSystem(diskSystemID);
	if (!diskSystem)
		return false;
	DiskSystemLoader loader(diskSystem, true);
	// get the info
	return diskSystem->SupportsInitializing(partition);
}

// _kern_supports_initializing_child_partition
bool
_kern_supports_initializing_child_partition(disk_system_id diskSystemID,
											partition_id partitionID,
											const char *childSystem)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->ReadLockPartition(partitionID);
	if (!partition)
		return false;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceReadLocker locker(partition->Device(), true);
	if (!check_shadow_partition(partition))
		return false;
	// get the disk system
	if (!partition->Parent())
		return false;
	KDiskSystem *diskSystem = partition->Parent()->DiskSystem();
	if (!diskSystem || diskSystem->ID() != diskSystemID)
		return false;
	// get the info
	return diskSystem->SupportsInitializingChild(partition, childSystem);
}

// _kern_prepare_disk_device_modifications
status_t
_kern_prepare_disk_device_modifications(partition_id deviceID)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the device
	if (KDiskDevice *device = manager->RegisterDevice(deviceID, true)) {
		PartitionRegistrar _(device, true);
		if (DeviceWriteLocker locker = device) {
			if (device->ShadowOwner() >= 0)
				return B_BUSY;
			// create shadow device
			return device->CreateShadowDevice(get_current_team());
		}
	}
	return B_ENTRY_NOT_FOUND;
}

// _kern_commit_disk_device_modifications
status_t
_kern_commit_disk_device_modifications(partition_id deviceID, port_id port,
									   int32 token, bool completeProgress)
{
	// not implemented
	return B_ERROR;
}

// _kern_cancel_disk_device_modifications
status_t
_kern_cancel_disk_device_modifications(partition_id deviceID)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the device
	if (KDiskDevice *device = manager->RegisterDevice(deviceID, true)) {
		PartitionRegistrar _(device, true);
		if (DeviceWriteLocker locker = device) {
			if (device->ShadowOwner() != get_current_team())
				return B_BAD_VALUE;
			// delete shadow device
			return device->DeleteShadowDevice();
		}
	}
	return B_ENTRY_NOT_FOUND;
}

// _kern_is_disk_device_modified
bool
_kern_is_disk_device_modified(partition_id device)
{
	// not implemented
	return false;
}

