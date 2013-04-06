/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include "Volume.h"

#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

#include <Directory.h>
#include <Entry.h>
#include <Path.h>

#include "DebugSupport.h"


Volume::Volume()
	:
	fPath(),
	fMountType(PACKAGE_FS_MOUNT_TYPE_CUSTOM),
	fDeviceID(-1),
	fRootDirectoryID(1),
	fRoot(NULL)
{
}


Volume::~Volume()
{
}


status_t
Volume::Init(BDirectory& directory, dev_t& _rootDeviceID, ino_t& _rootNodeID)
{
	// get the directory path
	BEntry entry;
	status_t error = directory.GetEntry(&entry);

	BPath path;
	if (error == B_OK)
		error = entry.GetPath(&path);

	if (error != B_OK) {
		ERROR("Volume::Init(): failed to get root directory path: %s\n",
			strerror(error));
		RETURN_ERROR(error);
	}

	fPath = path.Path();
	if (fPath.IsEmpty())
		RETURN_ERROR(B_NO_MEMORY);

	// stat() the directory
	struct stat st;
	error = directory.GetStat(&st);
	if (error != B_OK) {
		ERROR("Volume::Init(): failed to stat root directory: %s\n",
			strerror(error));
		RETURN_ERROR(error);
	}

	fDeviceID = st.st_dev;
	fRootDirectoryID = st.st_ino;

	// get a volume info from the FS
	int fd = directory.Dup();
	if (fd < 0) {
		ERROR("Volume::Init(): failed to get root directory FD: %s\n",
			strerror(fd));
		RETURN_ERROR(fd);
	}

	PackageFSVolumeInfo info;
	if (ioctl(fd, PACKAGE_FS_OPERATION_GET_VOLUME_INFO, &info, sizeof(info))
			!= 0) {
		error = errno;
		close(fd);
		if (error != B_OK) {
			ERROR("Volume::Init(): failed to get volume info: %s\n",
				strerror(error));
			RETURN_ERROR(error);
		}
	}

	close(fd);

	fMountType = info.mountType;
	_rootDeviceID = info.rootDeviceID;
	_rootNodeID = info.rootDirectoryID;

	return B_OK;
}
