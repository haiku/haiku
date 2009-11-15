/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Volume.h"

#include <new>

#include "Directory.h"
#include "kernel_interface.h"


// node ID of the root directory
static const ino_t kRootDirectoryID = 1;


Volume::Volume(fs_volume* fsVolume)
	:
	fFSVolume(fsVolume),
	fRootDirectory(NULL)
{
}


Volume::~Volume()
{
	delete fRootDirectory;
}


status_t
Volume::Mount()
{
	// create the root node
	fRootDirectory = new(std::nothrow) Directory(kRootDirectoryID);
	if (fRootDirectory == NULL)
		return B_NO_MEMORY;

	// publish it
	status_t error = publish_vnode(fFSVolume, fRootDirectory->ID(),
		fRootDirectory, &gPackageFSVnodeOps, S_IFDIR, 0);
	if (error != B_OK)
		return error;

	return B_OK;
}


void
Volume::Unmount()
{
}
