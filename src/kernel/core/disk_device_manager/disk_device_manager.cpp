// disk_device_manager.cpp

#include "disk_device_manager.h"
#include "KDiskDevice.h"
#include "KDiskDeviceManager.h"
#include "KPartition.h"

// write_lock_disk_device
disk_device_data *
write_lock_disk_device(partition_id partitionID)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	KDiskDevice *device = manager->RegisterDevice(partitionID);
	if (device && device->WriteLock()) {
		device->Unregister();
		return device->DeviceData();
	}
	return NULL;
}

// write_unlock_disk_device
void
write_unlock_disk_device(partition_id partitionID)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	KDiskDevice *device = manager->RegisterDevice(partitionID);
	if (device) {
		device->WriteUnlock();
		device->Unregister();
	}
}

// read_lock_disk_device
disk_device_data *
read_lock_disk_device(partition_id partitionID)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	KDiskDevice *device = manager->RegisterDevice(partitionID);
	if (device && device->ReadLock()) {
		device->Unregister();
		return device->DeviceData();
	}
	return NULL;
}

// read_unlock_disk_device
void
read_unlock_disk_device(partition_id partitionID)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	KDiskDevice *device = manager->RegisterDevice(partitionID);
	if (device) {
		device->ReadUnlock();
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
		device->ID();
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
		partition->ID();
		partition->Unregister();
	}
	return id;
}

// get_disk_device
disk_device_data *
get_disk_device(partition_id partitionID)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	KDiskDevice *device = manager->FindDevice(partitionID);
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
					   partition_id *childID)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	if (KPartition *partition = manager->FindPartition(partitionID)) {
		KPartition *child = NULL;
		if (partition->CreateChild(*childID, index, &child) == B_OK)
			return child->PartitionData();
	}
	return NULL;
}

// delete_partition
bool
delete_partition(partition_id partitionID)
{
	// not implemented
	return false;
}

// partition_modified
void
partition_modified(partition_id partitionID)
{
	// not implemented
}

