/** \file ddm_userland_interface.cpp
 *
 * 	\brief Interface for userspace calls.
 */

#include <stdlib.h>

#include <AutoDeleter.h>
#include <ddm_userland_interface.h>
#include <KDiskDevice.h>
#include <KDiskDeviceManager.h>
#include <KDiskDeviceUtils.h>
#include <KDiskSystem.h>
#include <KFileDiskDevice.h>
#include <KShadowPartition.h>
#include <syscall_args.h>

#include "UserDataWriter.h"

using namespace BPrivate::DiskDevice;

// debugging
#define ERROR(x)


// TODO: Add user address checks and check return values of user_memcpy()!


// ddm_strlcpy
/*! \brief Wrapper around user_strlcpy() that returns a status_t
	indicating appropriate success or failure.
	
	\param allowTruncation If \c true, does not return an error if 
	       \a from is longer than \to. If \c false, returns \c B_NAME_TOO_LONG
	       if \a from is longer than \to.
*/
static status_t
ddm_strlcpy(char *to, const char *from, size_t size,
	bool allowTruncation = false)
{
	ssize_t fromLen = user_strlcpy(to, from, size);
	if (fromLen < 0)
		return fromLen;
	if ((size_t)fromLen >= size && !allowTruncation)
		return B_NAME_TOO_LONG;
	return B_OK;
}


#if 0
// move_descendants
static void
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
static status_t
move_descendants_contents(KPartition *partition)
{
	if (!partition)
		return B_BAD_VALUE;
	// implicit content disk system changes
	KDiskSystem *diskSystem = partition->DiskSystem();
	if (diskSystem || partition->AlgorithmData()) {
		status_t error = diskSystem->ShadowPartitionChanged(partition,
			NULL, B_PARTITION_MOVE);
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
#endif // 0


// _user_get_next_disk_device_id
partition_id
_user_get_next_disk_device_id(int32 *_cookie, size_t *neededSize)
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


// _user_find_disk_device
partition_id
_user_find_disk_device(const char *_filename, size_t *neededSize)
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


// _user_find_partition
partition_id
_user_find_partition(const char *_filename, size_t *neededSize)
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


// _user_get_disk_device_data
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
_user_get_disk_device_data(partition_id id, bool deviceOnly, bool shadow,
	user_disk_device_data *buffer, size_t bufferSize, size_t *_neededSize)
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


// _user_register_file_device
partition_id
_user_register_file_device(const char *_filename)
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


// _user_unregister_file_device
status_t
_user_unregister_file_device(partition_id deviceID, const char *_filename)
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


// _user_get_disk_system_info
status_t
_user_get_disk_system_info(disk_system_id id, user_disk_system_info *_info)
{
	if (!_info)
		return B_BAD_VALUE;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	if (ManagerLocker locker = manager) {
		if (KDiskSystem *diskSystem = manager->FindDiskSystem(id)) {
			user_disk_system_info info;
			diskSystem->GetInfo(&info);
			user_memcpy(_info, &info, sizeof(info));
			return B_OK;
		}
	}
	return B_ENTRY_NOT_FOUND;
}


// _user_get_next_disk_system_info
status_t
_user_get_next_disk_system_info(int32 *_cookie, user_disk_system_info *_info)
{
	if (!_cookie || !_info)
		return B_BAD_VALUE;
	int32 cookie;
	user_memcpy(&cookie, _cookie, sizeof(cookie));
	status_t result = B_ENTRY_NOT_FOUND;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	if (ManagerLocker locker = manager) {
		if (KDiskSystem *diskSystem = manager->NextDiskSystem(&cookie)) {
			user_disk_system_info info;
			diskSystem->GetInfo(&info);
			user_memcpy(_info, &info, sizeof(info));
			result = B_OK;
		}
	}
	user_memcpy(_cookie, &cookie, sizeof(cookie));
	return result;
}


// _user_find_disk_system
status_t
_user_find_disk_system(const char *_name, user_disk_system_info *_info)
{
	if (!_name || !_info)
		return B_BAD_VALUE;
	char name[B_DISK_SYSTEM_NAME_LENGTH];
	status_t error = ddm_strlcpy(name, _name, B_DISK_SYSTEM_NAME_LENGTH);
	if (error)
		return error;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	if (ManagerLocker locker = manager) {
		if (KDiskSystem *diskSystem = manager->FindDiskSystem(name)) {
			user_disk_system_info info;
			diskSystem->GetInfo(&info);
			user_memcpy(_info, &info, sizeof(info));
			return B_OK;
		}
	}
	return B_ENTRY_NOT_FOUND;
}


// _user_defragment_partition
status_t
_user_defragment_partition(partition_id partitionID, int32* changeCounter)
{
#if 0
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
#endif
return B_BAD_VALUE;
}


// _user_repair_partition
status_t
_user_repair_partition(partition_id partitionID, int32* changeCounter,
	bool checkOnly)
{
#if 0
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
#endif
return B_BAD_VALUE;
}


// _user_resize_partition
status_t
_user_resize_partition(partition_id partitionID, int32* changeCounter,
	partition_id childID, int32* childChangeCounter, off_t size,
	off_t contentSize)
{
#if 0
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
		partition->Parent(), partition, B_PARTITION_RESIZE_CHILD);
	if (error != B_OK)
		return error;
	// implicit content disk system changes
	if (partition->DiskSystem()) {
		error = partition->DiskSystem()->ShadowPartitionChanged(
			partition, NULL, B_PARTITION_RESIZE);
	}
	return error;
#endif
return B_BAD_VALUE;
}


// _user_move_partition
status_t
_user_move_partition(partition_id partitionID, int32* changeCounter,
	partition_id childID, int32* childChangeCounter, off_t newOffset,
	partition_id* descendantIDs, int32* descendantChangeCounters,
	int32 descendantCount)
{
#if 0
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
		partition->Parent(), partition, B_PARTITION_MOVE_CHILD);
	if (error != B_OK)
		return error;
	// implicit descendants' content disk system changes
	return move_descendants_contents(partition);
#endif
return B_BAD_VALUE;
}


// _user_set_partition_name
status_t
_user_set_partition_name(partition_id partitionID, int32* changeCounter,
	partition_id childID, int32* childChangeCounter, const char* name)
{
#if 0
	if (!_name)
		return B_BAD_VALUE;
	char name[B_DISK_DEVICE_NAME_LENGTH];
	status_t error = ddm_strlcpy(name, _name, B_DISK_DEVICE_NAME_LENGTH);
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
	char proposedName[B_DISK_DEVICE_NAME_LENGTH];
	strcpy(proposedName, name);
	error = validate_set_partition_name(partition, changeCounter, proposedName);
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
		partition->Parent(), partition, B_PARTITION_SET_NAME);
#endif
return B_BAD_VALUE;
}


// _user_set_partition_content_name
status_t
_user_set_partition_content_name(partition_id partitionID, int32* changeCounter,
	const char* name)
{
#if 0
	if (!_name)
		return B_BAD_VALUE;
	char name[B_DISK_DEVICE_NAME_LENGTH];
	status_t error = ddm_strlcpy(name, _name, B_DISK_DEVICE_NAME_LENGTH);
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
	char proposedName[B_DISK_DEVICE_NAME_LENGTH];
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
		partition, NULL, B_PARTITION_SET_CONTENT_NAME);
#endif
return B_BAD_VALUE;
}


// _user_set_partition_type
status_t
_user_set_partition_type(partition_id partitionID, int32* changeCounter,
	partition_id childID, int32* childChangeCounter, const char* type)
{
#if 0
	if (!_type)
		return B_BAD_VALUE;
	char type[B_DISK_DEVICE_TYPE_LENGTH];
	status_t error = ddm_strlcpy(type, _type, B_DISK_DEVICE_TYPE_LENGTH);
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
	error = validate_set_partition_type(partition, changeCounter, type);
	if (error != B_OK)
		return error;
	// set type
	error = partition->SetType(type);
	if (error != B_OK)
		return error;
	partition->Changed(B_PARTITION_CHANGED_TYPE);
	// implicit partitioning system changes
	return partition->Parent()->DiskSystem()->ShadowPartitionChanged(
		partition->Parent(), partition, B_PARTITION_SET_TYPE);
#endif
return B_BAD_VALUE;
}


// _user_set_partition_parameters
status_t
_user_set_partition_parameters(partition_id partitionID, int32* changeCounter,
	partition_id childID, int32* childChangeCounter, const char* parameters,
	size_t parametersSize)
{
#if 0
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
	status_t error = partition ? (status_t)B_OK : (status_t)B_ENTRY_NOT_FOUND;
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
				error = partition->Parent()->DiskSystem()
					->ShadowPartitionChanged(partition->Parent(), partition,
						B_PARTITION_SET_PARAMETERS);
			}
		}
	}
	free(parameters);
	return error;
#endif
return B_BAD_VALUE;
}


// _user_set_partition_content_parameters
status_t
_user_set_partition_content_parameters(partition_id partitionID,
	int32* changeCounter, const char* parameters, size_t parametersSize)
{
#if 0
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
	status_t error = partition ? (status_t)B_OK : (status_t)B_ENTRY_NOT_FOUND;
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
					partition, NULL, B_PARTITION_SET_CONTENT_PARAMETERS);
			}
		}
	}
	free(partition);
	return error;
#endif
return B_BAD_VALUE;
}


// _user_initialize_partition
status_t
_user_initialize_partition(partition_id partitionID, int32* changeCounter,
	const char* diskSystemName, const char* name, const char* parameters,
	size_t parametersSize)
{
#if 0
	if (!_diskSystemName || parametersSize > B_DISK_DEVICE_MAX_PARAMETER_SIZE)
		return B_BAD_VALUE;

	// copy disk system name
	char diskSystemName[B_DISK_SYSTEM_NAME_LENGTH];
	status_t error = ddm_strlcpy(diskSystemName, _diskSystemName,
		B_DISK_SYSTEM_NAME_LENGTH);

	// copy name
	char name[B_DISK_DEVICE_NAME_LENGTH];
	if (!error && _name)
		error = ddm_strlcpy(name, _name, B_DISK_DEVICE_NAME_LENGTH);

	if (error)
		return error;

	// copy parameters
	MemoryDeleter parameterDeleter;
	char *parameters = NULL;
	if (_parameters) {
		parameters = static_cast<char*>(malloc(parametersSize));
		if (!parameters)
			return B_NO_MEMORY;
		parameterDeleter.SetTo(parameters);

		if (user_memcpy(parameters, _parameters, parametersSize) != B_OK)
			return B_BAD_ADDRESS;
	}

	// get the partition
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
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
	char proposedName[B_DISK_DEVICE_NAME_LENGTH];
	if (_name)
		strcpy(proposedName, name);

	error = validate_initialize_partition(partition, changeCounter,
		diskSystemName, _name ? proposedName : NULL, parameters);
	if (error != B_OK)
		return error;
	if (_name && strcmp(name, proposedName) != 0)
		return B_BAD_VALUE;

	// unitialize the partition's contents and set the new
	// parameters
	if ((error = partition->UninitializeContents(true)) != B_OK)
		return error;

	partition->SetDiskSystem(diskSystem);

	if ((error = partition->SetContentName(_name ? name : NULL)) != B_OK)
		return error;
	partition->Changed(B_PARTITION_CHANGED_CONTENT_NAME);

	if ((error = partition->SetContentParameters(parameters)) != B_OK)
		return error;
	partition->Changed(B_PARTITION_CHANGED_CONTENT_PARAMETERS);

	partition->Changed(B_PARTITION_CHANGED_INITIALIZATION);

	// implicit content disk system changes
	return partition->DiskSystem()->ShadowPartitionChanged(
		partition, NULL, B_PARTITION_INITIALIZE);
#endif
return B_BAD_VALUE;
}


// _user_uninitialize_partition
status_t
_user_uninitialize_partition(partition_id partitionID, int32* changeCounter)
{
#if 0
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
#endif
return B_BAD_VALUE;
}


// _user_create_child_partition
status_t
_user_create_child_partition(partition_id partitionID, int32* changeCounter,
	off_t offset, off_t size, const char* type, const char* name,
	const char* parameters, size_t parametersSize, partition_id* childID,
	int32* childChangeCounter)

{
#if 0
	if (!_type || parametersSize > B_DISK_DEVICE_MAX_PARAMETER_SIZE)
		return B_BAD_VALUE;
	char type[B_DISK_DEVICE_TYPE_LENGTH];
	char *parameters = NULL;
	status_t error = ddm_strlcpy(type, _type, B_DISK_DEVICE_TYPE_LENGTH);
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
	error = partition ? (status_t)B_OK : (status_t)B_ENTRY_NOT_FOUND;
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
					child->SetSize(size);
					error = child->SetType(type);
				}
				if (!error) {
					error = child->SetParameters(parameters);
				}
				if (!error) {
					// implicit partitioning system changes
					error = partition->DiskSystem()->ShadowPartitionChanged(
							partition, child, B_PARTITION_CREATE_CHILD);
				}
			}
		}
	}
	free(parameters);
	return error;
#endif
return B_BAD_VALUE;
}


// _user_delete_child_partition
status_t
_user_delete_child_partition(partition_id partitionID, int32* changeCounter,
	partition_id childID, int32 childChangeCounter)
{
#if 0
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
#endif
return B_BAD_VALUE;
}
