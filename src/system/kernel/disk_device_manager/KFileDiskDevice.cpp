/*
 * Copyright 2003-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <KFileDiskDevice.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <devfs.h>
#include <Drivers.h>
#include <KernelExport.h>

#include <KDiskDeviceUtils.h>
#include <KPath.h>


// debugging
//#define DBG(x)
#define DBG(x) x
#define OUT dprintf


static const char* kFileDevicesDir = "/dev/disk/virtual/files";


KFileDiskDevice::KFileDiskDevice(partition_id id)
	:
	KDiskDevice(id),
	fFilePath(NULL)
{
	SetDeviceFlags(DeviceFlags() | B_DISK_DEVICE_IS_FILE);
}


KFileDiskDevice::~KFileDiskDevice()
{
	Unset();
}


status_t
KFileDiskDevice::SetTo(const char* filePath, const char* devicePath)
{
	// check params
	if (!filePath || strlen(filePath) > B_PATH_NAME_LENGTH
		|| (devicePath && strlen(devicePath) > B_PATH_NAME_LENGTH)) {
		return B_BAD_VALUE;
	}
	// normalize the file path
	// (should actually not be necessary, since this method is only invoked
	// by the DDM, which has already normalized the path)
	KPath tmpFilePath;
	status_t error = tmpFilePath.SetTo(filePath, true);
	if (error != B_OK)
		return error;
	// check the file
	struct stat st;
	if (stat(filePath, &st) != 0)
		return errno;
	if (!S_ISREG(st.st_mode))
		return B_BAD_VALUE;
	// create the device, if requested
	KPath tmpDevicePath;
	if (devicePath == NULL) {
		// no device path: we shall create a new device entry
		if (tmpDevicePath.InitCheck() != B_OK)
			return tmpDevicePath.InitCheck();
// TODO: Cleanup. The directory creation is done automatically by the devfs.
//		// make the file devices dir
//		if (mkdir(kFileDevicesDir, 0777) != 0) {
//			if (errno != B_FILE_EXISTS)
//				return errno;
//		}
		// make the directory
		status_t error = _GetDirectoryPath(ID(), &tmpDevicePath);
		if (error != B_OK)
			return error;
//		if (mkdir(tmpDevicePath.Path(), 0777) != 0)
//			return errno;
		// get the device path name
		error = tmpDevicePath.Append("raw");
		if (error != B_OK)
			return error;
		devicePath = tmpDevicePath.Path();
		// register the file as virtual disk device
		error = _RegisterDevice(filePath, devicePath);
		if (error != B_OK)
			return error;
	}
	error = set_string(fFilePath, filePath);
	if (error != B_OK)
		return error;

	error = KDiskDevice::SetTo(devicePath);
	if (error != B_OK)
		return error;

	// reset the B_DISK_DEVICE_IS_FILE flag -- KDiskDevice::SetTo() has cleared
	// it
	SetDeviceFlags(DeviceFlags() | B_DISK_DEVICE_IS_FILE);

	return B_OK;
}


void
KFileDiskDevice::Unset()
{
	// remove the device and the directory it resides in
	if (Path() && ID() >= 0) {
		_UnregisterDevice(Path());
// TODO: Cleanup. The devfs will automatically remove the directory.
//		KPath dirPath;
//		if (_GetDirectoryPath(ID(), &dirPath) == B_OK)
//			rmdir(dirPath.Path());
	}
	// free file path
	free(fFilePath);
	fFilePath = NULL;
}


status_t
KFileDiskDevice::InitCheck() const
{
	return KDiskDevice::InitCheck();
}


const char*
KFileDiskDevice::FilePath() const
{
	return fFilePath;
}


status_t
KFileDiskDevice::GetMediaStatus(status_t* mediaStatus)
{
	// check the file
	struct stat st;
	if (stat(fFilePath, &st) == 0 && S_ISREG(st.st_mode))
		*mediaStatus = B_OK;
	else
		*mediaStatus = B_DEV_NO_MEDIA;
	return B_OK;
}


status_t
KFileDiskDevice::GetGeometry(device_geometry* geometry)
{
	// check the file
	struct stat st;
	if (stat(fFilePath, &st) != 0 || !S_ISREG(st.st_mode))
		return B_BAD_VALUE;

	// fill in the geometry
	// default to 512 bytes block size
	uint32 blockSize = 512;
	// Optimally we have only 1 block per sector and only one head.
	// Since we have only a uint32 for the cylinder count, this won't work
	// for files > 2TB. So, we set the head count to the minimally possible
	// value.
	off_t blocks = st.st_size / blockSize;
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


status_t
KFileDiskDevice::_RegisterDevice(const char* file, const char* device)
{
	return devfs_publish_file_device(device + 5, file);
		// we need to remove the "/dev/" part from the path
}


status_t
KFileDiskDevice::_UnregisterDevice(const char* _device)
{
	return devfs_unpublish_file_device(_device + 5);
		// we need to remove the "/dev/" part from the path
}


status_t
KFileDiskDevice::_GetDirectoryPath(partition_id id, KPath* path)
{
	if (path == NULL)
		return B_BAD_VALUE;

	if (path->InitCheck() != B_OK)
		return path->InitCheck();

	status_t error = path->SetPath(kFileDevicesDir);
	if (error == B_OK) {
		char idBuffer[12];
		snprintf(idBuffer, sizeof(idBuffer), "%" B_PRId32, id);
		error = path->Append(idBuffer);
	}
	return error;
}

