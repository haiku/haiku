// KFileDiskDevice.cpp

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <KernelExport.h>
#include <Drivers.h>

#include <KDiskDeviceUtils.h>
#include <KFileDiskDevice.h>
#include <KPath.h>

#include "virtualdrive.h"

// debugging
//#define DBG(x)
#define DBG(x) x
#define OUT dprintf

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
	KPath tmpFilePath;
	if (filePath[0] != '/') {
		if (tmpFilePath.InitCheck() != B_OK)
			return tmpFilePath.InitCheck();
		// prepend the current directory path
		if (!getcwd(tmpFilePath.LockBuffer(), tmpFilePath.BufferSize() - 1))
			return B_ERROR;
		tmpFilePath.UnlockBuffer();
		if (tmpFilePath.Append(filePath) != B_OK)
			return B_ERROR;
		filePath = tmpFilePath.Path();
	}
	// check the file
	struct stat st;
	if (stat(filePath, &st) != 0)
		return errno;
	if (!S_ISREG(st.st_mode))
		return B_BAD_VALUE;
	// create the device, if requested
	KPath tmpDevicePath;
	if (!devicePath) {
		if (tmpDevicePath.InitCheck() != B_OK)
			return tmpDevicePath.InitCheck();
		// no device path: we shall create a new device entry
		// make the file devices dir
		if (mkdir(kFileDevicesDir, 0777) != 0) {
			if (errno != B_FILE_EXISTS)
				return errno;
		}
		// make the directory
		status_t error = _GetDirectoryPath(ID(), &tmpDevicePath);
		if (error != B_OK)
			return error;
		if (mkdir(tmpDevicePath.Path(), 0777) != 0)
			return errno;
		// get the device path name
		if (tmpDevicePath.Append("raw") != B_OK)
			return B_BUFFER_OVERFLOW;
		devicePath = tmpDevicePath.Path();
		// register the file as virtual drive
		error = _RegisterDevice(filePath, devicePath);
		if (error != B_OK)
			return error;
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
	// remove the device and the directory it resides in
	if (Path() && ID() >= 0) {
		_UnregisterDevice(Path());
		KPath dirPath;
		if (_GetDirectoryPath(ID(), &dirPath) == B_OK)
			rmdir(dirPath.Path());
	}
	// free file path
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

// _RegisterDevice
status_t
KFileDiskDevice::_RegisterDevice(const char *file, const char *device)
{
	// TODO: For now we use the virtualdrive driver to register a file
	// as a device and then simply symlink the assigned device to the
	// desired device location. Replace that with the
	// respective kernel magic for the OBOS kernel!
	// -> Well, we could simply symlink the file there. Doesn't work for R5,
	// but we should be able to deal with it properly.

	// open the virtualdrive control device
	int fd = open(VIRTUAL_DRIVE_CONTROL_DEVICE, O_RDONLY);
	if (fd < 0) {
		DBG(OUT("Failed to open virtualdrive control device: %s\n",
				strerror(errno)));
		return errno;
	}
	// set up the info
	virtual_drive_info info;
	strcpy(info.file_name, file);
	info.use_geometry = false;
	status_t error = B_OK;
	if (ioctl(fd, VIRTUAL_DRIVE_REGISTER_FILE, &info) != 0) {
		error = errno;
		DBG(OUT("Failed to install file device: %s\n", strerror(error)));
	}
	// close the control device
	close(fd);
	// create a symlink
	if (error == B_OK) {
		if (symlink(info.device_name, device) != 0) {
			DBG(OUT("Failed to create file device symlink: %s\n",
					strerror(error)));
			error = errno;
			// unregister the file device
			// open the device
			int deviceFD = open(info.device_name, O_RDONLY);
			if (deviceFD >= 0) {
				ioctl(deviceFD, VIRTUAL_DRIVE_UNREGISTER_FILE);
				close(deviceFD);
			}
		}
	}
	return error;
}

// _UnregisterDevice
status_t
KFileDiskDevice::_UnregisterDevice(const char *_device)
{
return B_ERROR;
#if 0
	// read the symlink to get the path of the virtualdrive device
	char device[B_PATH_NAME_LENGTH];
	ssize_t bytesRead = readlink(_device, device, sizeof(device) - 1);
	if (bytesRead < 0)
		return errno;
	device[bytesRead] = '\0';
	// open the device
	int fd = open(device, O_RDONLY);
	if (fd < 0)
		return errno;
	// issue the ioctl
	status_t error = B_OK;
	if (ioctl(fd, VIRTUAL_DRIVE_UNREGISTER_FILE) != 0)
		error = errno;
	// close the control device
	close(fd);
	// remove the symlink
	if (error == B_OK) {
		if (remove(_device) < 0)
			error = errno;
	}
	return error;
#endif
}

// _GetDirectoryPath
status_t
KFileDiskDevice::_GetDirectoryPath(partition_id id, KPath *path)
{
	if (!path || path->InitCheck() != B_OK)
		return path->InitCheck();
	status_t error = path->SetPath(kFileDevicesDir);
	if (error == B_OK) {
		char idBuffer[12];
		sprintf(idBuffer, "%ld", id);
		error = path->Append(idBuffer);
	}
	return error;
}

