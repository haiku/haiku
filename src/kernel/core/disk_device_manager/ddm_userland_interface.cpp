// ddm_userland_interface.cpp

#include <ddm_userland_interface.h>
#include <KDiskDevice.h>
#include <KDiskDeviceJob.h>
#include <KDiskDeviceJobQueue.h>
#include <KDiskDeviceManager.h>
#include <KDiskDeviceUtils.h>
#include <KDiskSystem.h>
#include <KFileDiskDevice.h>
#include <KShadowPartition.h>
#include <malloc.h>
#include <vm.h>

#include "KDiskDeviceJobGenerator.h"
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
check_shadow_partition(const KPartition *partition, int32 changeCounter)
{
	return (partition && partition->Device() && partition->IsShadowPartition()
			&& partition->Device()->ShadowOwner() == get_current_team()
			&& changeCounter == partition->ChangeCounter());
}

// get_unmovable_descendants
static
bool
get_unmovable_descendants(KPartition *partition, partition_id *&unmovable,
						  size_t &unmovableSize, partition_id *&needUnmounting,
						  size_t &needUnmountingSize)
{
	// check parameters
	if (!partition || !unmovable || !needUnmounting || unmovableSize == 0
		|| needUnmountingSize) {
		return false;
	}
	// check partition
	KDiskSystem *diskSystem = partition->DiskSystem();
	bool isNoOp = true;
	bool supports = (diskSystem
					 && diskSystem->SupportsMoving(partition, &isNoOp));
	if (supports) {
		unmovable[0] = partition->ID();
		unmovableSize--;
	}
	if (supports && !isNoOp && diskSystem->IsFileSystem()) {
		needUnmounting[0] = partition->ID();
		needUnmountingSize--;
	}
	// check child partitions
	for (int32 i = 0; KPartition *child = partition->ChildAt(i); i++) {
		if (!get_unmovable_descendants(child, unmovable, unmovableSize,
									   needUnmounting, needUnmountingSize)) {
			return false;
		}
	}
	return true;
}

// validate_move_descendants
static
status_t
validate_move_descendants(KPartition *partition, off_t moveBy,
						  bool markMovable = false)
{
	if (!partition)
		return B_BAD_VALUE;
	// check partition
	bool uninitialized = partition->IsUninitialized();
	KDiskSystem *diskSystem = partition->DiskSystem();
	bool movable = (uninitialized || diskSystem
					|| diskSystem->SupportsMoving(partition, NULL));
	if (markMovable)
		partition->SetAlgorithmData(movable);
	// moving partition is supported in principle, now check the new offset
	if (!uninitialized) {
		if (movable) {
			off_t offset = partition->Offset() + moveBy;
			off_t newOffset = offset;
			if (!diskSystem->ValidateMove(partition, &newOffset)
				|| newOffset != offset) {
				return B_ERROR;
			}
		} else
			return B_ERROR;
		// check children
		for (int32 i = 0; KPartition *child = partition->ChildAt(i); i++) {
			status_t error = validate_move_descendants(child, moveBy);
			if (error != B_OK)
				return error;
		}
	}
	return B_OK;
}

// validate_defragment_partition
static
status_t
validate_defragment_partition(KPartition *partition, int32 changeCounter,
							  bool *whileMounted = NULL)
{
	if (!partition || !check_shadow_partition(partition, changeCounter))
		return B_BAD_VALUE;
	if (partition->IsBusy() || partition->IsDescendantBusy())
		return B_BUSY;
	// get the disk system and get the info
	KDiskSystem *diskSystem = partition->DiskSystem();
	if (!diskSystem)
		return B_ENTRY_NOT_FOUND;
	if (diskSystem->SupportsDefragmenting(partition, whileMounted))
		return B_OK;
	return B_ERROR;
}

// validate_repair_partition
static
status_t
validate_repair_partition(KPartition *partition, int32 changeCounter,
						  bool checkOnly, bool *whileMounted = NULL)
{
	if (!partition || !check_shadow_partition(partition, changeCounter))
		return B_BAD_VALUE;
	if (partition->IsBusy() || partition->IsDescendantBusy())
		return B_BUSY;
	// get the disk system and get the info
	KDiskSystem *diskSystem = partition->DiskSystem();
	if (!diskSystem)
		return B_ENTRY_NOT_FOUND;
	if (diskSystem->SupportsRepairing(partition, checkOnly, whileMounted))
		return B_OK;
	return B_ERROR;
}

// validate_resize_partition
static
status_t
validate_resize_partition(KPartition *partition, int32 changeCounter,
						  off_t *size)
{
	if (!partition || !size
		|| !check_shadow_partition(partition, changeCounter)
		|| !partition->Parent()) {
		return B_BAD_VALUE;
	}
	if (partition->Parent()->IsBusy()
		|| partition->Parent()->IsDescendantBusy()) {
		return B_BUSY;
	}
	// get the parent disk system and let it check the value
	KDiskSystem *parentDiskSystem = partition->Parent()->DiskSystem();
	if (!parentDiskSystem)
		return B_ENTRY_NOT_FOUND;
	if (!parentDiskSystem->ValidateResizeChild(partition, size))
		return B_ERROR;
	// if contents is uninitialized, then there's no need to check anything
	// more
	if (partition->IsUninitialized())
		return B_OK;
	// get the child disk system and let it check the value
	KDiskSystem *childDiskSystem = partition->DiskSystem();
	if (!childDiskSystem)
		return B_ENTRY_NOT_FOUND;
	off_t childSize = *size;
	// don't worry, if the content system desires to be smaller
	if (!childDiskSystem->ValidateResize(partition, &childSize)
		|| childSize > *size) {
		return B_ERROR;
	}
	return B_OK;
}

// validate_move_partition
status_t
validate_move_partition(KPartition *partition, int32 changeCounter,
						off_t *newOffset, bool markMovable = false)
{
	if (!partition || !newOffset
		|| !check_shadow_partition(partition, changeCounter)
		|| !partition->Parent()) {
		return B_BAD_VALUE;
	}
	if (partition->Parent()->IsBusy()
		|| partition->Parent()->IsDescendantBusy()) {
		return B_BUSY;
	}
	// get the parent disk system and let it check the value
	KDiskSystem *parentDiskSystem = partition->Parent()->DiskSystem();
	if (!parentDiskSystem)
		return B_ENTRY_NOT_FOUND;
	if (!parentDiskSystem->ValidateMoveChild(partition, newOffset))
		return B_ERROR;
	// let the concerned content disk systems check the value
	return validate_move_descendants(partition,
		partition->Offset() - *newOffset, markMovable);
}

// move_descendants
static
void
move_descendants(KPartition *partition, off_t moveBy)
{
	if (!partition)
		return;
	partition->SetOffset(partition->Offset() + moveBy);
	// move children
	for (int32 i = 0; KPartition *child = partition->ChildAt(i); i++)
		move_descendants(child, moveBy);
}

// move_descendants_contents
static
status_t
move_descendants_contents(KPartition *partition)
{
	if (!partition)
		return B_BAD_VALUE;
	// implicit content disk system changes
	KDiskSystem *diskSystem = partition->DiskSystem();
	if (diskSystem || partition->AlgorithmData()) {
		status_t error = diskSystem->ShadowPartitionChanged(partition,
			B_PARTITION_RESIZE);
		if (error != B_OK)
			return error;
	}
	// move children's contents
	for (int32 i = 0; KPartition *child = partition->ChildAt(i); i++) {
		status_t error = move_descendants_contents(child);
		if (error != B_OK)
			return error;
	}
	return B_OK;
}

// validate_set_partition_name
static
status_t
validate_set_partition_name(KPartition *partition, int32 changeCounter,
							char *name)
{
	if (!partition || !name)
		return B_BAD_VALUE;
	// truncate name to maximal size
	name[B_OS_NAME_LENGTH] = '\0';
	// check the partition
	if (!check_shadow_partition(partition, changeCounter)
		|| !partition->Parent()) {
		return B_BAD_VALUE;
	}
	if (partition->Parent()->IsBusy()
		|| partition->Parent()->IsDescendantBusy()) {
		return B_BUSY;
	}
	// get the disk system
	KDiskSystem *diskSystem = partition->Parent()->DiskSystem();
	if (!diskSystem)
		return B_ENTRY_NOT_FOUND;
	// get the info
	if (diskSystem->ValidateSetName(partition, name))
		return B_OK;
	return B_ERROR;
}

// validate_set_partition_content_name
static
status_t
validate_set_partition_content_name(KPartition *partition, int32 changeCounter,
									char *name)
{
	if (!partition || !name)
		return B_BAD_VALUE;
	// truncate name to maximal size
	name[B_OS_NAME_LENGTH] = '\0';
	// check the partition
	if (!check_shadow_partition(partition, changeCounter))
		return B_BAD_VALUE;
	if (partition->IsBusy() || partition->IsDescendantBusy())
		return B_BUSY;
	// get the disk system
	KDiskSystem *diskSystem = partition->DiskSystem();
	if (!diskSystem)
		return B_ENTRY_NOT_FOUND;
	// get the info
	if (diskSystem->ValidateSetContentName(partition, name))
		return B_OK;
	return B_ERROR;
}

// validate_set_partition_type
static
status_t
validate_set_partition_type(KPartition *partition, int32 changeCounter,
							const char *type)
{
	if (!partition || !type)
		return B_BAD_VALUE;
	// check the partition
	if (!check_shadow_partition(partition, changeCounter)
		|| !partition->Parent()) {
		return B_BAD_VALUE;
	}
	if (partition->Parent()->IsBusy()
		|| partition->Parent()->IsDescendantBusy()) {
		return B_BUSY;
	}
	// get the disk system
	KDiskSystem *diskSystem = partition->Parent()->DiskSystem();
	if (!diskSystem)
		return B_ENTRY_NOT_FOUND;
	// get the info
	if (diskSystem->ValidateSetType(partition, type))
		return B_OK;
	return B_ERROR;
}

// validate_set_partition_parameters
static
status_t
validate_set_partition_parameters(KPartition *partition, int32 changeCounter,
								  const char *parameters)
{
	if (!partition || !parameters)
		return B_BAD_VALUE;
	// check the partition
	if (!check_shadow_partition(partition, changeCounter)
		|| !partition->Parent()) {
		return B_BAD_VALUE;
	}
	if (partition->Parent()->IsBusy()
		|| partition->Parent()->IsDescendantBusy()) {
		return B_BUSY;
	}
	// get the disk system
	KDiskSystem *diskSystem = partition->Parent()->DiskSystem();
	if (!diskSystem)
		return B_ENTRY_NOT_FOUND;
	// get the info
	if (diskSystem->ValidateSetParameters(partition, parameters))
		return B_OK;
	return B_ERROR;
}

// validate_set_partition_content_parameters
static
status_t
validate_set_partition_content_parameters(KPartition *partition,
										  int32 changeCounter,
										  const char *parameters)
{
	if (!partition)
		return B_BAD_VALUE;
	// check the partition
	if (!check_shadow_partition(partition, changeCounter))
		return B_BAD_VALUE;
	if (partition->IsBusy() || partition->IsDescendantBusy())
		return B_BUSY;
	// get the disk system
	KDiskSystem *diskSystem = partition->DiskSystem();
	if (!diskSystem)
		return B_ENTRY_NOT_FOUND;
	// get the info
	if (diskSystem->ValidateSetContentParameters(partition, parameters))
		return B_OK;
	return B_ERROR;
}

// validate_initialize_partition
static
status_t
validate_initialize_partition(KPartition *partition, int32 changeCounter,
							  const char *diskSystemName, char *name,
							  const char *parameters)
{
	if (!partition || !diskSystemName || !name)
		return B_BAD_VALUE;
	// truncate name to maximal size
	name[B_OS_NAME_LENGTH] = '\0';
	// check the partition
	if (!check_shadow_partition(partition, changeCounter))
		return B_BAD_VALUE;
	if (partition->IsBusy() || partition->IsDescendantBusy())
		return B_BUSY;
	// get the disk system
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	KDiskSystem *diskSystem = manager->LoadDiskSystem(diskSystemName);
	if (!diskSystem)
		return B_ENTRY_NOT_FOUND;
	DiskSystemLoader loader(diskSystem, true);
	// get the info
	if (diskSystem->ValidateInitialize(partition, name, parameters))
		return B_OK;
	return B_ERROR;
}

// validate_create_child_partition
static
status_t
validate_create_child_partition(KPartition *partition, int32 changeCounter,
								off_t *offset, off_t *size, const char *type,
								const char *parameters, int32 *index = NULL)
{
	if (!partition || !offset || !size || !type)
		return B_BAD_VALUE;
	// check the partition
	if (!check_shadow_partition(partition, changeCounter))
		return B_BAD_VALUE;
	if (partition->IsBusy() || partition->IsDescendantBusy())
		return B_BUSY;
	// get the disk system
	KDiskSystem *diskSystem = partition->DiskSystem();
	if (!diskSystem)
		return B_ENTRY_NOT_FOUND;
	// get the info
	if (diskSystem->ValidateCreateChild(partition, offset, size, type,
										parameters, index)) {
		return B_OK;
	}
	return B_ERROR;
}

// validate_delete_child_partition
static
status_t
validate_delete_child_partition(KPartition *partition, int32 changeCounter)
{
	if (!partition)
		return B_BAD_VALUE;
	// check the partition
	if (!check_shadow_partition(partition, changeCounter)
		|| !partition->Parent()) {
		return B_BAD_VALUE;
	}
	if (partition->Parent()->IsBusy()
		|| partition->Parent()->IsDescendantBusy()) {
		return B_BUSY;
	}
	// get the disk system
	KDiskSystem *diskSystem = partition->Parent()->DiskSystem();
	if (!diskSystem)
		return B_ENTRY_NOT_FOUND;
	// get the info
	if (diskSystem->SupportsDeletingChild(partition))
		return B_OK;
	return B_ERROR;
}

// _kern_get_next_disk_device_id
partition_id
_kern_get_next_disk_device_id(int32 *_cookie, size_t *neededSize)
{
	if (!_cookie)
		return B_BAD_VALUE;
	int32 cookie = *_cookie;
	
	partition_id id = B_ENTRY_NOT_FOUND;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the next device
	if (KDiskDevice *device = manager->RegisterNextDevice(&cookie)) {
		PartitionRegistrar _(device, true);
		id = device->ID();
		if (neededSize) {
			if (DeviceReadLocker locker = device) {
				// get the needed size
				UserDataWriter writer;
				device->WriteUserData(writer, false);
				*neededSize = writer.AllocatedSize();
			} else {
				id = B_ERROR;
			}
		}
	}
	*_cookie = cookie;
	return id;
}

// _kern_find_disk_device
partition_id
_kern_find_disk_device(const char *_filename, size_t *neededSize)
{
	if (!_filename)
		return B_BAD_VALUE;

	char filename[SYS_MAX_PATH_LEN+1];
	if (user_strlcpy(filename, _filename, SYS_MAX_PATH_LEN) >= SYS_MAX_PATH_LEN) 		
		return B_NAME_TOO_LONG;
	
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
_kern_find_partition(const char *_filename, size_t *neededSize)
{
	if (!_filename)
		return B_BAD_VALUE;

	char filename[SYS_MAX_PATH_LEN+1];
	if (user_strlcpy(filename, _filename, SYS_MAX_PATH_LEN) >= SYS_MAX_PATH_LEN) 		
		return B_NAME_TOO_LONG;

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
						   user_disk_device_data *_buffer, size_t bufferSize,
						   size_t *neededSize)
{
	if (!_buffer && bufferSize > 0)
		return B_BAD_VALUE;

	// copy in
	user_disk_device_data *buffer = bufferSize > 0
	                                ? reinterpret_cast<user_disk_device_data*>(malloc(bufferSize))
	                                : NULL;
	if (buffer)
		user_memcpy(buffer, _buffer, bufferSize);
		
	status_t result = B_OK;	
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
			if (writer.AllocatedSize() > bufferSize)
				result = B_BUFFER_OVERFLOW;
		}
	} else {
		result = B_ENTRY_NOT_FOUND;
	}

	// copy out
	if (result == B_OK && buffer)
		user_memcpy(_buffer, buffer, bufferSize);
	free(buffer);
	
	return result;
}

// _kern_get_partitionable_spaces
status_t
_kern_get_partitionable_spaces(partition_id partitionID, int32 changeCounter,
							   partitionable_space_data *_buffer,
							   int32 count, int32 *_actualCount)
{
	if (!_buffer && count > 0)
		return B_BAD_VALUE;	
	// copy in
	int32 bufferSize = count * sizeof(partitionable_space_data);
	partitionable_space_data *buffer = count > 0
	                                   ? reinterpret_cast<partitionable_space_data*>(malloc(bufferSize))
	                                   : NULL;
	if (buffer)
		user_memcpy(buffer, _buffer, bufferSize);	
	status_t error = B_OK;
	// get the partition
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	KPartition *partition = manager->ReadLockPartition(partitionID);
	error = partition ? B_OK : B_ENTRY_NOT_FOUND;
	if (!error) {
		PartitionRegistrar registrar1(partition, true);
		PartitionRegistrar registrar2(partition->Device(), true);
		DeviceReadLocker locker(partition->Device(), true);
		error = check_shadow_partition(partition, changeCounter) ? B_OK : B_BAD_VALUE;
		if (!error) {
			// get the disk system
			KDiskSystem *diskSystem = partition->DiskSystem();
			error = diskSystem ? B_OK : B_ENTRY_NOT_FOUND;
			if (!error) {
				// get the info
				int32 actualCount;
				error = diskSystem->GetPartitionableSpaces(partition, buffer,
				                                           count, &actualCount);
				if (!error && _actualCount)
					*_actualCount = actualCount;
			}
		}
	}	
	// copy out
	if (!error && buffer)
		user_memcpy(_buffer, buffer, bufferSize);
	free(buffer);	
	return error;
}

// _kern_register_file_device
partition_id
_kern_register_file_device(const char *_filename)
{
	if (!_filename)
		return B_BAD_VALUE;
	char filename[B_PATH_NAME_LENGTH];
	if (user_strlcpy(filename, _filename, B_PATH_NAME_LENGTH) >= B_PATH_NAME_LENGTH)
		return B_NAME_TOO_LONG;
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
_kern_unregister_file_device(partition_id deviceID, const char *_filename)
{
	if (deviceID < 0 && !_filename)
		return B_BAD_VALUE;	
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	if (deviceID >= 0) {
		return manager->DeleteFileDevice(deviceID);
	} else {
		char filename[B_PATH_NAME_LENGTH];
		if (user_strlcpy(filename, _filename, B_PATH_NAME_LENGTH) >= B_PATH_NAME_LENGTH)
			return B_NAME_TOO_LONG;
		return manager->DeleteFileDevice(filename);
	}
}

// _kern_get_disk_system_info
status_t
_kern_get_disk_system_info(disk_system_id id, user_disk_system_info *_info)
{
	if (!_info)
		return B_BAD_VALUE;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	if (ManagerLocker locker = manager) {
		if (KDiskSystem *diskSystem = manager->FindDiskSystem(id)) {
			DiskSystemLoader _(diskSystem, true);
			user_disk_system_info info;
			diskSystem->GetInfo(&info);
			user_memcpy(_info, &info, sizeof(info));
			return B_OK;
		}
	}
	return B_ENTRY_NOT_FOUND;
}

// _kern_get_next_disk_system_info
status_t
_kern_get_next_disk_system_info(int32 *_cookie, user_disk_system_info *_info)
{
	if (!_cookie || !_info)
		return B_BAD_VALUE;
	int32 cookie = *_cookie;
	status_t result = B_ENTRY_NOT_FOUND;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	if (ManagerLocker locker = manager) {
		if (KDiskSystem *diskSystem = manager->NextDiskSystem(&cookie)) {
			DiskSystemLoader _(diskSystem, true);
			user_disk_system_info info;
			diskSystem->GetInfo(&info);
			user_memcpy(_info, &info, sizeof(info));
			result = B_OK;
		}
	}
	*_cookie = cookie;
	return result;
}

// _kern_find_disk_system
status_t
_kern_find_disk_system(const char *_name, user_disk_system_info *_info)
{
	if (!_name || !_info)
		return B_BAD_VALUE;
	char name[B_OS_NAME_LENGTH];
	if (user_strlcpy(name, _name, B_OS_NAME_LENGTH) >= B_OS_NAME_LENGTH)
		return B_NAME_TOO_LONG;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	if (ManagerLocker locker = manager) {
		if (KDiskSystem *diskSystem = manager->FindDiskSystem(name)) {
			DiskSystemLoader _(diskSystem, true);
			user_disk_system_info info;
			diskSystem->GetInfo(&info);
			user_memcpy(_info, &info, sizeof(info));
			return B_OK;
		}
	}
	return B_ENTRY_NOT_FOUND;
}

// _kern_supports_defragmenting_partition
bool
_kern_supports_defragmenting_partition(partition_id partitionID,
									   int32 changeCounter, bool *_whileMounted)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->ReadLockPartition(partitionID);
	if (!partition)
		return false;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceReadLocker locker(partition->Device(), true);
	bool whileMounted;
	bool result = validate_defragment_partition(partition, changeCounter,
										        &whileMounted) == B_OK;
	if (result && _whileMounted)
		*_whileMounted = whileMounted;
	return result;										   
}

// _kern_supports_repairing_partition
bool
_kern_supports_repairing_partition(partition_id partitionID,
								   int32 changeCounter, bool checkOnly,
								   bool *_whileMounted)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->ReadLockPartition(partitionID);
	if (!partition)
		return false;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceReadLocker locker(partition->Device(), true);
	bool whileMounted;
	bool result = validate_repair_partition(partition, changeCounter, checkOnly,
									        &whileMounted) == B_OK;
	if (result && _whileMounted)
		*_whileMounted = whileMounted;
	return result;										   
}

// _kern_supports_resizing_partition
bool
_kern_supports_resizing_partition(partition_id partitionID,
								  int32 changeCounter, bool *canResizeContents,
								  bool *_whileMounted)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->ReadLockPartition(partitionID);
	if (!partition)
		return false;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceReadLocker locker(partition->Device(), true);
	if (!check_shadow_partition(partition, changeCounter)
		|| !partition->Parent()) {
		return false;
	}
	if (partition->Parent()->IsBusy()
		|| partition->Parent()->IsDescendantBusy()) {
		return false;
	}
	// get the parent disk system
	KDiskSystem *parentDiskSystem = partition->Parent()->DiskSystem();
	if (!parentDiskSystem)
		return false;
	bool result = parentDiskSystem->SupportsResizingChild(partition);
	if (!result)
		return false;
	// get the child disk system
	KDiskSystem *childDiskSystem = partition->DiskSystem();
	if (canResizeContents) {
		bool whileMounted;
		*canResizeContents = (childDiskSystem
			&& childDiskSystem->SupportsResizing(partition, &whileMounted));
		if (_whileMounted)
			*_whileMounted = whileMounted;
	}
// TODO: Currently we report that we cannot resize the contents, if the
// partition's disk system is unknown. I found this more logical. It doesn't
// really matter, though, since the API user can check for this situation.
	return result;
}

// ToDo: Add parameter security
// _kern_supports_moving_partition
bool
_kern_supports_moving_partition(partition_id partitionID, int32 changeCounter,
								partition_id *unmovable,
								partition_id *needUnmounting,
								size_t bufferSize)
{
	if ((!unmovable || needUnmounting) && bufferSize > 0)
		return false;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->ReadLockPartition(partitionID);
	if (!partition)
		return false;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceReadLocker locker(partition->Device(), true);
	if (!check_shadow_partition(partition, changeCounter)
		|| !partition->Parent()) {
		return false;
	}
	if (partition->Parent()->IsBusy()
		|| partition->Parent()->IsDescendantBusy()) {
		return false;
	}
	// get the parent disk system
	KDiskSystem *parentDiskSystem = partition->Parent()->DiskSystem();
	if (!parentDiskSystem)
		return false;
	bool result = parentDiskSystem->SupportsMovingChild(partition);
	if (!result)
		return false;
	// check the movability of the descendants' contents
	size_t unmovableSize = bufferSize; 
	size_t needUnmountingSize = bufferSize; 
	if (!get_unmovable_descendants(partition, unmovable, unmovableSize,
								   needUnmounting, needUnmountingSize)) {
		return false;
	}
	return result;
}

// _kern_supports_setting_partition_name
bool
_kern_supports_setting_partition_name(partition_id partitionID,
									  int32 changeCounter)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->ReadLockPartition(partitionID);
	if (!partition)
		return false;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceReadLocker locker(partition->Device(), true);
	if (!check_shadow_partition(partition, changeCounter)
		|| !partition->Parent()) {
		return false;
	}
	if (partition->Parent()->IsBusy()
		|| partition->Parent()->IsDescendantBusy()) {
		return false;
	}
	// get the disk system
	KDiskSystem *diskSystem = partition->Parent()->DiskSystem();
	if (!diskSystem)
		return false;
	// get the info
	return diskSystem->SupportsSettingName(partition);
}

// _kern_supports_setting_partition_content_name
bool
_kern_supports_setting_partition_content_name(partition_id partitionID,
											  int32 changeCounter,
											  bool *_whileMounted)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->ReadLockPartition(partitionID);
	if (!partition)
		return false;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceReadLocker locker(partition->Device(), true);
	if (!check_shadow_partition(partition, changeCounter))
		return false;
	if (partition->IsBusy() || partition->IsDescendantBusy())
		return false;
	// get the disk system
	KDiskSystem *diskSystem = partition->DiskSystem();
	if (!diskSystem)
		return false;
	// get the info
	bool whileMounted;
	bool result = diskSystem->SupportsSettingContentName(partition, &whileMounted);
	if (result && _whileMounted)
		*_whileMounted = whileMounted;
	return result;
}

// _kern_supports_setting_partition_type
bool
_kern_supports_setting_partition_type(partition_id partitionID,
									  int32 changeCounter)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->ReadLockPartition(partitionID);
	if (!partition)
		return false;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceReadLocker locker(partition->Device(), true);
	if (!check_shadow_partition(partition, changeCounter)
		|| !partition->Parent()) {
		return false;
	}
	if (partition->Parent()->IsBusy()
		|| partition->Parent()->IsDescendantBusy()) {
		return false;
	}
	// get the disk system
	KDiskSystem *diskSystem = partition->Parent()->DiskSystem();
	if (!diskSystem)
		return false;
	// get the info
	return diskSystem->SupportsSettingType(partition);
}

// _kern_supports_setting_partition_parameters
bool
_kern_supports_setting_partition_parameters(partition_id partitionID,
											int32 changeCounter)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->ReadLockPartition(partitionID);
	if (!partition)
		return false;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceReadLocker locker(partition->Device(), true);
	if (!check_shadow_partition(partition, changeCounter)
		|| !partition->Parent()) {
		return false;
	}
	if (partition->Parent()->IsBusy()
		|| partition->Parent()->IsDescendantBusy()) {
		return false;
	}
	// get the disk system
	KDiskSystem *diskSystem = partition->Parent()->DiskSystem();
	if (!diskSystem)
		return false;
	// get the info
	return diskSystem->SupportsSettingParameters(partition);
}

// _kern_supports_setting_partition_content_parameters
bool
_kern_supports_setting_partition_content_parameters(partition_id partitionID,
													int32 changeCounter,
													bool *_whileMounted)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->ReadLockPartition(partitionID);
	if (!partition)
		return false;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceReadLocker locker(partition->Device(), true);
	if (!check_shadow_partition(partition, changeCounter))
		return false;
	if (partition->IsBusy() || partition->IsDescendantBusy())
		return false;
	// get the disk system
	KDiskSystem *diskSystem = partition->DiskSystem();
	if (!diskSystem)
		return false;
	// get the info
	bool whileMounted;
	bool result = diskSystem->SupportsSettingContentParameters(partition,
														       &whileMounted);
	if (result && _whileMounted)
		*_whileMounted = whileMounted;
	return result;
}

// _kern_supports_initializing_partition
bool
_kern_supports_initializing_partition(partition_id partitionID,
									  int32 changeCounter,
									  const char *_diskSystemName)
{
	if (_diskSystemName)
		return false;
	char diskSystemName[B_OS_NAME_LENGTH];
	if (user_strlcpy(diskSystemName, _diskSystemName, B_OS_NAME_LENGTH) >= B_OS_NAME_LENGTH)
		return false;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->ReadLockPartition(partitionID);
	if (!partition)
		return false;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceReadLocker locker(partition->Device(), true);
	if (!check_shadow_partition(partition, changeCounter))
		return false;
	if (partition->IsBusy() || partition->IsDescendantBusy())
		return false;
	// get the disk system
	KDiskSystem *diskSystem = manager->LoadDiskSystem(diskSystemName);
	if (!diskSystem)
		return false;
	DiskSystemLoader loader(diskSystem, true);
	// get the info
	return diskSystem->SupportsInitializing(partition);
// TODO: Ask the parent partitioning system as well.
}

// _kern_supports_creating_child_partition
bool
_kern_supports_creating_child_partition(partition_id partitionID,
										int32 changeCounter)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->ReadLockPartition(partitionID);
	if (!partition)
		return false;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceReadLocker locker(partition->Device(), true);
	if (!check_shadow_partition(partition, changeCounter))
		return false;
	if (partition->IsBusy() || partition->IsDescendantBusy())
		return false;
	// get the disk system
	KDiskSystem *diskSystem = partition->DiskSystem();
	if (!diskSystem)
		return false;
	// get the info
	return diskSystem->SupportsCreatingChild(partition);
}

// _kern_supports_deleting_child_partition
bool
_kern_supports_deleting_child_partition(partition_id partitionID,
										int32 changeCounter)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->ReadLockPartition(partitionID);
	if (!partition)
		return false;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceReadLocker locker(partition->Device(), true);
	return (validate_delete_child_partition(partition, changeCounter) == B_OK);
}

// _kern_is_sub_disk_system_for
bool
_kern_is_sub_disk_system_for(disk_system_id diskSystemID,
							 partition_id partitionID, int32 changeCounter)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->ReadLockPartition(partitionID);
	if (!partition)
		return false;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceReadLocker locker(partition->Device(), true);
	if (!check_shadow_partition(partition, changeCounter))
		return false;
	// get the disk system
	KDiskSystem *diskSystem = partition->DiskSystem();
	if (!diskSystem || diskSystem->ID() != diskSystemID)
		return false;
	// get the info
	return diskSystem->IsSubSystemFor(partition);
}

// _kern_validate_resize_partition
status_t
_kern_validate_resize_partition(partition_id partitionID, int32 changeCounter,
								off_t *_size)
{
	if (!_size)
		return B_BAD_VALUE;
	off_t size = *_size;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->ReadLockPartition(partitionID);
	if (!partition)
		return B_ENTRY_NOT_FOUND;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceReadLocker locker(partition->Device(), true);
	bool result = validate_resize_partition(partition, changeCounter, &size);
	if (result)
		*_size = size;
	return result;								        
}

// _kern_validate_move_partition
status_t
_kern_validate_move_partition(partition_id partitionID, int32 changeCounter,
							  off_t *newOffset)
{
	if (!newOffset)
		return B_BAD_VALUE;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->ReadLockPartition(partitionID);
	if (!partition)
		return B_ENTRY_NOT_FOUND;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceReadLocker locker(partition->Device(), true);
	return validate_move_partition(partition, changeCounter, newOffset);
}

// _kern_validate_set_partition_name
status_t
_kern_validate_set_partition_name(partition_id partitionID,
								  int32 changeCounter, char *name)
{
	if (!name)
		return B_BAD_VALUE;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->ReadLockPartition(partitionID);
	if (!partition)
		return B_ENTRY_NOT_FOUND;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceReadLocker locker(partition->Device(), true);
	return validate_set_partition_name(partition, changeCounter, name);
}

// _kern_validate_set_partition_content_name
status_t
_kern_validate_set_partition_content_name(partition_id partitionID,
										  int32 changeCounter, char *name)
{
	if (!name)
		return B_BAD_VALUE;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->ReadLockPartition(partitionID);
	if (!partition)
		return B_ENTRY_NOT_FOUND;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceReadLocker locker(partition->Device(), true);
	return validate_set_partition_content_name(partition, changeCounter, name);
}

// _kern_validate_set_partition_type
status_t
_kern_validate_set_partition_type(partition_id partitionID,
								  int32 changeCounter, const char *type)
{
	if (!type)
		return B_BAD_VALUE;
	if (strnlen(type, B_OS_NAME_LENGTH) == B_OS_NAME_LENGTH)
		return B_NAME_TOO_LONG;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->ReadLockPartition(partitionID);
	if (!partition)
		return B_ENTRY_NOT_FOUND;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceReadLocker locker(partition->Device(), true);
	return validate_set_partition_type(partition, changeCounter, type);
}

// _kern_validate_initialize_partition
status_t
_kern_validate_initialize_partition(partition_id partitionID,
									int32 changeCounter,
									const char *diskSystemName, char *name,
									const char *parameters)
{
	if (!diskSystemName || !name)
		return B_BAD_VALUE;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->ReadLockPartition(partitionID);
	if (!partition)
		return B_ENTRY_NOT_FOUND;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceReadLocker locker(partition->Device(), true);
	return validate_initialize_partition(partition, changeCounter,
										 diskSystemName, name, parameters);
}

// _kern_validate_create_child_partition
status_t
_kern_validate_create_child_partition(partition_id partitionID,
									  int32 changeCounter, off_t *offset,
									  off_t *size, const char *type,
									  const char *parameters)
{
	if (!offset || !size || !type)
		return B_BAD_VALUE;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->ReadLockPartition(partitionID);
	if (!partition)
		return B_ENTRY_NOT_FOUND;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceReadLocker locker(partition->Device(), true);
	return validate_create_child_partition(partition, changeCounter, offset,
										   size, type, parameters);
}

// _kern_get_next_supported_partition_type
status_t
_kern_get_next_supported_partition_type(partition_id partitionID,
										int32 changeCounter, int32 *cookie,
										char *type)
{
	if (!cookie || !type)
		return B_BAD_VALUE;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->ReadLockPartition(partitionID);
	if (!partition)
		return B_ENTRY_NOT_FOUND;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceReadLocker locker(partition->Device(), true);
	if (!check_shadow_partition(partition, changeCounter))
		return B_BAD_VALUE;
	// get the disk system
	KDiskSystem *diskSystem = partition->DiskSystem();
	if (!diskSystem)
		return B_ENTRY_NOT_FOUND;
	// get the info
	if (diskSystem->GetNextSupportedType(partition, cookie, type))
		return B_OK;
	return B_ERROR;
}

// _kern_get_partition_type_for_content_type
status_t
_kern_get_partition_type_for_content_type(disk_system_id diskSystemID,
										  const char *contentType, char *type)
{
	if (!contentType || !type)
		return B_BAD_VALUE;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the disk system
	KDiskSystem *diskSystem = manager->LoadDiskSystem(diskSystemID);
	if (!diskSystem)
		return false;
	DiskSystemLoader loader(diskSystem, true);
	// get the info
	if (diskSystem->GetTypeForContentType(contentType, type))
		return B_OK;
	return B_ERROR;
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
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the device
	if (KDiskDevice *device = manager->RegisterDevice(deviceID, true)) {
		PartitionRegistrar _(device, true);
		if (DeviceWriteLocker locker = device) {
			if (device->ShadowOwner() != get_current_team())
				return B_BAD_VALUE;
			// generate the jobs
			KDiskDeviceJobGenerator generator(manager->JobFactory(), device);
			status_t error = generator.GenerateJobs();
			if (error != B_OK)
				return error;
			// add the jobs to the manager
			if (ManagerLocker locker2 = manager) {
				error = manager->AddJobQueue(generator.JobQueue());
				if (error == B_OK)
					generator.DetachJobQueue();
				else
					return error;
			} else
				return B_ERROR;
			// TODO: notification stuff
			return device->DeleteShadowDevice();
		}
	}
	return B_ENTRY_NOT_FOUND;
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
_kern_is_disk_device_modified(partition_id deviceID)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the device
	if (KDiskDevice *device = manager->RegisterDevice(deviceID, true)) {
		PartitionRegistrar _(device, true);
		if (DeviceReadLocker locker = device) {
			// check whether there's a shadow device and whether it's changed
			return (device->ShadowOwner() == get_current_team()
					&& device->ShadowPartition()
					&& device->ShadowPartition()->ChangeFlags() != 0);
		}
	}
	return B_ENTRY_NOT_FOUND;
}

// _kern_defragment_partition
status_t
_kern_defragment_partition(partition_id partitionID, int32 changeCounter)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->WriteLockPartition(partitionID);
	if (!partition)
		return B_ENTRY_NOT_FOUND;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceWriteLocker locker(partition->Device(), true);
	// check whether the disk system supports defragmenting
	status_t error = validate_defragment_partition(partition, changeCounter);
	if (error != B_OK)
		return error;
	// set the defragmenting flag
	partition->Changed(B_PARTITION_CHANGED_DEFRAGMENTATION);
	return B_OK;
}

// _kern_repair_partition
status_t
_kern_repair_partition(partition_id partitionID, int32 changeCounter,
					   bool checkOnly)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->WriteLockPartition(partitionID);
	if (!partition)
		return B_ENTRY_NOT_FOUND;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceWriteLocker locker(partition->Device(), true);
	// check whether the disk system supports defragmenting
	status_t error = validate_repair_partition(partition, changeCounter,
											   checkOnly);
	if (error != B_OK)
		return error;
	// set the respective flag
	if (checkOnly)
		partition->Changed(B_PARTITION_CHANGED_CHECK);
	else
		partition->Changed(B_PARTITION_CHANGED_REPAIR);
	return B_OK;
}

// _kern_resize_partition
status_t
_kern_resize_partition(partition_id partitionID, int32 changeCounter,
					   off_t size)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->WriteLockPartition(partitionID);
	if (!partition)
		return B_ENTRY_NOT_FOUND;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceWriteLocker locker(partition->Device(), true);
	// check the size
	if (size == partition->Size())
		return B_OK;
	off_t proposedSize = size;
	status_t error = validate_resize_partition(partition, changeCounter,
											   &proposedSize);
	if (error != B_OK)
		return error;
	if (proposedSize != size)
		return B_BAD_VALUE;
	// new size is fine -- resize the thing
	partition->SetSize(size);
	partition->Changed(B_PARTITION_CHANGED_SIZE);
	// implicit partitioning system changes
	error = partition->Parent()->DiskSystem()->ShadowPartitionChanged(
		partition, B_PARTITION_RESIZE_CHILD);
	if (error != B_OK)
		return error;
	// implicit content disk system changes
	if (partition->DiskSystem()) {
		error = partition->DiskSystem()->ShadowPartitionChanged(
			partition, B_PARTITION_RESIZE);
	}
	return error;
}

// _kern_move_partition
status_t
_kern_move_partition(partition_id partitionID, int32 changeCounter,
					 off_t newOffset)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->WriteLockPartition(partitionID);
	if (!partition)
		return B_ENTRY_NOT_FOUND;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceWriteLocker locker(partition->Device(), true);
	// check the new offset
	if (newOffset == partition->Offset())
		return B_OK;
	off_t proposedOffset = newOffset;
	status_t error = validate_move_partition(partition, changeCounter,
											 &proposedOffset, true);
	if (error != B_OK)
		return error;
	if (proposedOffset != newOffset)
		return B_BAD_VALUE;
	// new offset is fine -- move the thing
	off_t moveBy = newOffset - partition->Offset();
	move_descendants(partition, moveBy);
	partition->Changed(B_PARTITION_CHANGED_OFFSET);
	// implicit partitioning system changes
	error = partition->Parent()->DiskSystem()->ShadowPartitionChanged(
		partition, B_PARTITION_MOVE_CHILD);
	if (error != B_OK)
		return error;
	// implicit descendants' content disk system changes
	return move_descendants_contents(partition);
}

// _kern_set_partition_name
status_t
_kern_set_partition_name(partition_id partitionID, int32 changeCounter,
						 const char *name)
{
	if (!name)
		return B_BAD_VALUE;
	if (strnlen(name, B_OS_NAME_LENGTH) == B_OS_NAME_LENGTH)
		return B_NAME_TOO_LONG;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->WriteLockPartition(partitionID);
	if (!partition)
		return B_ENTRY_NOT_FOUND;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceWriteLocker locker(partition->Device(), true);
	// check name
	char proposedName[B_OS_NAME_LENGTH];
	strcpy(proposedName, name);
	status_t error = validate_set_partition_name(partition, changeCounter,
												 proposedName);
	if (error != B_OK)
		return error;
	if (strcmp(name, proposedName))
		return B_BAD_VALUE;
	// set name
	error = partition->SetName(name);
	if (error != B_OK)
		return error;
	partition->Changed(B_PARTITION_CHANGED_NAME);
	// implicit partitioning system changes
	return partition->Parent()->DiskSystem()->ShadowPartitionChanged(
		partition, B_PARTITION_SET_NAME);
}

// _kern_set_partition_content_name
status_t
_kern_set_partition_content_name(partition_id partitionID, int32 changeCounter,
								 const char *name)
{
	if (!name)
		return B_BAD_VALUE;
	if (strnlen(name, B_OS_NAME_LENGTH) == B_OS_NAME_LENGTH)
		return B_NAME_TOO_LONG;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->WriteLockPartition(partitionID);
	if (!partition)
		return B_ENTRY_NOT_FOUND;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceWriteLocker locker(partition->Device(), true);
	// check name
	char proposedName[B_OS_NAME_LENGTH];
	strcpy(proposedName, name);
	status_t error = validate_set_partition_content_name(partition,
		changeCounter, proposedName);
	if (error != B_OK)
		return error;
	if (strcmp(name, proposedName))
		return B_BAD_VALUE;
	// set name
	error = partition->SetContentName(name);
	if (error != B_OK)
		return error;
	partition->Changed(B_PARTITION_CHANGED_CONTENT_NAME);
	// implicit content disk system changes
	return partition->DiskSystem()->ShadowPartitionChanged(
		partition, B_PARTITION_SET_CONTENT_NAME);
}

// _kern_set_partition_type
status_t
_kern_set_partition_type(partition_id partitionID, int32 changeCounter,
						 const char *type)
{
	if (!type)
		return B_BAD_VALUE;
	if (strnlen(type, B_OS_NAME_LENGTH) == B_OS_NAME_LENGTH)
		return B_NAME_TOO_LONG;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->WriteLockPartition(partitionID);
	if (!partition)
		return B_ENTRY_NOT_FOUND;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceWriteLocker locker(partition->Device(), true);
	// check type
	status_t error = validate_set_partition_type(partition, changeCounter,
												 type);
	if (error != B_OK)
		return error;
	// set type
	error = partition->SetType(type);
	if (error != B_OK)
		return error;
	partition->Changed(B_PARTITION_CHANGED_TYPE);
	// implicit partitioning system changes
	return partition->Parent()->DiskSystem()->ShadowPartitionChanged(
		partition, B_PARTITION_SET_TYPE);
}

// _kern_set_partition_parameters
status_t
_kern_set_partition_parameters(partition_id partitionID, int32 changeCounter,
							   const char *parameters)
{
	if (!parameters)
		return B_BAD_VALUE;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->WriteLockPartition(partitionID);
	if (!partition)
		return B_ENTRY_NOT_FOUND;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceWriteLocker locker(partition->Device(), true);
	// check parameters
	status_t error = validate_set_partition_parameters(partition,
		changeCounter, parameters);
	if (error != B_OK)
		return error;
	// set type
	error = partition->SetParameters(parameters);
	if (error != B_OK)
		return error;
	partition->Changed(B_PARTITION_CHANGED_PARAMETERS);
	// implicit partitioning system changes
	return partition->Parent()->DiskSystem()->ShadowPartitionChanged(
		partition, B_PARTITION_SET_PARAMETERS);
}

// _kern_set_partition_content_parameters
status_t
_kern_set_partition_content_parameters(partition_id partitionID,
									   int32 changeCounter,
									   const char *parameters)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->WriteLockPartition(partitionID);
	if (!partition)
		return B_ENTRY_NOT_FOUND;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceWriteLocker locker(partition->Device(), true);
	// check parameters
	status_t error = validate_set_partition_content_parameters(partition,
		changeCounter, parameters);
	if (error != B_OK)
		return error;
	// set name
	error = partition->SetContentParameters(parameters);
	if (error != B_OK)
		return error;
	partition->Changed(B_PARTITION_CHANGED_CONTENT_PARAMETERS);
	// implicit content disk system changes
	return partition->DiskSystem()->ShadowPartitionChanged(
		partition, B_PARTITION_SET_CONTENT_PARAMETERS);
}

// _kern_initialize_partition
status_t
_kern_initialize_partition(partition_id partitionID, int32 changeCounter,
						   const char *diskSystemName, const char *name,
						   const char *parameters)
{
	if (!diskSystemName || !name)
		return B_BAD_VALUE;
	if (strnlen(name, B_OS_NAME_LENGTH) == B_OS_NAME_LENGTH)
		return B_NAME_TOO_LONG;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->WriteLockPartition(partitionID);
	if (!partition)
		return B_ENTRY_NOT_FOUND;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceWriteLocker locker(partition->Device(), true);
	// get the disk system
	KDiskSystem *diskSystem = manager->LoadDiskSystem(diskSystemName);
	if (!diskSystem)
		return B_ENTRY_NOT_FOUND;
	DiskSystemLoader loader(diskSystem, true);
	// check parameters
	char proposedName[B_OS_NAME_LENGTH];
	strcpy(proposedName, name);
	status_t error = validate_initialize_partition(partition, changeCounter,
		diskSystemName, proposedName, parameters);
	if (error != B_OK)
		return error;
	if (strcmp(name, proposedName))
		return B_BAD_VALUE;
	// unitialize the partition's contents and set the new parameters
	partition->UninitializeContents(true);
	partition->SetDiskSystem(diskSystem);
	error = partition->SetContentName(name);
	if (error != B_OK)
		return error;
	partition->Changed(B_PARTITION_CHANGED_CONTENT_NAME);
	error = partition->SetContentParameters(parameters);
	if (error != B_OK)
		return error;
	partition->Changed(B_PARTITION_CHANGED_CONTENT_PARAMETERS);
	// implicit content disk system changes
	return partition->DiskSystem()->ShadowPartitionChanged(
		partition, B_PARTITION_INITIALIZE);
}

// _kern_uninitialize_partition
status_t
_kern_uninitialize_partition(partition_id partitionID, int32 changeCounter)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->WriteLockPartition(partitionID);
	if (!partition)
		return B_ENTRY_NOT_FOUND;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceWriteLocker locker(partition->Device(), true);
	// unitialize the partition's contents and set the new parameters
	partition->UninitializeContents(true);
	return B_OK;
}

// _kern_create_child_partition
status_t
_kern_create_child_partition(partition_id partitionID, int32 changeCounter,
							 off_t offset, off_t size, const char *type,
							 const char *parameters, partition_id *childID)
{
	if (!type)
		return B_BAD_VALUE;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->WriteLockPartition(partitionID);
	if (!partition)
		return B_ENTRY_NOT_FOUND;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceWriteLocker locker(partition->Device(), true);
	// check the parameters
	off_t proposedOffset = offset;
	off_t proposedSize = size;
	int32 index = 0;
	status_t error = validate_create_child_partition(partition, changeCounter,
		&proposedOffset, &proposedSize, type, parameters, &index);
	if (error != B_OK)
		return error;
	if (proposedOffset != offset || proposedSize != size)
		return B_BAD_VALUE;
	// create the child
	KPartition *child = NULL;
	error = partition->CreateChild(-1, index, &child);
	if (error != B_OK)
		return error;
	partition->Changed(B_PARTITION_CHANGED_CHILDREN);
	if (childID)
		*childID = child->ID();
	// set the parameters
	child->SetOffset(offset);
	child->SetSize(offset);
	error = child->SetType(type);
	if (error != B_OK)
		return error;
	error = partition->SetParameters(parameters);
	if (error != B_OK)
		return error;
	// implicit partitioning system changes
	return partition->DiskSystem()->ShadowPartitionChanged(
		partition, B_PARTITION_CREATE_CHILD);
}

// _kern_delete_partition
status_t
_kern_delete_partition(partition_id partitionID, int32 changeCounter)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->WriteLockPartition(partitionID);
	if (!partition)
		return B_ENTRY_NOT_FOUND;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceWriteLocker locker(partition->Device(), true);
	// check whether delete the child is OK
	status_t error = validate_delete_child_partition(partition, changeCounter);
	if (error != B_OK)
		return error;
	// delete the child
	KPartition *parent = partition->Parent();
	if (!parent->RemoveChild(partition))
		return B_ERROR;
	parent->Changed(B_PARTITION_CHANGED_CHILDREN);
	return B_OK;
}

// _kern_get_next_disk_device_job_info
status_t
_kern_get_next_disk_device_job_info(int32 *cookie,
									user_disk_device_job_info *info)
{
	if (!cookie || !info)
		return B_BAD_VALUE;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	if (ManagerLocker locker = manager) {
		// get the next job and an info
		while (KDiskDeviceJob *job = manager->NextJob(cookie)) {
			// return info only on job scheduled or in progress
			switch (job->Status()) {
				case B_DISK_DEVICE_JOB_SCHEDULED:
				case B_DISK_DEVICE_JOB_IN_PROGRESS:
					return job->GetInfo(info);
				case B_DISK_DEVICE_JOB_UNINITIALIZED:
				case B_DISK_DEVICE_JOB_SUCCEEDED:
				case B_DISK_DEVICE_JOB_FAILED:
				case B_DISK_DEVICE_JOB_CANCELED:
					break;
			}
		}
	}
	return B_ENTRY_NOT_FOUND;
}

// _kern_get_disk_device_job_info
status_t
_kern_get_disk_device_job_info(disk_job_id id, user_disk_device_job_info *info)
{
	if (!info)
		return B_BAD_VALUE;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	if (ManagerLocker locker = manager) {
		// find the job and get the info
		if (KDiskDeviceJob *job = manager->FindJob(id))
			return job->GetInfo(info);
	}
	return B_ENTRY_NOT_FOUND;
}

// _kern_get_disk_device_job_status
status_t
_kern_get_disk_device_job_progress_info(disk_job_id id,
										disk_device_job_progress_info *info)
{
	if (!info)
		return B_BAD_VALUE;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	if (ManagerLocker locker = manager) {
		// find the job and get the info
		if (KDiskDeviceJob *job = manager->FindJob(id))
			return job->GetProgressInfo(info);
	}
	return B_ENTRY_NOT_FOUND;
}

// _kern_pause_disk_device_job
status_t
_kern_pause_disk_device_job(disk_job_id id)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	if (ManagerLocker locker = manager) {
		// get the job and the respective job queue
		if (KDiskDeviceJob *job = manager->FindJob(id)) {
			if (KDiskDeviceJobQueue *jobQueue = job->JobQueue()) {
				// only the active job in progress can be paused
				if (jobQueue->ActiveJob() != job)
					return B_BAD_VALUE;
				return jobQueue->Pause();
			}
		}
	}
	return B_ENTRY_NOT_FOUND;
}

// _kern_cancel_disk_device_job
status_t
_kern_cancel_disk_device_job(disk_job_id id, bool reverse)
{
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	if (ManagerLocker locker = manager) {
		// get the job and the respective job queue
		if (KDiskDeviceJob *job = manager->FindJob(id)) {
			if (KDiskDeviceJobQueue *jobQueue = job->JobQueue()) {
				// only the active job in progress can be canceled
				if (jobQueue->ActiveJob() != job)
					return B_BAD_VALUE;
				return jobQueue->Cancel(reverse);
			}
		}
	}
	return B_ENTRY_NOT_FOUND;
}

