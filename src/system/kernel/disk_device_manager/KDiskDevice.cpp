// KDiskDevice.cpp

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <KernelExport.h>
#include <Drivers.h>

#include "ddm_userland_interface.h"
#include "KDiskDevice.h"
#include "KDiskDeviceUtils.h"
#include "KPath.h"
#include "UserDataWriter.h"

// debugging
//#define DBG(x)
#define DBG(x) x
#define OUT dprintf

// constructor
KDiskDevice::KDiskDevice(partition_id id)
	: KPartition(id),
	  fDeviceData(),
	  fLocker("diskdevice"),
	  fFD(-1),
	  fMediaStatus(B_ERROR)
{
	Unset();
	fDevice = this;
	fPublished = true;
}

// destructor
KDiskDevice::~KDiskDevice()
{
	Unset();
}

// SetTo
status_t
KDiskDevice::SetTo(const char *path)
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
		// no media: try to get the device geometry, but don't fail, if
		// we can't get it
		GetGeometry(&fDeviceData.geometry);
	}
	// set device flags
	if (fDeviceData.geometry.removable)
		SetDeviceFlags(DeviceFlags() | B_DISK_DEVICE_REMOVABLE);
	if (fMediaStatus == B_OK)
		SetDeviceFlags(DeviceFlags() | B_DISK_DEVICE_HAS_MEDIA);
	if (fDeviceData.geometry.read_only)
		SetDeviceFlags(DeviceFlags() | B_DISK_DEVICE_READ_ONLY);
	if (fDeviceData.geometry.write_once)
		SetDeviceFlags(DeviceFlags() | B_DISK_DEVICE_WRITE_ONCE);
	// update partition data
	_InitPartitionData();
	return B_OK;
}

// Unset
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
	fDeviceData.geometry.bytes_per_sector = 0;
	fDeviceData.geometry.sectors_per_track = 0;
	fDeviceData.geometry.cylinder_count = 0;
	fDeviceData.geometry.head_count = 0;
	fDeviceData.geometry.device_type = B_DISK;
	fDeviceData.geometry.removable = true;
	fDeviceData.geometry.read_only = true;
	fDeviceData.geometry.write_once = false;
}

// InitCheck
status_t
KDiskDevice::InitCheck() const
{
	return fLocker.InitCheck();
}

// ReadLock
bool
KDiskDevice::ReadLock()
{
	return fLocker.ReadLock();
}

// ReadUnlock
void
KDiskDevice::ReadUnlock()
{
	fLocker.ReadUnlock();
}

// IsReadLocked
bool
KDiskDevice::IsReadLocked(bool orWriteLocked)
{
	return fLocker.IsReadLocked(orWriteLocked);
}

// WriteLock
bool
KDiskDevice::WriteLock()
{
	return fLocker.WriteLock();
}

// WriteUnlock
void
KDiskDevice::WriteUnlock()
{
	fLocker.WriteUnlock();
}

// IsWriteLocked
bool
KDiskDevice::IsWriteLocked()
{
	return fLocker.IsWriteLocked();
}


// SetID
void
KDiskDevice::SetID(partition_id id)
{
	KPartition::SetID(id);
	fDeviceData.id = id;
}

// PublishDevice
status_t
KDiskDevice::PublishDevice()
{
	// PublishDevice() and UnpublishDevice() are no-ops for KDiskDevices,
	// since they are always published.
	return B_OK;
}

// UnpublishDevice
status_t
KDiskDevice::UnpublishDevice()
{
	// PublishDevice() and UnpublishDevice() are no-ops for KDiskDevices,
	// since they are always published.
	return B_OK;
}


// SetDeviceFlags
void
KDiskDevice::SetDeviceFlags(uint32 flags)
{
	fDeviceData.flags = flags;
}

// DeviceFlags
uint32
KDiskDevice::DeviceFlags() const
{
	return fDeviceData.flags;
}

// IsReadOnlyMedia
bool
KDiskDevice::IsReadOnlyMedia() const
{
	return fDeviceData.geometry.read_only;
}

// IsWriteOnce
bool
KDiskDevice::IsWriteOnce() const
{
	return fDeviceData.geometry.write_once;
}

// IsRemovable
bool
KDiskDevice::IsRemovable() const
{
	return fDeviceData.geometry.removable;
}

// HasMedia
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


// SetPath
status_t
KDiskDevice::SetPath(const char *path)
{
	return set_string(fDeviceData.path, path);
}

// Path
const char *
KDiskDevice::Path() const
{
	return fDeviceData.path;
}

// GetPath
status_t
KDiskDevice::GetPath(KPath *path) const
{
	if (!path || path->InitCheck() != B_OK)
		return B_BAD_VALUE;
	if (!fDeviceData.path)
		return B_NO_INIT;
	return path->SetPath(fDeviceData.path);
}

// SetFD
void
KDiskDevice::SetFD(int fd)
{
	fFD = fd;
}

// FD
int
KDiskDevice::FD() const
{
	return fFD;
}

// DeviceData
disk_device_data *
KDiskDevice::DeviceData()
{
	return &fDeviceData;
}

// DeviceData
const disk_device_data *
KDiskDevice::DeviceData() const
{
	return &fDeviceData;
}


// WriteUserData
void
KDiskDevice::WriteUserData(UserDataWriter &writer, user_partition_data *data)
{
	return KPartition::WriteUserData(writer, data);
}

// WriteUserData
void
KDiskDevice::WriteUserData(UserDataWriter &writer)
{
	KPartition *partition = this;
	user_disk_device_data *data
		= writer.AllocateDeviceData(partition->CountChildren());
	char *path = writer.PlaceString(Path());
	if (data) {
		data->device_flags = DeviceFlags();
		data->path = path;
		writer.AddRelocationEntry(&data->path);
		partition->WriteUserData(writer, &data->device_partition_data);
	} else
		partition->WriteUserData(writer, NULL);
}

// Dump
void
KDiskDevice::Dump(bool deep, int32 level)
{
	OUT("device %ld: %s\n", ID(), Path());
	OUT("  media status:      %s\n", strerror(fMediaStatus));
	OUT("  device flags:      %lx\n", DeviceFlags());
	if (fMediaStatus == B_OK)
		KPartition::Dump(deep, 0);
}

// GetMediaStatus
status_t
KDiskDevice::GetMediaStatus(status_t *mediaStatus)
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

// GetGeometry
status_t
KDiskDevice::GetGeometry(device_geometry *geometry)
{
	if (ioctl(fFD, B_GET_GEOMETRY, geometry) != 0)
		return errno;
	return B_OK;
}

// _InitPartitionData
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
}

