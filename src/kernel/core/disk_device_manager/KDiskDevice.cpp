// KDiskDevice.cpp

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <Drivers.h>

#include "KDiskDevice.h"
#include "KDiskDeviceUtils.h"

// constructor
KDiskDevice::KDiskDevice(partition_id id)
	: KPartition(id),
	  fDeviceData(),
	  fLocker("diskdevice"),
	  fFD(-1),
	  fMediaStatus(B_ERROR),
	  fShadowOwner(-1)
{
	Unset();
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
	// get device geometry and media status
	// TODO: files need to be handled differently
	if (ioctl(fFD, B_GET_MEDIA_STATUS, &fMediaStatus) == 0
		&& ioctl(fFD, B_GET_GEOMETRY, &fDeviceData.geometry) == 0) {
		_InitPartitionData();
	}
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
	fShadowOwner = -1;
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
	fDeviceData.geometry.removable = false;
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

// SetID
void
KDiskDevice::SetID(partition_id id)
{
	KPartition::SetID(id);
	fDeviceData.id = id;
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
	return (fMediaStatus == B_OK);
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
KDiskDevice::GetPath(char *path) const
{
	if (!path)
		return B_BAD_VALUE;
	if (!fDeviceData.path)
		return B_NO_INIT;
	strcpy(path, fDeviceData.path);
	return B_OK;
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

// CreateShadowPartition
KPartition *
KDiskDevice::CreateShadowPartition()
{
	// not implemented
	return NULL;
}

// SetShadowOwner
void
KDiskDevice::SetShadowOwner(team_id team)
{
	fShadowOwner = team;
}

// ShadowOwner
team_id
KDiskDevice::ShadowOwner() const
{
	return fShadowOwner;
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
}

