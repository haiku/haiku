/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include "Volume.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>

#include <new>

#include <driver_settings.h>
#include <KernelExport.h>
#include <vfs.h>

#include <AutoDeleter.h>
#include <AutoDeleterDrivers.h>

#include "DebugSupport.h"
#include "kernel_interface.h"
#include "Node.h"


// #pragma mark - Volume


Volume::Volume(fs_volume* fsVolume)
	:
	fFSVolume(fsVolume),
	fSourceFSVolume(NULL),
	fSourceVnode(NULL),
	fRootNode(NULL)
{
}


Volume::~Volume()
{
	if (fSourceVnode != NULL)
		vfs_put_vnode(fSourceVnode);
}


status_t
Volume::Mount(const char* parameterString)
{
	const char* source = NULL;
	void* parameterHandle = parse_driver_settings_string(parameterString);
	DriverSettingsUnloader parameterDeleter(parameterHandle);
	if (parameterHandle != NULL)
		source = get_driver_parameter(parameterHandle, "source", NULL, NULL);
	if (source == NULL || source[0] == '\0') {
		ERROR("need source folder ('source' parameter)!\n");
		RETURN_ERROR(B_BAD_VALUE);
	}

	status_t error = vfs_get_vnode_from_path(source, true, &fSourceVnode);
	if (error != B_OK)
		RETURN_ERROR(error);
	if (fSourceVnode == NULL)
		RETURN_ERROR(B_ENTRY_NOT_FOUND);
	fs_vnode* sourceFSNode = vfs_fsnode_for_vnode(fSourceVnode);
	fSourceFSVolume = volume_for_vnode(sourceFSNode);

	struct stat st;
	if ((stat(source, &st)) != 0)
		RETURN_ERROR(B_ERROR);

	strlcpy(fName, "bindfs:", sizeof(fName));
	strlcpy(fName, source, sizeof(fName));

	// create the root node
	fRootNode = new(std::nothrow) Node(st.st_ino, st.st_mode);
	if (fRootNode == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	_InitVnodeOpsFrom(sourceFSNode);

	// publish the root node
	error = publish_vnode(fFSVolume, fRootNode->ID(), fRootNode, &fVnodeOps,
		fRootNode->Mode() & S_IFMT, 0);
	if (error != B_OK) {
		delete fRootNode;
		fRootNode = NULL;
		return error;
	}

	return B_OK;
}


void
Volume::Unmount()
{
}


status_t
Volume::_InitVnodeOpsFrom(fs_vnode* sourceNode)
{
	// vnode ops
	int opsCount = sizeof(fVnodeOps) / sizeof(void*);
	for (int i = 0; i < opsCount; ++i) {
		if (((void**)sourceNode->ops)[i] == NULL)
			((void**)&fVnodeOps)[i] = NULL;
		else
			((void**)&fVnodeOps)[i] = ((void**)&gBindFSVnodeOps)[i];
	}

	return B_OK;
}
