// ddm_userland_interface.cpp

#include <stdlib.h>

#include <AutoDeleter.h>
#include <ddm_userland_interface.h>
#include <KDiskDevice.h>
#include <KDiskDeviceJob.h>
#include <KDiskDeviceJobQueue.h>
#include <KDiskDeviceManager.h>
#include <KDiskDeviceUtils.h>
#include <KDiskSystem.h>
#include <KFileDiskDevice.h>
#include <KShadowPartition.h>
#include <syscall_args.h>

#include "ddm_operation_validation.h"
#include "KDiskDeviceJobGenerator.h"
#include "UserDataWriter.h"

using namespace BPrivate::DiskDevice;

// debugging
#define ERROR(x)

// ddm_strlcpy
/*! \brief Wrapper around user_strlcpy() that returns a status_t
	indicating appropriate success or failure.
	
	\param allowTruncation If \c true, does not return an error if 
	       \a from is longer than \to. If \c false, returns \c B_NAME_TOO_LONG
	       if \a from is longer than \to.
*/
static
status_t
ddm_strlcpy(char *to, const char *from, size_t size, bool allowTruncation = false) {
	int error = user_strlcpy(to, from, size);
	error = (0 <= error && size_t(error) < size) ? B_OK
	        : (error < B_OK ? error : (allowTruncation ? B_OK : B_NAME_TOO_LONG));
	return status_t(error);
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

// _kern_get_next_disk_device_id
partition_id
_kern_get_next_disk_device_id(int32 *_cookie, size_t *neededSize)
{
	if (!_cookie)
		return B_BAD_VALUE;
	int32 cookie;
	user_memcpy(&cookie, _cookie, sizeof(cookie));
	
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
	user_memcpy(_cookie, &cookie, sizeof(cookie));
	return id;
}

// _kern_find_disk_device
partition_id
_kern_find_disk_device(const char *_filename, size_t *neededSize)
{
	if (!_filename)
		return B_BAD_VALUE;

	char filename[B_PATH_NAME_LENGTH];
	status_t error = ddm_strlcpy(filename, _filename, B_PATH_NAME_LENGTH);
	if (error)
		return error;
		
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

	char filename[B_PATH_NAME_LENGTH];
	status_t error = ddm_strlcpy(filename, _filename, B_PATH_NAME_LENGTH);
	if (error)
		return error;

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
/*!	\brief Writes data describing the disk device identified by ID and all
		   its partitions into the supplied buffer.

	The function passes the buffer size required to hold the data back
	through the \a _neededSize parameter, if the device could be found at
	least and no serious error occured. If fails with \c B_BUFFER_OVERFLOW,
	if the supplied buffer is too small or a \c NULL buffer is supplied
	(and \c bufferSize is 0).

	The device is identified by \a id. If \a deviceOnly is \c true, then
	it must be the ID of a disk device, otherwise the disk device is
	chosen, on which the partition \a id refers to resides.

	\param id The ID of an arbitrary partition on the disk device (including
		   the disk device itself), whose data shall be returned
		   (if \a deviceOnly is \c false), or the ID of the disk device
		   itself (if \a deviceOnly is true).
	\param deviceOnly Specifies whether only IDs of disk devices (\c true),
		   or also IDs of partitions (\c false) are accepted for \a id.
	\param shadow If \c true, the data of the shadow disk device is returned,
		   otherwise of the physical device. If there is no shadow device,
		   the parameter is ignored.
	\param buffer The buffer into which the disk device data shall be written.
		   May be \c NULL.
	\param bufferSize The size of \a buffer.
	\param _neededSize Pointer to a variable into which the actually needed
		   buffer size is written. May be \c NULL.
	\return
	- \c B_OK: Everything went fine. The device was found and, if not \c NULL,
	  in \a _neededSize the actually needed buffer size is returned. And
	  \a buffer will contain the disk device data.
	- \c B_BAD_VALUE: \c NULL \a buffer, but not 0 \a bufferSize.
	- \c B_BUFFER_OVERFLOW: The supplied buffer was too small. \a _neededSize,
	  if not \c NULL, will contain the required buffer size.
	- \c B_NO_MEMORY: Insufficient memory to complete the operation.
	- \c B_ENTRY_NOT_FOUND: \a id is no valid disk device ID (if \a deviceOnly
	  is \c true) or not even a valid partition ID (if \a deviceOnly is
	  \c false).
	- \c B_ERROR: An unexpected error occured.
	- another error code...
*/
status_t
_kern_get_disk_device_data(partition_id id, bool deviceOnly, bool shadow,
						   user_disk_device_data *buffer, size_t bufferSize,
						   size_t *_neededSize)
{
	if (!buffer && bufferSize > 0)
		return B_BAD_VALUE;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the device
	if (KDiskDevice *device = manager->RegisterDevice(id, deviceOnly)) {
		PartitionRegistrar _(device, true);
		if (DeviceReadLocker locker = device) {
			// do a dry run first to get the needed size
			UserDataWriter writer;
			device->WriteUserData(writer, shadow);
			size_t neededSize = writer.AllocatedSize();
			if (_neededSize) {
				status_t error = copy_ref_var_to_user(neededSize, _neededSize);
				if (error != B_OK)
					return error;
			}
			// if no buffer has been supplied or the buffer is too small,
			// then we're done
			if (!buffer || bufferSize < neededSize)
				return B_BUFFER_OVERFLOW;
			// otherwise allocate a kernel buffer
			user_disk_device_data *kernelBuffer
				= static_cast<user_disk_device_data*>(malloc(neededSize));
			if (!kernelBuffer)
				return B_NO_MEMORY;
			MemoryDeleter deleter(kernelBuffer);
			// write the device data into the buffer
			writer.SetTo(kernelBuffer, bufferSize);
			device->WriteUserData(writer, shadow);
			// sanity check
			if (writer.AllocatedSize() != neededSize) {
				ERROR(("Size of written disk device user data changed from "
					   "%lu to %lu while device was locked!\n"));
				return B_ERROR;
			}
			// relocate
			status_t error = writer.Relocate(buffer);
			if (error != B_OK)
				return error;
			// copy out
			if (buffer)
				return user_memcpy(buffer, kernelBuffer, neededSize);
		} else
			return B_ERROR;
	}
	return B_ENTRY_NOT_FOUND;
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
					user_memcpy(_actualCount, &actualCount, sizeof(actualCount));
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
	status_t error = ddm_strlcpy(filename, _filename, B_PATH_NAME_LENGTH);
	if (error)
		return error;
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
		status_t error = ddm_strlcpy(filename, _filename, B_PATH_NAME_LENGTH);
		if (error)
			return error;
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
	int32 cookie;
	user_memcpy(&cookie, _cookie, sizeof(cookie));
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
	user_memcpy(_cookie, &cookie, sizeof(cookie));
	return result;
}

// _kern_find_disk_system
status_t
_kern_find_disk_system(const char *_name, user_disk_system_info *_info)
{
	if (!_name || !_info)
		return B_BAD_VALUE;
	char name[B_OS_NAME_LENGTH];
	status_t error = ddm_strlcpy(name, _name, B_OS_NAME_LENGTH);
	if (error)
		return error;
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
		user_memcpy(_whileMounted, &whileMounted, sizeof(whileMounted));
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
		user_memcpy(_whileMounted, &whileMounted, sizeof(whileMounted));
	return result;										   
}

// _kern_supports_resizing_partition
bool
_kern_supports_resizing_partition(partition_id partitionID,
								  int32 changeCounter, bool *_canResizeContents,
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
	if (_canResizeContents) {
		bool whileMounted;
		bool canResizeContents = (childDiskSystem
			&& childDiskSystem->SupportsResizing(partition, &whileMounted));
		user_memcpy(_canResizeContents, &canResizeContents, sizeof(canResizeContents));
		if (_whileMounted)
			user_memcpy(_whileMounted, &whileMounted, sizeof(whileMounted));
	}
// TODO: Currently we report that we cannot resize the contents, if the
// partition's disk system is unknown. I found this more logical. It doesn't
// really matter, though, since the API user can check for this situation.
	return result;
}

// _kern_supports_moving_partition
bool
_kern_supports_moving_partition(partition_id partitionID, int32 changeCounter,
								partition_id *_unmovable,
								partition_id *_needUnmounting,
								size_t bufferSize)
{
	if ((!_unmovable || !_needUnmounting) && bufferSize > 0)
		return false;
	partition_id *unmovable = NULL; 
	partition_id *needUnmounting = NULL;
	if (bufferSize > 0) {
		unmovable = static_cast<partition_id*>(malloc(bufferSize));	
		needUnmounting = static_cast<partition_id*>(malloc(bufferSize));
		if (unmovable && needUnmounting) {
			user_memcpy(unmovable, _unmovable, bufferSize);
			user_memcpy(needUnmounting, _needUnmounting, bufferSize);
		} else {
			free(unmovable);
			free(needUnmounting);
			return false;
		}
	}	
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->ReadLockPartition(partitionID);
	bool result = partition;
	if (result) {
		PartitionRegistrar registrar1(partition, true);
		PartitionRegistrar registrar2(partition->Device(), true);
		DeviceReadLocker locker(partition->Device(), true);
		result = check_shadow_partition(partition, changeCounter)
			     && partition->Parent();
		if (result) {
			result = !partition->Parent()->IsBusy()
			         && !partition->Parent()->IsDescendantBusy();
		}
		if (result) {
			// get the parent disk system
			KDiskSystem *parentDiskSystem = partition->Parent()->DiskSystem();
			result = parentDiskSystem;
			if (result) 
				result = parentDiskSystem->SupportsMovingChild(partition);
			if (result) {
				// check the movability of the descendants' contents
				size_t unmovableSize = bufferSize; 
				size_t needUnmountingSize = bufferSize; 
				result = get_unmovable_descendants(partition, unmovable,
												   unmovableSize, needUnmounting,
												   needUnmountingSize);
			}
		}
	}
	if (result && bufferSize > 0) {
		user_memcpy(_unmovable, unmovable, bufferSize);
		user_memcpy(_needUnmounting, needUnmounting, bufferSize);
	}
	free(unmovable);
	free(needUnmounting);
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
		user_memcpy(_whileMounted, &whileMounted, sizeof(whileMounted));
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
		user_memcpy(_whileMounted, &whileMounted, sizeof(whileMounted));
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
	status_t error = ddm_strlcpy(diskSystemName, _diskSystemName, B_OS_NAME_LENGTH);
	if (error)
		return error;
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
	off_t size;
	user_memcpy(&size, _size, sizeof(size));
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->ReadLockPartition(partitionID);
	if (!partition)
		return B_ENTRY_NOT_FOUND;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceReadLocker locker(partition->Device(), true);
	off_t contentSize = 0;
	bool result = validate_resize_partition(partition, changeCounter, &size,
											&contentSize);
	if (result)
		user_memcpy(_size, &size, sizeof(size));
	return result;								        
}

// _kern_validate_move_partition
status_t
_kern_validate_move_partition(partition_id partitionID, int32 changeCounter,
							  off_t *_newOffset)
{
	if (!_newOffset)
		return B_BAD_VALUE;
	off_t newOffset;
	user_memcpy(&newOffset, _newOffset, sizeof(newOffset));
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->ReadLockPartition(partitionID);
	if (!partition)
		return B_ENTRY_NOT_FOUND;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceReadLocker locker(partition->Device(), true);
	status_t result = validate_move_partition(partition, changeCounter, &newOffset);
	if (result)
		user_memcpy(_newOffset, &newOffset, sizeof(newOffset));
	return result;
}

// _kern_validate_set_partition_name
status_t
_kern_validate_set_partition_name(partition_id partitionID,
								  int32 changeCounter, char *_name)
{
	if (!_name)
		return B_BAD_VALUE;
	char name[B_FILE_NAME_LENGTH];
	status_t error = ddm_strlcpy(name, _name, B_FILE_NAME_LENGTH, true);
	if (error)
		return error;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->ReadLockPartition(partitionID);
	if (!partition)
		return B_ENTRY_NOT_FOUND;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceReadLocker locker(partition->Device(), true);
	error = validate_set_partition_name(partition, changeCounter, name);
	if (!error)
		error = ddm_strlcpy(_name, name, B_FILE_NAME_LENGTH);
	return error;
}

// _kern_validate_set_partition_content_name
status_t
_kern_validate_set_partition_content_name(partition_id partitionID,
										  int32 changeCounter, char *_name)
{
	if (!_name)
		return B_BAD_VALUE;
	char name[B_FILE_NAME_LENGTH];
	status_t error = ddm_strlcpy(name, _name, B_FILE_NAME_LENGTH, true);
	if (error)
		return error;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->ReadLockPartition(partitionID);
	if (!partition)
		return B_ENTRY_NOT_FOUND;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceReadLocker locker(partition->Device(), true);
	error = validate_set_partition_content_name(partition, changeCounter, name);
	if (!error)
		error = ddm_strlcpy(_name, name, B_FILE_NAME_LENGTH);
	return error;
}

// _kern_validate_set_partition_type
status_t
_kern_validate_set_partition_type(partition_id partitionID,
								  int32 changeCounter, const char *_type)
{
	if (!_type)
		return B_BAD_VALUE;
	char type[B_FILE_NAME_LENGTH];
	status_t error = ddm_strlcpy(type, _type, B_FILE_NAME_LENGTH);
	if (error)
		return error;
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
									const char *_diskSystemName, char *_name,
									const char *_parameters,
									size_t parametersSize)
{
	if (!_diskSystemName || !_name || parametersSize > B_DISK_DEVICE_MAX_PARAMETER_SIZE)
		return B_BAD_VALUE;
	char diskSystemName[B_OS_NAME_LENGTH];
	char name[B_FILE_NAME_LENGTH];
	char *parameters = NULL;
	status_t error = ddm_strlcpy(diskSystemName, _diskSystemName, B_OS_NAME_LENGTH);
	if (!error) 
		error = ddm_strlcpy(name, _name, B_FILE_NAME_LENGTH, true);
	if (error)
		return error;
	if (_parameters) {
		parameters = static_cast<char*>(malloc(parametersSize));
		if (parameters)
			user_memcpy(parameters, _parameters, parametersSize);
		else
			return B_NO_MEMORY;
	}
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->ReadLockPartition(partitionID);
	error = partition ? B_OK : B_ENTRY_NOT_FOUND;
	if (!error) {
		PartitionRegistrar registrar1(partition, true);
		PartitionRegistrar registrar2(partition->Device(), true);
		DeviceReadLocker locker(partition->Device(), true);
		error = validate_initialize_partition(partition, changeCounter,
											  diskSystemName, name, parameters);
	}
	if (!error)
		error = ddm_strlcpy(_name, name, B_FILE_NAME_LENGTH);
	free(parameters);
	return error;											  
}

// _kern_validate_create_child_partition
status_t
_kern_validate_create_child_partition(partition_id partitionID,
									  int32 changeCounter, off_t *_offset,
									  off_t *_size, const char *_type,
									  const char *_parameters,
									  size_t parametersSize)
{
	if (!_offset || !_size || !_type
	    || parametersSize > B_DISK_DEVICE_MAX_PARAMETER_SIZE)
	{
		return B_BAD_VALUE;
	}
	off_t offset;
	off_t size;
	char type[B_FILE_NAME_LENGTH];
	char *parameters = NULL;
	user_memcpy(&offset, _offset, sizeof(offset));
	user_memcpy(&size, _size, sizeof(size));
	status_t error = ddm_strlcpy(type, _type, B_FILE_NAME_LENGTH);
	if (error)
		return error;
	if (_parameters) {
		parameters = static_cast<char*>(malloc(parametersSize));
		if (parameters)
			user_memcpy(parameters, _parameters, parametersSize);
		else
			return B_NO_MEMORY;
	}	
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->ReadLockPartition(partitionID);
	error = partition ? B_OK : B_ENTRY_NOT_FOUND;
	if (!error) {
		PartitionRegistrar registrar1(partition, true);
		PartitionRegistrar registrar2(partition->Device(), true);
		DeviceReadLocker locker(partition->Device(), true);
		error = validate_create_child_partition(partition, changeCounter, &offset,
											    &size, type, parameters);
	}
	if (!error) {
		user_memcpy(_offset, &offset, sizeof(offset));
		user_memcpy(_size, &size, sizeof(size));
	}
	free(parameters);
	return error;
}

// _kern_get_next_supported_partition_type
status_t
_kern_get_next_supported_partition_type(partition_id partitionID,
										int32 changeCounter, int32 *_cookie,
										char *_type)
{
	if (!_cookie || !_type)
		return B_BAD_VALUE;
	int32 cookie;
	user_memcpy(&cookie, _cookie, sizeof(cookie));
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->ReadLockPartition(partitionID);
	status_t error = partition ? B_OK : B_ENTRY_NOT_FOUND;
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
				char type[B_FILE_NAME_LENGTH];
				error = diskSystem->GetNextSupportedType(partition, &cookie, type);
				if (!error) {
					error = ddm_strlcpy(_type, type, B_FILE_NAME_LENGTH);
				}
			}
		}
	}
	if (!error)
		user_memcpy(_cookie, &cookie, sizeof(cookie));
	return error;
}

// _kern_get_partition_type_for_content_type
status_t
_kern_get_partition_type_for_content_type(disk_system_id diskSystemID,
										  const char *_contentType, char *_type)
{
	if (!_contentType || !_type)
		return B_BAD_VALUE;
	char contentType[B_FILE_NAME_LENGTH];
	status_t error = ddm_strlcpy(contentType, _contentType, B_FILE_NAME_LENGTH);
	if (error)
		return error;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the disk system
	KDiskSystem *diskSystem = manager->LoadDiskSystem(diskSystemID);
	if (!diskSystem)
		return false;
	DiskSystemLoader loader(diskSystem, true);
	// get the info
	char type[B_FILE_NAME_LENGTH];
	if (diskSystem->GetTypeForContentType(contentType, type)) {
		return ddm_strlcpy(_type, type, B_FILE_NAME_LENGTH);
	}
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
	off_t contentSize = 0;
	status_t error = validate_resize_partition(partition, changeCounter,
											   &proposedSize, &contentSize);
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
						 const char *_name)
{
	if (!_name)
		return B_BAD_VALUE;
	char name[B_FILE_NAME_LENGTH];
	status_t error = ddm_strlcpy(name, _name, B_FILE_NAME_LENGTH);
	if (error)
		return error;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->WriteLockPartition(partitionID);
	if (!partition)
		return B_ENTRY_NOT_FOUND;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceWriteLocker locker(partition->Device(), true);
	// check name
	char proposedName[B_FILE_NAME_LENGTH];
	strcpy(proposedName, name);
	error = validate_set_partition_name(partition, changeCounter,
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
								 const char *_name)
{
	if (!_name)
		return B_BAD_VALUE;
	char name[B_FILE_NAME_LENGTH];
	status_t error = ddm_strlcpy(name, _name, B_FILE_NAME_LENGTH);
	if (error)
		return error;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->WriteLockPartition(partitionID);
	if (!partition)
		return B_ENTRY_NOT_FOUND;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceWriteLocker locker(partition->Device(), true);
	// check name
	char proposedName[B_FILE_NAME_LENGTH];
	strcpy(proposedName, name);
	error = validate_set_partition_content_name(partition,
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
						 const char *_type)
{
	if (!_type)
		return B_BAD_VALUE;
	char type[B_FILE_NAME_LENGTH];
	status_t error = ddm_strlcpy(type, _type, B_FILE_NAME_LENGTH);
	if (error)
		return error;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->WriteLockPartition(partitionID);
	if (!partition)
		return B_ENTRY_NOT_FOUND;
	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceWriteLocker locker(partition->Device(), true);
	// check type
	error = validate_set_partition_type(partition, changeCounter,
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
							   const char *_parameters, size_t parametersSize)
{
	if (!_parameters || parametersSize > B_DISK_DEVICE_MAX_PARAMETER_SIZE)
		return B_BAD_VALUE;
	char *parameters = NULL;
	if (_parameters) {
		parameters = static_cast<char*>(malloc(parametersSize));
		if (parameters)
			user_memcpy(parameters, _parameters, parametersSize);
		else
			return B_NO_MEMORY;
	}
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->WriteLockPartition(partitionID);
	status_t error = partition ? B_OK : B_ENTRY_NOT_FOUND;
	if (!error) {
		PartitionRegistrar registrar1(partition, true);
		PartitionRegistrar registrar2(partition->Device(), true);
		DeviceWriteLocker locker(partition->Device(), true);
		// check parameters
		error = validate_set_partition_parameters(partition,
			changeCounter, parameters);
		if (!error) {
			// set type
			error = partition->SetParameters(parameters);
			if (!error) {
				partition->Changed(B_PARTITION_CHANGED_PARAMETERS);
				// implicit partitioning system changes
				error = partition->Parent()->DiskSystem()->ShadowPartitionChanged(
					partition, B_PARTITION_SET_PARAMETERS);
			}
		}
	}
	free(parameters);
	return error;
}

// _kern_set_partition_content_parameters
status_t
_kern_set_partition_content_parameters(partition_id partitionID,
									   int32 changeCounter,
									   const char *_parameters,
									   size_t parametersSize)
{
	if (!_parameters || parametersSize > B_DISK_DEVICE_MAX_PARAMETER_SIZE)
		return B_BAD_VALUE;
	char *parameters = NULL;
	if (_parameters) {
		parameters = static_cast<char*>(malloc(parametersSize));
		if (parameters)
			user_memcpy(parameters, _parameters, parametersSize);
		else
			return B_NO_MEMORY;
	}
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->WriteLockPartition(partitionID);
	status_t error = partition ? B_OK : B_ENTRY_NOT_FOUND;
	if (!error) {
		PartitionRegistrar registrar1(partition, true);
		PartitionRegistrar registrar2(partition->Device(), true);
		DeviceWriteLocker locker(partition->Device(), true);
		// check parameters
		error = validate_set_partition_content_parameters(partition,
			changeCounter, parameters);
		if (!error) {
			// set name
			error = partition->SetContentParameters(parameters);
			if (!error) {
				partition->Changed(B_PARTITION_CHANGED_CONTENT_PARAMETERS);
				// implicit content disk system changes
				error = partition->DiskSystem()->ShadowPartitionChanged(
					partition, B_PARTITION_SET_CONTENT_PARAMETERS);
			}
		}
	}
	free(partition);
	return error;
}

// _kern_initialize_partition
status_t
_kern_initialize_partition(partition_id partitionID, int32 changeCounter,
						   const char *_diskSystemName, const char *_name,
						   const char *_parameters, size_t parametersSize)
{
	if (!_diskSystemName || !_name || parametersSize > B_DISK_DEVICE_MAX_PARAMETER_SIZE)
		return B_BAD_VALUE;
	char diskSystemName[B_OS_NAME_LENGTH];
	char name[B_FILE_NAME_LENGTH];
	char *parameters = NULL;
	status_t error = ddm_strlcpy(diskSystemName, _diskSystemName, B_OS_NAME_LENGTH);
	if (!error)
		error = ddm_strlcpy(name, _name, B_FILE_NAME_LENGTH);
	if (error)
		return error;
	if (_parameters) {
		parameters = static_cast<char*>(malloc(parametersSize));
		if (parameters)
			user_memcpy(parameters, _parameters, parametersSize);
		else
			return B_NO_MEMORY;
	}
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->WriteLockPartition(partitionID);
	error = partition ? B_OK : B_ENTRY_NOT_FOUND;
	if (!error) {
		PartitionRegistrar registrar1(partition, true);
		PartitionRegistrar registrar2(partition->Device(), true);
		DeviceWriteLocker locker(partition->Device(), true);
		// get the disk system
		KDiskSystem *diskSystem = manager->LoadDiskSystem(diskSystemName);
		error = diskSystem ? B_OK : B_ENTRY_NOT_FOUND;
		if (!error) {
			DiskSystemLoader loader(diskSystem, true);
			// check parameters
			char proposedName[B_FILE_NAME_LENGTH];
			strcpy(proposedName, name);
			error = validate_initialize_partition(partition, changeCounter,
				diskSystemName, proposedName, parameters);
			if (!error) {
				error = !strcmp(name, proposedName) ? B_OK : B_BAD_VALUE;
			}
			if (!error) {
				// unitialize the partition's contents and set the new parameters
				error = partition->UninitializeContents(true);
			}
			if (!error) {
				partition->SetDiskSystem(diskSystem);
				error = partition->SetContentName(name);
			}
			if (!error) {
				partition->Changed(B_PARTITION_CHANGED_CONTENT_NAME);
				error = partition->SetContentParameters(parameters);
			}
			if (!error) {
				partition->Changed(B_PARTITION_CHANGED_CONTENT_PARAMETERS);
				// implicit content disk system changes
				error = partition->DiskSystem()->ShadowPartitionChanged(
					partition, B_PARTITION_INITIALIZE);
			}
		}
	}
	free(parameters);
	return error;
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
	return partition->UninitializeContents(true);
}

// _kern_create_child_partition
status_t
_kern_create_child_partition(partition_id partitionID, int32 changeCounter,
							 off_t offset, off_t size, const char *_type,
							 const char *_parameters, size_t parametersSize,
							 partition_id *_childID)
{
	if (!_type || parametersSize > B_DISK_DEVICE_MAX_PARAMETER_SIZE)
		return B_BAD_VALUE;
	char type[B_FILE_NAME_LENGTH];
	char *parameters = NULL;
	status_t error = ddm_strlcpy(type, _type, B_FILE_NAME_LENGTH);
	if (error)
		return error;
	if (_parameters) {
		parameters = static_cast<char*>(malloc(parametersSize));
		if (parameters)
			user_memcpy(parameters, _parameters, parametersSize);
		else
			return B_NO_MEMORY;
	}
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// get the partition
	KPartition *partition = manager->WriteLockPartition(partitionID);
	error = partition ? B_OK : B_ENTRY_NOT_FOUND;
	if (!error) {
		PartitionRegistrar registrar1(partition, true);
		PartitionRegistrar registrar2(partition->Device(), true);
		DeviceWriteLocker locker(partition->Device(), true);
		// check the parameters
		off_t proposedOffset = offset;
		off_t proposedSize = size;
		int32 index = 0;
		error = validate_create_child_partition(partition, changeCounter,
			&proposedOffset, &proposedSize, type, parameters, &index);
		if (!error) {
			error =	(proposedOffset == offset && proposedSize == size)
				    ? B_OK : B_BAD_VALUE;
			if (!error) {
				// create the child
				KPartition *child = NULL;
				error = partition->CreateChild(-1, index, &child);
				if (!error) {
					partition->Changed(B_PARTITION_CHANGED_CHILDREN);
					if (_childID) {
						partition_id childID = child->ID();
						user_memcpy(_childID, &childID, sizeof(childID));
					}				
					// set the parameters
					child->SetOffset(offset);
					child->SetSize(offset);
					error = child->SetType(type);
				}
				if (!error) {
					error = partition->SetParameters(parameters);
				}
				if (!error) {
					// implicit partitioning system changes
					error = partition->DiskSystem()->ShadowPartitionChanged(
							partition, B_PARTITION_CREATE_CHILD);
				}
			}
		}
	}
	free(parameters);
	return error;
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
_kern_get_next_disk_device_job_info(int32 *_cookie,
									user_disk_device_job_info *_info)
{
	if (!_cookie || !_info)
		return B_BAD_VALUE;
	int32 cookie;
	user_disk_device_job_info info;
	user_memcpy(&cookie, _cookie, sizeof(cookie));
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	status_t error = B_ENTRY_NOT_FOUND;
	if (ManagerLocker locker = manager) {
		// get the next job and an info
		while (KDiskDeviceJob *job = manager->NextJob(&cookie)) {
			// return info only on job scheduled or in progress
			switch (job->Status()) {
				case B_DISK_DEVICE_JOB_SCHEDULED:
				case B_DISK_DEVICE_JOB_IN_PROGRESS:
					error = job->GetInfo(&info);
					break;
				case B_DISK_DEVICE_JOB_UNINITIALIZED:
				case B_DISK_DEVICE_JOB_SUCCEEDED:
				case B_DISK_DEVICE_JOB_FAILED:
				case B_DISK_DEVICE_JOB_CANCELED:
					break;
			}
		}
	}
	if (!error) {
		user_memcpy(_cookie, &cookie, sizeof(cookie));
		user_memcpy(_info, &info, sizeof(info));
	}
	return error;
}

// _kern_get_disk_device_job_info
status_t
_kern_get_disk_device_job_info(disk_job_id id, user_disk_device_job_info *_info)
{
	if (!_info)
		return B_BAD_VALUE;
	user_disk_device_job_info info;		
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	status_t error = B_ENTRY_NOT_FOUND;
	if (ManagerLocker locker = manager) {
		// find the job and get the info
		if (KDiskDeviceJob *job = manager->FindJob(id))
			error = job->GetInfo(&info);
	}
	if (!error)
		user_memcpy(_info, &info, sizeof(info));
	return error;
}

// _kern_get_disk_device_job_status
status_t
_kern_get_disk_device_job_progress_info(disk_job_id id,
										disk_device_job_progress_info *_info)
{
	if (!_info)
		return B_BAD_VALUE;
	disk_device_job_progress_info info;		
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	status_t error = B_ENTRY_NOT_FOUND;
	if (ManagerLocker locker = manager) {
		// find the job and get the info
		if (KDiskDeviceJob *job = manager->FindJob(id))
			error = job->GetProgressInfo(&info);
	}
	if (!error)
		user_memcpy(_info, &info, sizeof(info));
	return error;
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

