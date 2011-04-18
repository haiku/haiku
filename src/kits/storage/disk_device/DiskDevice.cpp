/*
 * Copyright 2003-2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <DiskDevice.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <DiskDevice.h>
#include <DiskDeviceVisitor.h>
#include <Drivers.h>
#include <Message.h>
#include <Path.h>

#include <syscalls.h>
#include <ddm_userland_interface_defs.h>

#include "DiskDeviceJob.h"
#include "DiskDeviceJobGenerator.h"
#include "DiskDeviceJobQueue.h"
#include <DiskSystemAddOnManager.h>


//#define TRACE_DISK_DEVICE
#undef TRACE
#ifdef TRACE_DISK_DEVICE
# define TRACE(x...) printf(x)
#else
# define TRACE(x...) do {} while (false)
#endif


/*!	\class BDiskDevice
	\brief A BDiskDevice object represents a storage device.
*/


// constructor
/*!	\brief Creates an uninitialized BDiskDevice object.
*/
BDiskDevice::BDiskDevice()
	:
	fDeviceData(NULL)
{
}


// destructor
/*!	\brief Frees all resources associated with this object.
*/
BDiskDevice::~BDiskDevice()
{
	CancelModifications();
}


// HasMedia
/*!	\brief Returns whether the device contains a media.
	\return \c true, if the device contains a media, \c false otherwise.
*/
bool
BDiskDevice::HasMedia() const
{
	return fDeviceData
		&& (fDeviceData->device_flags & B_DISK_DEVICE_HAS_MEDIA) != 0;
}


// IsRemovableMedia
/*!	\brief Returns whether the device media are removable.
	\return \c true, if the device media are removable, \c false otherwise.
*/
bool
BDiskDevice::IsRemovableMedia() const
{
	return fDeviceData
		&& (fDeviceData->device_flags & B_DISK_DEVICE_REMOVABLE) != 0;
}


// IsReadOnlyMedia
bool
BDiskDevice::IsReadOnlyMedia() const
{
	return fDeviceData
		&& (fDeviceData->device_flags & B_DISK_DEVICE_READ_ONLY) != 0;
}


// IsWriteOnceMedia
bool
BDiskDevice::IsWriteOnceMedia() const
{
	return fDeviceData
		&& (fDeviceData->device_flags & B_DISK_DEVICE_WRITE_ONCE) != 0;
}


// Eject
/*!	\brief Eject the device's media.

	The device media must, of course, be removable, and the device must
	support ejecting the media.

	\param update If \c true, Update() is invoked after successful ejection.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The device is not properly initialized.
	- \c B_BAD_VALUE: The device media is not removable.
	- other error codes
*/
status_t
BDiskDevice::Eject(bool update)
{
	if (fDeviceData == NULL)
		return B_NO_INIT;

	// check whether the device media is removable
	if (!IsRemovableMedia())
		return B_BAD_VALUE;

	// open, eject and close the device
	int fd = open(fDeviceData->path, O_RDONLY);
	if (fd < 0)
		return errno;

	status_t status = B_OK;
	if (ioctl(fd, B_EJECT_DEVICE) != 0)
		status = errno;

	close(fd);

	if (status == B_OK && update)
		status = Update();

	return status;
}


// SetTo
status_t
BDiskDevice::SetTo(partition_id id)
{
	return _SetTo(id, true, 0);
}


// Update
/*!	\brief Updates the object to reflect the latest changes to the device.

	Note, that subobjects (BSessions, BPartitions) may be deleted during this
	operation. It is also possible, that the device doesn't exist anymore --
	e.g. if it is hot-pluggable. Then an error is returned and the object is
	uninitialized.

	\param updated Pointer to a bool variable which shall be set to \c true,
		   if the object needed to be updated and to \c false otherwise.
		   May be \c NULL. Is not touched, if the method fails.
	\return \c B_OK, if the update went fine, another error code otherwise.
*/
status_t
BDiskDevice::Update(bool* updated)
{
	if (InitCheck() != B_OK)
		return InitCheck();

	// get the device data
	user_disk_device_data* data = NULL;
	status_t error = _GetData(ID(), true, 0, &data);

	// set the data
	if (error == B_OK)
		error = _Update(data, updated);

	// cleanup on error
	if (error != B_OK && data)
		free(data);

	return error;
}


// Unset
void
BDiskDevice::Unset()
{
	BPartition::_Unset();
	free(fDeviceData);
	fDeviceData = NULL;
}


// InitCheck
status_t
BDiskDevice::InitCheck() const
{
	return fDeviceData ? B_OK : B_NO_INIT;
}


// GetPath
status_t
BDiskDevice::GetPath(BPath* path) const
{
	if (!path || !fDeviceData)
		return B_BAD_VALUE;
	return path->SetTo(fDeviceData->path);
}


// IsModified
bool
BDiskDevice::IsModified() const
{
	if (InitCheck() != B_OK)
		return false;

	struct IsModifiedVisitor : public BDiskDeviceVisitor {
		virtual bool Visit(BDiskDevice* device)
		{
			return Visit(device, 0);
		}

		virtual bool Visit(BPartition* partition, int32 level)
		{
			return partition->_IsModified();
		}
	} visitor;

	return (VisitEachDescendant(&visitor) != NULL);
}


// PrepareModifications
/*!	\brief Initializes the partition hierarchy for modifications.
 *
 * 	Subsequent modifications are performed on so-called \a shadow structure
 * 	and not written to device until \ref CommitModifications is called.
 *
 * 	\note This call locks the device. You need to call \ref CommitModifications
 * 		or \ref CancelModifications to unlock it.
 */
status_t
BDiskDevice::PrepareModifications()
{
	TRACE("%p->BDiskDevice::PrepareModifications()\n", this);

	// check initialization
	status_t error = InitCheck();
	if (error != B_OK) {
		TRACE("  InitCheck() failed\n");
		return error;
	}
	if (fDelegate) {
		TRACE("  already prepared!\n");
		return B_BAD_VALUE;
	}

	// make sure the disk system add-ons are loaded
	error = DiskSystemAddOnManager::Default()->LoadDiskSystems();
	if (error != B_OK) {
		TRACE("  failed to load disk systems\n");
		return error;
	}

	// recursively create the delegates
	error = _CreateDelegates();

	// init them
	if (error == B_OK)
		error = _InitDelegates();
	else
		TRACE("  failed to create delegates\n");

	// delete all of them, if something went wrong
	if (error != B_OK) {
		TRACE("  failed to init delegates\n");
		_DeleteDelegates();
		DiskSystemAddOnManager::Default()->UnloadDiskSystems();
	}

	return error;
}


// CommitModifications
/*!	\brief Commits modifications to device.
 *
 * 	Creates a set of jobs that perform all the changes done after
 * 	\ref PrepareModifications. The changes are then written to device.
 * 	Deletes and recreates all BPartition children to apply the changes,
 * 	so cached pointers to BPartition objects cannot be used after this
 * 	call.
 * 	Unlocks the device for further use.
 */
status_t
BDiskDevice::CommitModifications(bool synchronously,
	BMessenger progressMessenger, bool receiveCompleteProgressUpdates)
{
// TODO: Support parameters!
	status_t error = InitCheck();
	if (error != B_OK)
		return error;

	if (!fDelegate)
		return B_BAD_VALUE;

	// generate jobs
	DiskDeviceJobQueue jobQueue;
	error = DiskDeviceJobGenerator(this, &jobQueue).GenerateJobs();

	// do the jobs
	if (error == B_OK)
		error = jobQueue.Execute();

	_DeleteDelegates();
	DiskSystemAddOnManager::Default()->UnloadDiskSystems();

	if (error == B_OK)
		error = _SetTo(ID(), true, 0);

	return error;
}


// CancelModifications
/*!	\brief Cancels all modifications performed on the device.
 *
 * 	Nothing is written on the device and it is unlocked for further use.
 */
status_t
BDiskDevice::CancelModifications()
{
	status_t error = InitCheck();
	if (error != B_OK)
		return error;

	if (!fDelegate)
		return B_BAD_VALUE;

	_DeleteDelegates();
	DiskSystemAddOnManager::Default()->UnloadDiskSystems();

	if (error == B_OK)
		error = _SetTo(ID(), true, 0);

	return error;
}


/*!	\brief Returns whether or not this device is a virtual device backed
		up by a file.
*/
bool
BDiskDevice::IsFile() const
{
	return fDeviceData
		&& (fDeviceData->device_flags & B_DISK_DEVICE_IS_FILE) != 0;
}


/*!	\brief Retrieves the path of the file backing up the disk device.*/
status_t
BDiskDevice::GetFilePath(BPath* path) const
{
	if (path == NULL)
		return B_BAD_VALUE;
	if (!IsFile())
		return B_BAD_TYPE;

	char pathBuffer[B_PATH_NAME_LENGTH];
	status_t status = _kern_get_file_disk_device_path(
		fDeviceData->device_partition_data.id, pathBuffer, sizeof(pathBuffer));
	if (status != B_OK)
		return status;

	return path->SetTo(pathBuffer);
}


// copy constructor
/*!	\brief Privatized copy constructor to avoid usage.
*/
BDiskDevice::BDiskDevice(const BDiskDevice&)
{
}


// =
/*!	\brief Privatized assignment operator to avoid usage.
*/
BDiskDevice&
BDiskDevice::operator=(const BDiskDevice&)
{
	return *this;
}


// _GetData
status_t
BDiskDevice::_GetData(partition_id id, bool deviceOnly, size_t neededSize,
	user_disk_device_data** data)
{
	// get the device data
	void* buffer = NULL;
	size_t bufferSize = 0;
	if (neededSize > 0) {
		// allocate initial buffer
		buffer = malloc(neededSize);
		if (!buffer)
			return B_NO_MEMORY;
		bufferSize = neededSize;
	}

	status_t error = B_OK;
	do {
		error = _kern_get_disk_device_data(id, deviceOnly,
			(user_disk_device_data*)buffer, bufferSize, &neededSize);
		if (error == B_BUFFER_OVERFLOW) {
			// buffer to small re-allocate it
			free(buffer);

			buffer = malloc(neededSize);

			if (buffer)
				bufferSize = neededSize;
			else
				error = B_NO_MEMORY;
		}
	} while (error == B_BUFFER_OVERFLOW);

	// set result / cleanup on error
	if (error == B_OK)
		*data = (user_disk_device_data*)buffer;
	else
		free(buffer);

	return error;
}


// _SetTo
status_t
BDiskDevice::_SetTo(partition_id id, bool deviceOnly, size_t neededSize)
{
	Unset();

	// get the device data
	user_disk_device_data* data = NULL;
	status_t error = _GetData(id, deviceOnly, neededSize, &data);

	// set the data
	if (error == B_OK)
		error = _SetTo(data);

	// cleanup on error
	if (error != B_OK && data)
		free(data);

	return error;
}


// _SetTo
status_t
BDiskDevice::_SetTo(user_disk_device_data* data)
{
	Unset();

	if (!data)
		return B_BAD_VALUE;

	fDeviceData = data;

	status_t error = BPartition::_SetTo(this, NULL,
		&fDeviceData->device_partition_data);
	if (error != B_OK) {
		// If _SetTo() fails, the caller retains ownership of the supplied
		// data. So, unset fDeviceData before calling Unset().
		fDeviceData = NULL;
		Unset();
	}

	return error;
}


// _Update
status_t
BDiskDevice::_Update(user_disk_device_data* data, bool* updated)
{
	if (!data || !fDeviceData || ID() != data->device_partition_data.id)
		return B_BAD_VALUE;
	bool _updated;
	if (!updated)
		updated = &_updated;
	*updated = false;

	// clear the user_data fields first
	_ClearUserData(&data->device_partition_data);

	// remove obsolete partitions
	status_t error = _RemoveObsoleteDescendants(&data->device_partition_data,
		updated);
	if (error != B_OK)
		return error;

	// update existing partitions and add new ones
	error = BPartition::_Update(&data->device_partition_data, updated);
	if (error == B_OK) {
		user_disk_device_data* oldData = fDeviceData;
		fDeviceData = data;
		// check for changes
		if (data->device_flags != oldData->device_flags
			|| strcmp(data->path, oldData->path)) {
			*updated = true;
		}
		free(oldData);
	}

	return error;
}


// _AcceptVisitor
bool
BDiskDevice::_AcceptVisitor(BDiskDeviceVisitor* visitor, int32 level)
{
	return visitor->Visit(this);
}


// _ClearUserData
void
BDiskDevice::_ClearUserData(user_partition_data* data)
{
	data->user_data = NULL;

	// recurse
	for (int i = 0; i < data->child_count; i++)
		_ClearUserData(data->children[i]);
}
