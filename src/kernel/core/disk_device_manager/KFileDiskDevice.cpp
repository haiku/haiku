// KFileDiskDevice.cpp

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <Drivers.h>

#include <KDiskDeviceUtils.h>
#include <KFileDiskDevice.h>

static const char *kFileDevicesDir = "/dev/disk/virtual/files";

// constructor
KFileDiskDevice::KFileDiskDevice(partition_id id)
	: KDiskDevice(id),
	  fFilePath(NULL)
{
}

// destructor
KFileDiskDevice::~KFileDiskDevice()
{
	Unset();
}

// SetTo
status_t
KFileDiskDevice::SetTo(const char *filePath, const char *devicePath)
{
	// check params
	if (!filePath || strlen(filePath) > B_PATH_NAME_LENGTH
		|| (devicePath && strlen(devicePath) > B_PATH_NAME_LENGTH)) {
		return B_BAD_VALUE;
	}
	// normalize the file path
	// TODO: For the time being, we get an absolute file name only.
	char tmpFilePath[B_PATH_NAME_LENGTH];
	if (filePath[0] != '/') {
		int32 len = (int32)B_PATH_NAME_LENGTH - (int32)strlen(filePath) - 2;
		if (len < 0)
			return B_ERROR;
		// prepend the current directory path
		if (!getcwd(tmpFilePath, len))
			return B_ERROR;
		strcat(tmpFilePath, "/");
		strcat(tmpFilePath, filePath);
		filePath = tmpFilePath;
	}
	// check the file
	struct stat st;
	if (stat(filePath, &st) != 0)
		return errno;
	if (!S_ISREG(st.st_mode))
		return B_BAD_VALUE;
	// create the device, if requested
	char tmpDevicePath[B_PATH_NAME_LENGTH];
	if (!devicePath) {
		// no device path: we shall create a new device entry
		// create the device entry
		// TODO: Now we simply create a link. Replace that with the
		// respective kernel magic!
		// make the file devices dir
		if (mkdir(kFileDevicesDir, 0777) != 0) {
			if (errno != B_FILE_EXISTS)
				return errno;
		}
		// make the directory
		sprintf(tmpDevicePath, "%s/%ld", kFileDevicesDir, ID());
		if (mkdir(tmpDevicePath, 0777) != 0)
			return errno;
		// get the device path name
		sprintf(tmpDevicePath, "%s/%ld/raw", kFileDevicesDir, ID());
		devicePath = tmpDevicePath;
		if (symlink(filePath, devicePath) != 0)
			return errno;
	}
	status_t error = set_string(fFilePath, filePath);
	if (error != B_OK)
		return error;
	return KDiskDevice::SetTo(devicePath);
}

// Unset
void
KFileDiskDevice::Unset()
{
	free(fFilePath);
	fFilePath = NULL;
}

// InitCheck
status_t
KFileDiskDevice::InitCheck() const
{
	return KDiskDevice::InitCheck();
}

// FilePath
const char *
KFileDiskDevice::FilePath() const
{
	return fFilePath;
}

// CreateShadowPartition
KPartition *
KFileDiskDevice::CreateShadowPartition()
{
	// not implemented
	return NULL;
}

// GetMediaStatus
status_t
KFileDiskDevice::GetMediaStatus(status_t *mediaStatus)
{
	*mediaStatus = B_OK;
	return B_OK;
}

// GetGeometry
status_t
KFileDiskDevice::GetGeometry(device_geometry *geometry)
{
	// get the file size
	struct stat st;
	if (stat(fFilePath, &st) != 0)
		return errno;
	// default to 512 bytes block size
	off_t size = st.st_size;
	uint32 blockSize = 512;
	// Optimally we have only 1 block per sector and only one head.
	// Since we have only a uint32 for the cylinder count, this won't work
	// for files > 2TB. So, we set the head count to the minimally possible
	// value.
	off_t blocks = size / blockSize;
	uint32 heads = (blocks + ULONG_MAX - 1) / ULONG_MAX;
	if (heads == 0)
		heads = 1;
	geometry->bytes_per_sector = blockSize;
    geometry->sectors_per_track = 1;
    geometry->cylinder_count = blocks / heads;
    geometry->head_count = heads;
    geometry->device_type = B_DISK;	// TODO: Add a new constant.
    geometry->removable = false;
    geometry->read_only = false;
    geometry->write_once = false;
	return B_OK;
}

