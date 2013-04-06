/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include "Root.h"

#include <Directory.h>
#include <Entry.h>
#include <Path.h>

#include "DebugSupport.h"
#include "Volume.h"


Root::Root()
	:
	fDeviceID(-1),
	fNodeID(-1),
	fPath(),
	fSystemVolume(NULL),
	fCommonVolume(NULL),
	fHomeVolume(NULL)
{
}


Root::~Root()
{
}


status_t
Root::Init(dev_t deviceID, ino_t nodeID)
{
	fDeviceID = deviceID;
	fNodeID = nodeID;

	// get the path
	node_ref nodeRef;
	nodeRef.device = fDeviceID;
	nodeRef.node = fNodeID;
	BDirectory directory;
	status_t error = directory.SetTo(&nodeRef);
	if (error != B_OK) {
		ERROR("Root::Init(): failed to open directory: %s\n", strerror(error));
		return error;
	}

	BEntry entry;
	error = directory.GetEntry(&entry);

	BPath path;
	if (error == B_OK)
		error = entry.GetPath(&path);

	if (error != B_OK) {
		ERROR("Root::Init(): failed to get directory path: %s\n",
			strerror(error));
		RETURN_ERROR(error);
	}

	fPath = path.Path();
	if (fPath.IsEmpty())
		RETURN_ERROR(B_NO_MEMORY);

	return B_OK;
}


status_t
Root::RegisterVolume(Volume* volume)
{
	Volume** volumeToSet = _GetVolume(volume->MountType());
	if (volumeToSet == NULL)
		return B_BAD_VALUE;

	if (*volumeToSet != NULL) {
		ERROR("Root::RegisterVolume(): can't register volume at \"%s\", since "
			"there's already volume at \"%s\" with the same type.\n",
			volume->Path().String(), (*volumeToSet)->Path().String());
		return B_BAD_VALUE;
	}

	*volumeToSet = volume;
	volume->SetRoot(this);

	return B_OK;
}


void
Root::UnregisterVolume(Volume* volume)
{
	Volume** volumeToSet = _GetVolume(volume->MountType());
	if (volumeToSet == NULL || *volumeToSet != volume) {
		ERROR("Root::UnregisterVolume(): can't unregister unknown volume at "
			"\"%s.\n", volume->Path().String());
		return;
	}

	*volumeToSet = NULL;
	volume->SetRoot(NULL);
}


Volume*
Root::FindVolume(dev_t deviceID) const
{
	Volume* volumes[] = { fSystemVolume, fCommonVolume, fHomeVolume };
	for (size_t i = 0; i < sizeof(volumes) / sizeof(volumes[0]); i++) {
		Volume* volume = volumes[i];
		if (volume != NULL && volume->DeviceID() == deviceID)
			return volume;
	}

	return NULL;
}


void
Root::LastReferenceReleased()
{
}


Volume**
Root::_GetVolume(PackageFSMountType mountType)
{
	switch (mountType) {
		case PACKAGE_FS_MOUNT_TYPE_SYSTEM:
			return &fSystemVolume;
		case PACKAGE_FS_MOUNT_TYPE_COMMON:
			return &fCommonVolume;
		case PACKAGE_FS_MOUNT_TYPE_HOME:
			return &fHomeVolume;
		case PACKAGE_FS_MOUNT_TYPE_CUSTOM:
		default:
			return NULL;
	}
}
