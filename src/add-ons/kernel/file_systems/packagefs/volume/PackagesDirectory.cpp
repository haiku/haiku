/*
 * Copyright 2009-2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "PackagesDirectory.h"

#include <errno.h>

#include <fs/KPath.h>
#include <team.h>
#include <vfs.h>

#include "DebugSupport.h"


PackagesDirectory::PackagesDirectory()
	:
	fPath(NULL),
	fDirFD(-1)
{
}


PackagesDirectory::~PackagesDirectory()
{
	if (fDirFD >= 0)
		close(fDirFD);

	free(fPath);
}


status_t PackagesDirectory::Init(const char* path, dev_t mountPointDeviceID,
	ino_t mountPointNodeID, struct stat& _st)
{
	// Open the directory. We want the path be interpreted depending on from
	// where it came (kernel or userland), but we always want a FD in the
	// kernel I/O context. There's no VFS service method to do that for us,
	// so we need to do that ourselves.
	bool calledFromKernel
		= team_get_current_team_id() == team_get_kernel_team_id();
		// Not entirely correct, but good enough for now. The only
		// alternative is to have that information passed in as well.

	struct vnode* vnode;
	status_t error;
	if (path != NULL) {
		error = vfs_get_vnode_from_path(path, calledFromKernel, &vnode);
	} else {
		// No path given -- use the "packages" directory at our mount point.
		error = vfs_entry_ref_to_vnode(mountPointDeviceID, mountPointNodeID,
			"packages", &vnode);
	}
	if (error != B_OK) {
		ERROR("Failed to open package domain \"%s\"\n", strerror(error));
		RETURN_ERROR(error);
	}

	fDirFD = vfs_open_vnode(vnode, O_RDONLY, true);

	if (fDirFD < 0) {
		ERROR("Failed to open package domain \"%s\"\n", strerror(fDirFD));
		vfs_put_vnode(vnode);
		RETURN_ERROR(fDirFD);
	}
	// Our vnode reference has been transferred to the FD.

	// Is it a directory at all?
	struct stat& st = _st;
	if (fstat(fDirFD, &st) < 0)
		RETURN_ERROR(errno);

	fDeviceID = st.st_dev;
	fNodeID = st.st_ino;

	// get a normalized path
	KPath normalizedPath;
	if (normalizedPath.InitCheck() != B_OK)
		RETURN_ERROR(normalizedPath.InitCheck());

	char* normalizedPathBuffer = normalizedPath.LockBuffer();
	error = vfs_entry_ref_to_path(fDeviceID, fNodeID, NULL, true,
		normalizedPathBuffer, normalizedPath.BufferSize());
	if (error != B_OK)
		RETURN_ERROR(error);

	fPath = strdup(normalizedPathBuffer);
	if (fPath == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	return B_OK;
}
