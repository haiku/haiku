/*
 * Copyright 2003-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 *		Axel Dörfler, axeld@pinc-software.de
 */

/*! \file ddm_userland_interface.cpp

 	\brief Interface for userspace calls.
*/

#include <ddm_userland_interface.h>

#include <stdlib.h>

#include <AutoDeleter.h>
#include <fs/KPath.h>
#include <KDiskDevice.h>
#include <KDiskDeviceManager.h>
#include <KDiskDeviceUtils.h>
#include <KDiskSystem.h>
#include <KFileDiskDevice.h>
#include <syscall_args.h>

#include "UserDataWriter.h"

using namespace BPrivate::DiskDevice;

// debugging
#define ERROR(x)


// TODO: Replace all instances, when it has been decided how to handle
// notifications during jobs.
#define DUMMY_JOB_ID	0


// TODO: Add user address checks and check return values of user_memcpy()!


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


template<typename Type>
static inline status_t
copy_from_user_value(Type& value, const Type* userValue)
{
	if (!userValue)
		return B_BAD_VALUE;

	if (!IS_USER_ADDRESS(userValue))
		return B_BAD_ADDRESS;

	return user_memcpy(&value, userValue, sizeof(Type));
}


template<typename Type>
static inline status_t
copy_to_user_value(Type* userValue, const Type& value)
{
	if (!userValue)
		return B_BAD_VALUE;

	if (!IS_USER_ADDRESS(userValue))
		return B_BAD_ADDRESS;

	return user_memcpy(userValue, &value, sizeof(Type));
}


template<bool kAllowsNull>
struct UserStringParameter {
	char*	value;

	inline UserStringParameter()
		: value(NULL)
	{
	}

	inline ~UserStringParameter()
	{
		free(value);
	}

	inline status_t Init(const char* userValue, size_t maxSize)
	{
		if (userValue == NULL) {
			if (!kAllowsNull)
				return B_BAD_VALUE;

			return B_OK;
		}

		if (!IS_USER_ADDRESS(userValue))
			return B_BAD_ADDRESS;

		value = (char*)malloc(maxSize);
		if (value == NULL)
			return B_NO_MEMORY;

		ssize_t bytesCopied = user_strlcpy(value, userValue, maxSize);
		if (bytesCopied < 0)
			return bytesCopied;

		if ((size_t)bytesCopied >= maxSize)
			return B_BUFFER_OVERFLOW;

		return B_OK;
	}

	inline operator const char*()
	{
		return value;
	}

	inline operator char*()
	{
		return value;
	}
};


#if 0
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
				device->WriteUserData(writer);
				status_t status = copy_to_user_value(neededSize,
					writer.AllocatedSize());
				if (status != B_OK)
					return status;
			} else {
				id = B_ERROR;
			}
		}
	}
	user_memcpy(_cookie, &cookie, sizeof(cookie));
	return id;
}


partition_id
_user_find_disk_device(const char *_filename, size_t *neededSize)
{
	UserStringParameter<false> filename;
	status_t error = filename.Init(_filename, B_PATH_NAME_LENGTH);
	if (error != B_OK)
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
				device->WriteUserData(writer);
				error = copy_to_user_value(neededSize, writer.AllocatedSize());
				if (error != B_OK)
					return error;
			} else
				return B_ERROR;
		}
	}
	return id;
}


partition_id
_user_find_partition(const char *_filename, size_t *neededSize)
{
	UserStringParameter<false> filename;
	status_t error = filename.Init(_filename, B_PATH_NAME_LENGTH);
	if (error != B_OK)
		return error;

	partition_id id = B_ENTRY_NOT_FOUND;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// find the partition
	if (KPartition *partition = manager->RegisterPartition(filename)) {
		PartitionRegistrar _(partition, true);
		id = partition->ID();
		if (neededSize) {
			// get and lock the partition's device
			KDiskDevice *device = manager->RegisterDevice(partition->ID(),
				false);
			if (!device)
				return B_ENTRY_NOT_FOUND;
			PartitionRegistrar _2(device, true);
			if (DeviceReadLocker locker = device) {
				// get the needed size
				UserDataWriter writer;
				device->WriteUserData(writer);
				error = copy_to_user_value(neededSize, writer.AllocatedSize());
				if (error != B_OK)
					return error;
			} else
				return B_ERROR;
		}
	}
	return id;
}


partition_id
_user_find_file_disk_device(const char *_filename, size_t *neededSize)
{
	UserStringParameter<false> filename;
	status_t error = filename.Init(_filename, B_PATH_NAME_LENGTH);
	if (error != B_OK)
		return error;

	KPath path(filename, true);

	partition_id id = B_ENTRY_NOT_FOUND;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	// find the device
	if (KFileDiskDevice* device = manager->RegisterFileDevice(path.Path())) {
		PartitionRegistrar _(device, true);
		id = device->ID();
		if (neededSize) {
			if (DeviceReadLocker locker = device) {
				// get the needed size
				UserDataWriter writer;
				device->WriteUserData(writer);
				error = copy_to_user_value(neededSize, writer.AllocatedSize());
				if (error != B_OK)
					return error;
			} else
				return B_ERROR;
		}
	}
	return id;
}


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
_user_get_disk_device_data(partition_id id, bool deviceOnly,
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
			device->WriteUserData(writer);
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
			device->WriteUserData(writer);
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


partition_id
_user_register_file_device(const char *_filename)
{
	UserStringParameter<false> filename;
	status_t error = filename.Init(_filename, B_PATH_NAME_LENGTH);
	if (error != B_OK)
		return error;

	KPath path(filename, true);

	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	if (ManagerLocker locker = manager) {
		if (KFileDiskDevice *device = manager->FindFileDevice(path.Path()))
			return device->ID();
		return manager->CreateFileDevice(path.Path());
	}
	return B_ERROR;
}


status_t
_user_unregister_file_device(partition_id deviceID, const char *_filename)
{
	if (deviceID < 0 && !_filename)
		return B_BAD_VALUE;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	if (deviceID >= 0) {
		return manager->DeleteFileDevice(deviceID);
	} else {
		UserStringParameter<false> filename;
		status_t error = filename.Init(_filename, B_PATH_NAME_LENGTH);
		if (error != B_OK)
			return error;

		return manager->DeleteFileDevice(filename);
	}
}


status_t
_user_get_file_disk_device_path(partition_id id, char* buffer,
	size_t bufferSize)
{
	if (id < 0 || buffer == NULL || bufferSize == 0)
		return B_BAD_VALUE;

	KDiskDeviceManager *manager = KDiskDeviceManager::Default();

	if (KDiskDevice *device = manager->RegisterDevice(id, true)) {
		PartitionRegistrar _(device, true);
		if (DeviceReadLocker locker = device) {
			KFileDiskDevice* fileDevice
				= dynamic_cast<KFileDiskDevice*>(device);
			if (fileDevice == NULL)
				return B_BAD_VALUE;

			ssize_t copied = user_strlcpy(buffer, fileDevice->FilePath(),
				bufferSize);
			if (copied < 0)
				return copied;
			return (size_t)copied < bufferSize ? B_OK : B_BUFFER_OVERFLOW;
		}
	}

	return B_ERROR;
}


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


status_t
_user_defragment_partition(partition_id partitionID, int32* _changeCounter)
{
	// copy parameters in
	int32 changeCounter;

	status_t error;
	if ((error = copy_from_user_value(changeCounter, _changeCounter)) != B_OK)
		return error;

	// get the partition
	KDiskDeviceManager* manager = KDiskDeviceManager::Default();
	KPartition* partition = manager->WriteLockPartition(partitionID);
	if (!partition)
		return B_ENTRY_NOT_FOUND;

	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceWriteLocker locker(partition->Device(), true);

	// check change counter
	if (changeCounter != partition->ChangeCounter())
		return B_BAD_VALUE;

	// the partition must be initialized
	KDiskSystem* diskSystem = partition->DiskSystem();
	if (!diskSystem)
		return B_BAD_VALUE;

	// mark the partition busy and unlock
	if (!partition->CheckAndMarkBusy(false))
		return B_BUSY;
	locker.Unlock();

	// defragment
	error = diskSystem->Defragment(partition, DUMMY_JOB_ID);

	// re-lock and unmark busy
	locker.Lock();
	partition->UnmarkBusy(false);

	if (error != B_OK)
		return error;

	// return change counter
	if ((error = copy_to_user_value(_changeCounter, partition->ChangeCounter()))
			!= B_OK) {
		return error;
	}

	return B_OK;
}


status_t
_user_repair_partition(partition_id partitionID, int32* _changeCounter,
	bool checkOnly)
{
	// copy parameters in
	int32 changeCounter;

	status_t error;
	if ((error = copy_from_user_value(changeCounter, _changeCounter)) != B_OK)
		return error;

	// get the partition
	KDiskDeviceManager* manager = KDiskDeviceManager::Default();
	KPartition* partition = manager->WriteLockPartition(partitionID);
	if (!partition)
		return B_ENTRY_NOT_FOUND;

	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceWriteLocker locker(partition->Device(), true);

	// check change counter
	if (changeCounter != partition->ChangeCounter())
		return B_BAD_VALUE;

	// the partition must be initialized
	KDiskSystem* diskSystem = partition->DiskSystem();
	if (!diskSystem)
		return B_BAD_VALUE;

	// mark the partition busy and unlock
	if (!partition->CheckAndMarkBusy(false))
		return B_BUSY;
	locker.Unlock();

	// repair/check
	error = diskSystem->Repair(partition, checkOnly, DUMMY_JOB_ID);

	// re-lock and unmark busy
	locker.Lock();
	partition->UnmarkBusy(false);

	if (error != B_OK)
		return error;

	// return change counter
	if ((error = copy_to_user_value(_changeCounter, partition->ChangeCounter()))
			!= B_OK) {
		return error;
	}

	return B_OK;
}


status_t
_user_resize_partition(partition_id partitionID, int32* _changeCounter,
	partition_id childID, int32* _childChangeCounter, off_t size,
	off_t contentSize)
{
	// copy parameters in
	int32 changeCounter;
	int32 childChangeCounter;

	status_t error;
	if ((error = copy_from_user_value(changeCounter, _changeCounter)) != B_OK
		|| (error = copy_from_user_value(childChangeCounter,
			_childChangeCounter)) != B_OK) {
		return error;
	}

	// get the partition
	KDiskDeviceManager* manager = KDiskDeviceManager::Default();
	KPartition* partition = manager->WriteLockPartition(partitionID);
	if (!partition)
		return B_ENTRY_NOT_FOUND;

	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceWriteLocker locker(partition->Device(), true);

	// register child
	KPartition* child = manager->RegisterPartition(childID);
	if (!child)
		return B_ENTRY_NOT_FOUND;

	PartitionRegistrar registrar3(child, true);

	// check change counters
	if (changeCounter != partition->ChangeCounter()
		|| childChangeCounter != child->ChangeCounter()) {
		return B_BAD_VALUE;
	}

	// the partition must be initialized
	KDiskSystem* diskSystem = partition->DiskSystem();
	if (!diskSystem)
		return B_BAD_VALUE;

	// child must indeed be a child of partition
	if (child->Parent() != partition)
		return B_BAD_VALUE;

	// check sizes
	if (size < 0 || contentSize < 0 || size < contentSize
		|| size > partition->ContentSize()) {
		return B_BAD_VALUE;
	}

	// mark the partitions busy and unlock
	if (partition->IsBusy() || child->IsBusy())
		return B_BUSY;
	partition->SetBusy(true);
	child->SetBusy(true);
	locker.Unlock();

	// resize contents first, if shrinking
	if (child->DiskSystem() && contentSize < child->ContentSize())
		error = child->DiskSystem()->Resize(child, contentSize, DUMMY_JOB_ID);

	// resize the partition
	if (error == B_OK && size != child->Size())
		error = diskSystem->ResizeChild(child, size, DUMMY_JOB_ID);

	// resize contents last, if growing
	if (error == B_OK && child->DiskSystem()
		&& contentSize > child->ContentSize()) {
		error = child->DiskSystem()->Resize(child, contentSize, DUMMY_JOB_ID);
	}

	// re-lock and unmark busy
	locker.Lock();
	partition->SetBusy(false);
	child->SetBusy(false);

	if (error != B_OK)
		return error;

	// return change counters
	if ((error = copy_to_user_value(_changeCounter, partition->ChangeCounter()))
			!= B_OK
		|| (error = copy_to_user_value(_childChangeCounter,
			child->ChangeCounter())) != B_OK) {
		return error;
	}

	return B_OK;
}


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


status_t
_user_set_partition_name(partition_id partitionID, int32* _changeCounter,
	partition_id childID, int32* _childChangeCounter, const char* _name)
{
	// copy parameters in
	UserStringParameter<false> name;
	int32 changeCounter;
	int32 childChangeCounter;

	status_t error;
	if ((error = name.Init(_name, B_DISK_DEVICE_NAME_LENGTH)) != B_OK
		|| (error = copy_from_user_value(changeCounter, _changeCounter))
			!= B_OK
		|| (error = copy_from_user_value(childChangeCounter,
			_childChangeCounter)) != B_OK) {
		return error;
	}

	// get the partition
	KDiskDeviceManager* manager = KDiskDeviceManager::Default();
	KPartition* partition = manager->WriteLockPartition(partitionID);
	if (!partition)
		return B_ENTRY_NOT_FOUND;

	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceWriteLocker locker(partition->Device(), true);

	// register child
	KPartition* child = manager->RegisterPartition(childID);
	if (!child)
		return B_ENTRY_NOT_FOUND;

	PartitionRegistrar registrar3(child, true);

	// check change counters
	if (changeCounter != partition->ChangeCounter()
		|| childChangeCounter != child->ChangeCounter()) {
		return B_BAD_VALUE;
	}

	// the partition must be initialized
	KDiskSystem* diskSystem = partition->DiskSystem();
	if (!diskSystem)
		return B_BAD_VALUE;

	// child must indeed be a child of partition
	if (child->Parent() != partition)
		return B_BAD_VALUE;

	// mark the partitions busy and unlock
	if (partition->IsBusy() || child->IsBusy())
		return B_BUSY;
	partition->SetBusy(true);
	child->SetBusy(true);
	locker.Unlock();

	// set the child name
	error = diskSystem->SetName(child, name.value, DUMMY_JOB_ID);

	// re-lock and unmark busy
	locker.Lock();
	partition->SetBusy(false);
	child->SetBusy(false);

	if (error != B_OK)
		return error;

	// return change counters
	if ((error = copy_to_user_value(_changeCounter, partition->ChangeCounter()))
			!= B_OK
		|| (error = copy_to_user_value(_childChangeCounter,
			child->ChangeCounter())) != B_OK) {
		return error;
	}

	return B_OK;
}


status_t
_user_set_partition_content_name(partition_id partitionID,
	int32* _changeCounter, const char* _name)
{
	// copy parameters in
	UserStringParameter<true> name;
	int32 changeCounter;

	status_t error;
	if ((error = name.Init(_name, B_DISK_DEVICE_NAME_LENGTH)) != B_OK
		|| (error = copy_from_user_value(changeCounter, _changeCounter))
			!= B_OK) {
		return error;
	}

	// get the partition
	KDiskDeviceManager* manager = KDiskDeviceManager::Default();
	KPartition* partition = manager->WriteLockPartition(partitionID);
	if (!partition)
		return B_ENTRY_NOT_FOUND;

	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceWriteLocker locker(partition->Device(), true);

	// check change counter
	if (changeCounter != partition->ChangeCounter())
		return B_BAD_VALUE;

	// the partition must be initialized
	KDiskSystem* diskSystem = partition->DiskSystem();
	if (!diskSystem)
		return B_BAD_VALUE;

	// mark the partition busy and unlock
	if (!partition->CheckAndMarkBusy(false))
		return B_BUSY;
	locker.Unlock();

	// set content parameters
	error = diskSystem->SetContentName(partition, name.value, DUMMY_JOB_ID);

	// re-lock and unmark busy
	locker.Lock();
	partition->UnmarkBusy(false);

	if (error != B_OK)
		return error;

	// return change counter
	if ((error = copy_to_user_value(_changeCounter, partition->ChangeCounter()))
			!= B_OK) {
		return error;
	}

	return B_OK;
}


status_t
_user_set_partition_type(partition_id partitionID, int32* _changeCounter,
	partition_id childID, int32* _childChangeCounter, const char* _type)
{
	// copy parameters in
	UserStringParameter<false> type;
	int32 changeCounter;
	int32 childChangeCounter;

	status_t error;
	if ((error = type.Init(_type, B_DISK_DEVICE_TYPE_LENGTH)) != B_OK
		|| (error = copy_from_user_value(changeCounter, _changeCounter))
			!= B_OK
		|| (error = copy_from_user_value(childChangeCounter,
			_childChangeCounter)) != B_OK) {
		return error;
	}

	// get the partition
	KDiskDeviceManager* manager = KDiskDeviceManager::Default();
	KPartition* partition = manager->WriteLockPartition(partitionID);
	if (!partition)
		return B_ENTRY_NOT_FOUND;

	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceWriteLocker locker(partition->Device(), true);

	// register child
	KPartition* child = manager->RegisterPartition(childID);
	if (!child)
		return B_ENTRY_NOT_FOUND;

	PartitionRegistrar registrar3(child, true);

	// check change counters
	if (changeCounter != partition->ChangeCounter()
		|| childChangeCounter != child->ChangeCounter()) {
		return B_BAD_VALUE;
	}

	// the partition must be initialized
	KDiskSystem* diskSystem = partition->DiskSystem();
	if (!diskSystem)
		return B_BAD_VALUE;

	// child must indeed be a child of partition
	if (child->Parent() != partition)
		return B_BAD_VALUE;

	// mark the partition busy and unlock
	if (partition->IsBusy() || child->IsBusy())
		return B_BUSY;
	partition->SetBusy(true);
	child->SetBusy(true);
	locker.Unlock();

	// set the child type
	error = diskSystem->SetType(child, type.value, DUMMY_JOB_ID);

	// re-lock and unmark busy
	locker.Lock();
	partition->SetBusy(false);
	child->SetBusy(false);

	if (error != B_OK)
		return error;

	// return change counters
	if ((error = copy_to_user_value(_changeCounter, partition->ChangeCounter()))
			!= B_OK
		|| (error = copy_to_user_value(_childChangeCounter,
			child->ChangeCounter())) != B_OK) {
		return error;
	}

	return B_OK;
}


status_t
_user_set_partition_parameters(partition_id partitionID, int32* _changeCounter,
	partition_id childID, int32* _childChangeCounter, const char* _parameters)
{
	// copy parameters in
	UserStringParameter<true> parameters;
	int32 changeCounter;
	int32 childChangeCounter;

	status_t error;
	if ((error = parameters.Init(_parameters, B_DISK_DEVICE_MAX_PARAMETER_SIZE))
			!= B_OK
		|| (error = copy_from_user_value(changeCounter, _changeCounter))
			!= B_OK
		|| (error = copy_from_user_value(childChangeCounter,
			_childChangeCounter)) != B_OK) {
		return error;
	}

	// get the partition
	KDiskDeviceManager* manager = KDiskDeviceManager::Default();
	KPartition* partition = manager->WriteLockPartition(partitionID);
	if (!partition)
		return B_ENTRY_NOT_FOUND;

	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceWriteLocker locker(partition->Device(), true);

	// register child
	KPartition* child = manager->RegisterPartition(childID);
	if (!child)
		return B_ENTRY_NOT_FOUND;

	PartitionRegistrar registrar3(child, true);

	// check change counters
	if (changeCounter != partition->ChangeCounter()
		|| childChangeCounter != child->ChangeCounter()) {
		return B_BAD_VALUE;
	}

	// the partition must be initialized
	KDiskSystem* diskSystem = partition->DiskSystem();
	if (!diskSystem)
		return B_BAD_VALUE;

	// child must indeed be a child of partition
	if (child->Parent() != partition)
		return B_BAD_VALUE;

	// mark the partition busy and unlock
	if (partition->IsBusy() || child->IsBusy())
		return B_BUSY;
	partition->SetBusy(true);
	child->SetBusy(true);
	locker.Unlock();

	// set the child parameters
	error = diskSystem->SetParameters(child, parameters.value, DUMMY_JOB_ID);

	// re-lock and unmark busy
	locker.Lock();
	partition->SetBusy(false);
	child->SetBusy(false);

	if (error != B_OK)
		return error;

	// return change counters
	if ((error = copy_to_user_value(_changeCounter, partition->ChangeCounter()))
			!= B_OK
		|| (error = copy_to_user_value(_childChangeCounter,
			child->ChangeCounter())) != B_OK) {
		return error;
	}

	return B_OK;
}


status_t
_user_set_partition_content_parameters(partition_id partitionID,
	int32* _changeCounter, const char* _parameters)
{
	// copy parameters in
	UserStringParameter<true> parameters;
	int32 changeCounter;

	status_t error
		= parameters.Init(_parameters, B_DISK_DEVICE_MAX_PARAMETER_SIZE);
	if (error != B_OK || (error = copy_from_user_value(changeCounter,
			_changeCounter)) != B_OK) {
		return error;
	}

	// get the partition
	KDiskDeviceManager* manager = KDiskDeviceManager::Default();
	KPartition* partition = manager->WriteLockPartition(partitionID);
	if (!partition)
		return B_ENTRY_NOT_FOUND;

	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceWriteLocker locker(partition->Device(), true);

	// check change counter
	if (changeCounter != partition->ChangeCounter())
		return B_BAD_VALUE;

	// the partition must be initialized
	KDiskSystem* diskSystem = partition->DiskSystem();
	if (!diskSystem)
		return B_BAD_VALUE;

	// mark the partition busy and unlock
	if (!partition->CheckAndMarkBusy(true))
		return B_BUSY;
	locker.Unlock();

	// set content parameters
	error = diskSystem->SetContentParameters(partition, parameters.value,
		DUMMY_JOB_ID);

	// re-lock and unmark busy
	locker.Lock();
	partition->UnmarkBusy(true);

	if (error != B_OK)
		return error;

	// return change counter
	if ((error = copy_to_user_value(_changeCounter, partition->ChangeCounter()))
			!= B_OK) {
		return error;
	}

	return B_OK;
}


status_t
_user_initialize_partition(partition_id partitionID, int32* _changeCounter,
	const char* _diskSystemName, const char* _name, const char* _parameters)
{
	// copy parameters in
	UserStringParameter<false> diskSystemName;
	UserStringParameter<true> name;
	UserStringParameter<true> parameters;
	int32 changeCounter;

	status_t error;
	if ((error = diskSystemName.Init(_diskSystemName,
			B_DISK_SYSTEM_NAME_LENGTH)) != B_OK
		|| (error = name.Init(_name, B_DISK_DEVICE_NAME_LENGTH)) != B_OK
		|| (error = parameters.Init(_parameters,
				B_DISK_DEVICE_MAX_PARAMETER_SIZE)) != B_OK
		|| (error = copy_from_user_value(changeCounter, _changeCounter))
				!= B_OK) {
		return error;
	}

	// get the partition
	KDiskDeviceManager* manager = KDiskDeviceManager::Default();
	KPartition* partition = manager->WriteLockPartition(partitionID);
	if (!partition)
		return B_ENTRY_NOT_FOUND;

	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceWriteLocker locker(partition->Device(), true);

	// check change counter
	if (changeCounter != partition->ChangeCounter())
		return B_BAD_VALUE;

	// the partition must be uninitialized
	if (partition->DiskSystem())
		return B_BAD_VALUE;

	// load the new disk system
	KDiskSystem *diskSystem = manager->LoadDiskSystem(diskSystemName.value,
		true);
	if (!diskSystem)
		return B_ENTRY_NOT_FOUND;
	DiskSystemLoader loader(diskSystem, true);

	// mark the partition busy and unlock
	if (!partition->CheckAndMarkBusy(true))
		return B_BUSY;
	locker.Unlock();

	// let the disk system initialize the partition
	error = diskSystem->Initialize(partition, name.value, parameters.value,
		DUMMY_JOB_ID);

	// re-lock and unmark busy
	locker.Lock();
	partition->UnmarkBusy(true);

	if (error != B_OK)
		return error;

	partition->SetDiskSystem(diskSystem);

	// return change counter
	error = copy_to_user_value(_changeCounter, partition->ChangeCounter());
	if (error != B_OK)
		return error;

	return B_OK;
}


status_t
_user_uninitialize_partition(partition_id partitionID, int32* _changeCounter)
{
	// copy parameters in
	int32 changeCounter;

	status_t error;
	if ((error = copy_from_user_value(changeCounter, _changeCounter)) != B_OK)
		return error;

	// get the partition
	KDiskDeviceManager* manager = KDiskDeviceManager::Default();
	KPartition* partition = manager->WriteLockPartition(partitionID);
	if (!partition)
		return B_ENTRY_NOT_FOUND;

	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceWriteLocker locker(partition->Device(), true);

	// check change counter
	if (changeCounter != partition->ChangeCounter())
		return B_BAD_VALUE;

	// the partition must be initialized
	if (!partition->DiskSystem())
		return B_BAD_VALUE;

	// check busy
	if (!partition->CheckAndMarkBusy(true))
		return B_BUSY;

	// TODO: We should also check, if any partition is mounted!

	KDiskSystem* diskSystem = partition->DiskSystem();

	locker.Unlock();

	// Let the disk system uninitialize the partition. This operation is not
	// mandatory. If implemented, it will destroy the on-disk structures, so
	// that the disk system cannot accidentally identify the partition later on.
	if (diskSystem != NULL)
		diskSystem->Uninitialize(partition, DUMMY_JOB_ID);

	// re-lock and uninitialize the partition object
	locker.Lock();
	error = partition->UninitializeContents(true);

	partition->UnmarkBusy(true);

	if (error != B_OK)
		return error;

	// return change counter
	error = copy_to_user_value(_changeCounter, partition->ChangeCounter());
	if (error != B_OK)
		return error;

	return B_OK;
}


status_t
_user_create_child_partition(partition_id partitionID, int32* _changeCounter,
	off_t offset, off_t size, const char* _type, const char* _name,
	const char* _parameters, partition_id* childID, int32* childChangeCounter)
{
	// copy parameters in
	UserStringParameter<false> type;
	UserStringParameter<true> name;
	UserStringParameter<true> parameters;
	int32 changeCounter;

	status_t error;
	if ((error = type.Init(_type, B_DISK_DEVICE_TYPE_LENGTH)) != B_OK
		|| (error = name.Init(_name, B_DISK_DEVICE_NAME_LENGTH)) != B_OK
		|| (error = parameters.Init(_parameters,
				B_DISK_DEVICE_MAX_PARAMETER_SIZE)) != B_OK
		|| (error = copy_from_user_value(changeCounter, _changeCounter))
				!= B_OK) {
		return error;
	}

	// get the partition
	KDiskDeviceManager* manager = KDiskDeviceManager::Default();
	KPartition* partition = manager->WriteLockPartition(partitionID);
	if (!partition)
		return B_ENTRY_NOT_FOUND;

	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceWriteLocker locker(partition->Device(), true);

	// check change counter
	if (changeCounter != partition->ChangeCounter())
		return B_BAD_VALUE;

	// the partition must be initialized
	KDiskSystem* diskSystem = partition->DiskSystem();
	if (!diskSystem)
		return B_BAD_VALUE;

	// mark the partition busy and unlock
	if (!partition->CheckAndMarkBusy(false))
		return B_BUSY;
	locker.Unlock();

	// create the child
	KPartition *child = NULL;
	error = diskSystem->CreateChild(partition, offset, size, type.value,
		name.value, parameters.value, DUMMY_JOB_ID, &child, -1);

	// re-lock and unmark busy
	locker.Lock();
	partition->UnmarkBusy(false);

	if (error != B_OK)
		return error;

	if (child == NULL)
		return B_ERROR;

	child->UnmarkBusy(true);

	// return change counter and child ID
	if ((error = copy_to_user_value(_changeCounter, partition->ChangeCounter()))
			!= B_OK
		|| (error = copy_to_user_value(childID, child->ID())) != B_OK) {
		return error;
	}

	return B_OK;
}


status_t
_user_delete_child_partition(partition_id partitionID, int32* _changeCounter,
	partition_id childID, int32 childChangeCounter)
{
	// copy parameters in
	int32 changeCounter;

	status_t error;
	if ((error = copy_from_user_value(changeCounter, _changeCounter)) != B_OK)
		return error;

	// get the partition
	KDiskDeviceManager* manager = KDiskDeviceManager::Default();
	KPartition* partition = manager->WriteLockPartition(partitionID);
	if (!partition)
		return B_ENTRY_NOT_FOUND;

	PartitionRegistrar registrar1(partition, true);
	PartitionRegistrar registrar2(partition->Device(), true);
	DeviceWriteLocker locker(partition->Device(), true);

	// register child
	KPartition* child = manager->RegisterPartition(childID);
	if (!child)
		return B_ENTRY_NOT_FOUND;

	PartitionRegistrar registrar3(child, true);

	// check change counters
	if (changeCounter != partition->ChangeCounter()
		|| childChangeCounter != child->ChangeCounter()) {
		return B_BAD_VALUE;
	}

	// the partition must be initialized
	KDiskSystem* diskSystem = partition->DiskSystem();
	if (!diskSystem)
		return B_BAD_VALUE;

	// child must indeed be a child of partition
	if (child->Parent() != partition)
		return B_BAD_VALUE;

	// mark the partition and child busy and unlock
	if (partition->IsBusy() || !child->CheckAndMarkBusy(true))
		return B_BUSY;
	partition->SetBusy(true);
	locker.Unlock();

	// delete the child
	error = diskSystem->DeleteChild(child, DUMMY_JOB_ID);

	// re-lock and unmark busy
	locker.Lock();
	partition->SetBusy(false);
	child->UnmarkBusy(true);

	if (error != B_OK)
		return error;

	// return change counter
	return copy_to_user_value(_changeCounter, partition->ChangeCounter());
}


status_t
_user_start_watching_disks(uint32 eventMask, port_id port, int32 token)
{
	KDiskDeviceManager* manager = KDiskDeviceManager::Default();
	return manager->Notifications().UpdateUserListener(eventMask, port, token);
}


status_t
_user_stop_watching_disks(port_id port, int32 token)
{
	KDiskDeviceManager* manager = KDiskDeviceManager::Default();
	return manager->Notifications().RemoveUserListeners(port, token);
}

