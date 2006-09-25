//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

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
#include <disk_device_manager/ddm_userland_interface.h>


/*!	\class BDiskDevice
	\brief A BDiskDevice object represents a storage device.
*/

// constructor
/*!	\brief Creates an uninitialized BDiskDevice object.
*/
BDiskDevice::BDiskDevice()
	: fDeviceData(NULL)
{
}

// destructor
/*!	\brief Frees all resources associated with this object.
*/
BDiskDevice::~BDiskDevice()
{
}

// HasMedia
/*!	\brief Returns whether the device contains a media.
	\return \c true, if the device contains a media, \c false otherwise.
*/
bool
BDiskDevice::HasMedia() const
{
	return (fDeviceData
			&& fDeviceData->device_flags & B_DISK_DEVICE_HAS_MEDIA);
}

// IsRemovableMedia
/*!	\brief Returns whether the device media are removable.
	\return \c true, if the device media are removable, \c false otherwise.
*/
bool
BDiskDevice::IsRemovableMedia() const
{
	return (fDeviceData
			&& fDeviceData->device_flags & B_DISK_DEVICE_REMOVABLE);
}

// IsReadOnlyMedia
bool
BDiskDevice::IsReadOnlyMedia() const
{
	return (fDeviceData
			&& fDeviceData->device_flags & B_DISK_DEVICE_READ_ONLY);
}

// IsWriteOnceMedia
bool
BDiskDevice::IsWriteOnceMedia() const
{
	return (fDeviceData
			&& fDeviceData->device_flags & B_DISK_DEVICE_WRITE_ONCE);
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
/*	// get path
	const char *path = Path();
	status_t error = (path ? B_OK : B_NO_INIT);
	// check whether the device media is removable
	if (error == B_OK && !IsRemovable())
		error = B_BAD_VALUE;
	// open, eject and close the device
	if (error == B_OK) {
		int fd = open(path, O_RDONLY);
		if (fd < 0 || ioctl(fd, B_EJECT_DEVICE) != 0)
			error = errno;
		if (fd >= 0)
			close(fd);
	}
	if (error == B_OK && update)
		error = Update();
	return error;
*/
	// not implemented
	return B_ERROR;
}

// SetTo
status_t
BDiskDevice::SetTo(partition_id id)
{
	return _SetTo(id, true, false, 0);
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
BDiskDevice::Update(bool *updated)
{
	return _Update(_IsShadow(), updated);
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
	return (fDeviceData ? B_OK : B_NO_INIT);
}

// GetPath
status_t
BDiskDevice::GetPath(BPath *path) const
{
	if (!path || !fDeviceData)
		return B_BAD_VALUE;
	return path->SetTo(fDeviceData->path);
}

// IsModified
bool
BDiskDevice::IsModified() const
{
	return (InitCheck() == B_OK && _IsShadow()
			&& _kern_is_disk_device_modified(ID()));
}

// PrepareModifications
status_t
BDiskDevice::PrepareModifications()
{
	// check initialization
	status_t error = InitCheck();
	if (error != B_OK)
		return error;
	if (_IsShadow())
		return B_ERROR;
	// ask kernel to prepare for modifications
	error = _kern_prepare_disk_device_modifications(ID());
	if (error != B_OK)
		return error;
	// update
	error = _Update(true, NULL);
	if (error != B_OK) {
		// bad -- cancelling the modifications is all we can do
		_kern_cancel_disk_device_modifications(ID());
	}
	return error;
}

// CommitModifications
status_t
BDiskDevice::CommitModifications(bool synchronously,
								 BMessenger progressMessenger,
								 bool receiveCompleteProgressUpdates)
{
	status_t error = InitCheck();
	if (error != B_OK)
		return error;
	if (!_IsShadow())
		return B_BAD_VALUE;
	// TODO: Get port and token from the progressMessenger
	port_id port = -1;
	int32 token = -1;
	error = _kern_commit_disk_device_modifications(ID(), port, token,
		receiveCompleteProgressUpdates);
	if (error == B_OK)
		error = _SetTo(ID(), true, false, 0);
	return error;
}

// CancelModifications
status_t
BDiskDevice::CancelModifications()
{
	status_t error = InitCheck();
	if (error != B_OK)
		return error;
	if (!_IsShadow())
		return B_BAD_VALUE;
	error = _kern_cancel_disk_device_modifications(ID());
	if (error == B_OK)
		error = _SetTo(ID(), true, false, 0);
	return error;
}

// copy constructor
/*!	\brief Privatized copy constructor to avoid usage.
*/
BDiskDevice::BDiskDevice(const BDiskDevice &)
{
}

// =
/*!	\brief Privatized assignment operator to avoid usage.
*/
BDiskDevice &
BDiskDevice::operator=(const BDiskDevice &)
{
	return *this;
}

// _GetData
status_t
BDiskDevice::_GetData(partition_id id, bool deviceOnly, bool shadow,
					  size_t neededSize, user_disk_device_data **data)
{
	// get the device data
	void *buffer = NULL;
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
		error = _kern_get_disk_device_data(id, deviceOnly, shadow,
										   (user_disk_device_data*)buffer,
										   bufferSize, &neededSize);
		if (error == B_BUFFER_OVERFLOW) {
			// buffer to small re-allocate it
			if (buffer)
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
	else if (buffer)
		free(buffer);
	return error;
}

// _SetTo
status_t
BDiskDevice::_SetTo(partition_id id, bool deviceOnly, bool shadow,
					size_t neededSize)
{
	Unset();
	// get the device data
	user_disk_device_data *data = NULL;
	status_t error = _GetData(id, deviceOnly, shadow, neededSize, &data);
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
BDiskDevice::_SetTo(user_disk_device_data *data)
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
BDiskDevice::_Update(bool shadow, bool *updated)
{
	if (InitCheck() != B_OK)
		return InitCheck();
	// get the device data
	user_disk_device_data *data = NULL;
	status_t error = _GetData(ID(), true, shadow, 0, &data);
	// set the data
	if (error == B_OK)
		error = _Update(data, updated);
	// cleanup on error
	if (error != B_OK && data)
		free(data);
	return error;
}

// _Update
status_t
BDiskDevice::_Update(user_disk_device_data *data, bool *updated)
{
	if (!data || !fDeviceData || ID() != data->device_partition_data.id)
		return B_BAD_VALUE;
	bool _updated;
	if (!updated)
		updated = &_updated;
	*updated = false;
	// remove obsolete partitions
	status_t error = _RemoveObsoleteDescendants(&data->device_partition_data,
												updated);
	if (error != B_OK)
		return error;
	// update existing partitions and add new ones
	error = BPartition::_Update(&data->device_partition_data, updated);
	if (error == B_OK) {
		user_disk_device_data *oldData = fDeviceData;
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
BDiskDevice::_AcceptVisitor(BDiskDeviceVisitor *visitor, int32 level)
{
	return visitor->Visit(this);
}

