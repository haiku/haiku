/*
 * Copyright 2006-2011, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2003-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "KDiskDevice.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <KernelExport.h>
#include <Drivers.h>

#include "ddm_userland_interface.h"
#include "KDiskDeviceUtils.h"
#include "KPath.h"
#include "UserDataWriter.h"


// debugging
//#define DBG(x)
#define DBG(x) x
#define OUT dprintf


KDiskDevice::KDiskDevice(partition_id id)
	:
	KPartition(id),
	fDeviceData(),
	fFD(-1),
	fMediaStatus(B_ERROR)
{
	rw_lock_init(&fLocker, "disk device");

	Unset();
	fDevice = this;
	fPublishedName = (char*)"raw";
}


KDiskDevice::~KDiskDevice()
{
	Unset();
}


status_t
KDiskDevice::SetTo(const char* path)
{
	// check initialization and parameter
	status_t error = InitCheck();
	if (error != B_OK)
		return error;
	if (!path)
		return B_BAD_VALUE;
	Unset();
	// set the path
	error = set_string(fDeviceData.path, path);
	if (error != B_OK)
		return error;
	// open the device
	fFD = open(path, O_RDONLY);
	if (fFD < 0)
		return errno;
	// get media status
	error = GetMediaStatus(&fMediaStatus);
	if (error != B_OK)
		return error;
	if (fMediaStatus == B_DEV_MEDIA_CHANGED)
		fMediaStatus = B_OK;
	// get device geometry
	if (fMediaStatus == B_OK) {
		error = GetGeometry(&fDeviceData.geometry);
		if (error != B_OK)
			return error;
	} else {
		// no media present: reset the geometry
		_ResetGeometry();
	}

	// set device flags
	_UpdateDeviceFlags();
	// update partition data
	_InitPartitionData();
	return B_OK;
}


void
KDiskDevice::Unset()
{
	if (fFD >= 0) {
		close(fFD);
		fFD = -1;
	}
	fMediaStatus = B_ERROR;
	fDeviceData.id = -1;
	fDeviceData.flags = 0;
	if (fDeviceData.path) {
		free(fDeviceData.path);
		fDeviceData.path = NULL;
	}
	_ResetGeometry();
}


status_t
KDiskDevice::InitCheck() const
{
	return B_OK;
}


bool
KDiskDevice::ReadLock()
{
	return rw_lock_read_lock(&fLocker) == B_OK;
}


void
KDiskDevice::ReadUnlock()
{
	rw_lock_read_unlock(&fLocker);
}


bool
KDiskDevice::WriteLock()
{
	return rw_lock_write_lock(&fLocker) == B_OK;
}


void
KDiskDevice::WriteUnlock()
{
	rw_lock_write_unlock(&fLocker);
}


void
KDiskDevice::SetID(partition_id id)
{
	KPartition::SetID(id);
	fDeviceData.id = id;
}


status_t
KDiskDevice::PublishDevice()
{
	// PublishDevice(), UnpublishDevice() and Republish are no-ops
	// for KDiskDevices, since they are always published.
	return B_OK;
}


status_t
KDiskDevice::UnpublishDevice()
{
	// PublishDevice(), UnpublishDevice() and Republish are no-ops
	// for KDiskDevices, since they are always published.
	return B_OK;
}


status_t
KDiskDevice::RepublishDevice()
{
	// PublishDevice(), UnpublishDevice() and Republish are no-ops
	// for KDiskDevices, since they are always published.
	return B_OK;
}


void
KDiskDevice::SetDeviceFlags(uint32 flags)
{
	fDeviceData.flags = flags;
}


uint32
KDiskDevice::DeviceFlags() const
{
	return fDeviceData.flags;
}


bool
KDiskDevice::IsReadOnlyMedia() const
{
	return fDeviceData.geometry.read_only;
}


bool
KDiskDevice::IsWriteOnce() const
{
	return fDeviceData.geometry.write_once;
}


bool
KDiskDevice::IsRemovable() const
{
	return fDeviceData.geometry.removable;
}


bool
KDiskDevice::HasMedia() const
{
	return fMediaStatus == B_OK || fMediaStatus == B_DEV_MEDIA_CHANGED;
}


bool
KDiskDevice::MediaChanged() const
{
	return fMediaStatus == B_DEV_MEDIA_CHANGED;
}


void
KDiskDevice::UpdateMediaStatusIfNeeded()
{
	// TODO: allow a device to notify us about its media status!
	// This will then also need to clear any B_DEV_MEDIA_CHANGED
	GetMediaStatus(&fMediaStatus);
}


void
KDiskDevice::UninitializeMedia()
{
	UninitializeContents();
	_ResetGeometry();
	_UpdateDeviceFlags();
	_InitPartitionData();
}


void
KDiskDevice::UpdateGeometry()
{
	if (GetGeometry(&fDeviceData.geometry) != B_OK)
		return;

	_UpdateDeviceFlags();
	_InitPartitionData();
}


status_t
KDiskDevice::SetPath(const char* path)
{
	return set_string(fDeviceData.path, path);
}


const char*
KDiskDevice::Path() const
{
	return fDeviceData.path;
}


status_t
KDiskDevice::GetFileName(char* buffer, size_t size) const
{
	if (strlcpy(buffer, "raw", size) >= size)
		return B_NAME_TOO_LONG;
	return B_OK;
}


status_t
KDiskDevice::GetPath(KPath* path) const
{
	if (!path || path->InitCheck() != B_OK)
		return B_BAD_VALUE;
	if (!fDeviceData.path)
		return B_NO_INIT;
	return path->SetPath(fDeviceData.path);
}


void
KDiskDevice::SetFD(int fd)
{
	fFD = fd;
}


int
KDiskDevice::FD() const
{
	return fFD;
}


disk_device_data*
KDiskDevice::DeviceData()
{
	return &fDeviceData;
}


const disk_device_data*
KDiskDevice::DeviceData() const
{
	return &fDeviceData;
}


void
KDiskDevice::WriteUserData(UserDataWriter& writer, user_partition_data* data)
{
	return KPartition::WriteUserData(writer, data);
}


void
KDiskDevice::WriteUserData(UserDataWriter& writer)
{
	KPartition* partition = this;
	user_disk_device_data* data
		= writer.AllocateDeviceData(partition->CountChildren());
	char* path = writer.PlaceString(Path());
	if (data != NULL) {
		data->device_flags = DeviceFlags();
		data->path = path;
		writer.AddRelocationEntry(&data->path);
		partition->WriteUserData(writer, &data->device_partition_data);
	} else
		partition->WriteUserData(writer, NULL);
}


void
KDiskDevice::Dump(bool deep, int32 level)
{
	OUT("device %" B_PRId32 ": %s\n", ID(), Path());
	OUT("  media status:      %s\n", strerror(fMediaStatus));
	OUT("  device flags:      %" B_PRIx32 "\n", DeviceFlags());
	if (fMediaStatus == B_OK)
		KPartition::Dump(deep, 0);
}


status_t
KDiskDevice::GetMediaStatus(status_t* mediaStatus)
{
	status_t error = B_OK;
	if (ioctl(fFD, B_GET_MEDIA_STATUS, mediaStatus) != 0)
		error = errno;
	// maybe the device driver doesn't implement this ioctl -- see, if getting
	// the device geometry succeeds
	if (error != B_OK) {
		device_geometry geometry;
		if (GetGeometry(&geometry) == B_OK) {
			// if the device is not removable, we can ignore the failed ioctl
			// and return a media status of B_OK
			if (!geometry.removable) {
				error = B_OK;
				*mediaStatus = B_OK;
			}
		}
	}
	return error;
}


status_t
KDiskDevice::GetGeometry(device_geometry* geometry)
{
	if (ioctl(fFD, B_GET_GEOMETRY, geometry) != 0)
		return errno;
	return B_OK;
}


void
KDiskDevice::_InitPartitionData()
{
	fDeviceData.id = fPartitionData.id;
	fPartitionData.block_size = fDeviceData.geometry.bytes_per_sector;
	fPartitionData.offset = 0;
	fPartitionData.size = (off_t)fPartitionData.block_size
		* fDeviceData.geometry.sectors_per_track
		* fDeviceData.geometry.cylinder_count
		* fDeviceData.geometry.head_count;
	fPartitionData.flags |= B_PARTITION_IS_DEVICE;

	char name[B_FILE_NAME_LENGTH];
	if (ioctl(fFD, B_GET_DEVICE_NAME, name, sizeof(name)) == B_OK)
		fPartitionData.name = strdup(name);
}


void
KDiskDevice::_ResetGeometry()
{
	fDeviceData.geometry.bytes_per_sector = 0;
	fDeviceData.geometry.sectors_per_track = 0;
	fDeviceData.geometry.cylinder_count = 0;
	fDeviceData.geometry.head_count = 0;
	fDeviceData.geometry.device_type = B_DISK;
	fDeviceData.geometry.removable = true;
	fDeviceData.geometry.read_only = true;
	fDeviceData.geometry.write_once = false;
}


void
KDiskDevice::_UpdateDeviceFlags()
{
	if (fDeviceData.geometry.removable)
		SetDeviceFlags(DeviceFlags() | B_DISK_DEVICE_REMOVABLE);
	if (HasMedia())
		SetDeviceFlags(DeviceFlags() | B_DISK_DEVICE_HAS_MEDIA);
	else
		SetDeviceFlags(DeviceFlags() & ~B_DISK_DEVICE_HAS_MEDIA);

	if (fDeviceData.geometry.read_only)
		SetDeviceFlags(DeviceFlags() | B_DISK_DEVICE_READ_ONLY);
	if (fDeviceData.geometry.write_once)
		SetDeviceFlags(DeviceFlags() | B_DISK_DEVICE_WRITE_ONCE);
}
