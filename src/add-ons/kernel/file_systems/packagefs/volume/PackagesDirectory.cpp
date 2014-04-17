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
	fStateName(),
	fPath(NULL),
	fDirFD(-1),
	fNodeRef(),
	fHashNext(NULL)
{
}


PackagesDirectory::~PackagesDirectory()
{
	if (fDirFD >= 0)
		close(fDirFD);

	free(fPath);
}


/*static*/ bool
PackagesDirectory::IsNewer(const PackagesDirectory* a,
	const PackagesDirectory* b)
{
	if (b->fStateName.IsEmpty())
		return false;
	if (a->fStateName.IsEmpty())
		return true;
	return strcmp(a->fStateName, b->fStateName) > 0;
}


status_t
PackagesDirectory::Init(const char* path, dev_t mountPointDeviceID,
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
		ERROR("Failed to open packages directory \"%s\"\n", strerror(error));
		RETURN_ERROR(error);
	}

	return _Init(vnode, _st);
}


status_t
PackagesDirectory::InitOldState(dev_t adminDirDeviceID, ino_t adminDirNodeID,
	const char* stateName)
{
	if (!fStateName.SetTo(stateName))
		RETURN_ERROR(B_NO_MEMORY);

	struct vnode* vnode;
	status_t error = vfs_entry_ref_to_vnode(adminDirDeviceID, adminDirNodeID,
		stateName, &vnode);
	if (error != B_OK) {
		ERROR("Failed to open old state directory \"%s\"\n", strerror(error));
		RETURN_ERROR(error);
	}

	struct stat st;
	return _Init(vnode, st);
}


status_t
PackagesDirectory::_Init(struct vnode* vnode, struct stat& _st)
{
	fDirFD = vfs_open_vnode(vnode, O_RDONLY, true);

	if (fDirFD < 0) {
		ERROR("Failed to open packages directory \"%s\"\n", strerror(fDirFD));
		vfs_put_vnode(vnode);
		RETURN_ERROR(fDirFD);
	}
	// Our vnode reference has been transferred to the FD.

	// Is it a directory at all?
	struct stat& st = _st;
	if (fstat(fDirFD, &st) < 0)
		RETURN_ERROR(errno);

	fNodeRef.device = st.st_dev;
	fNodeRef.node = st.st_ino;

	// get a normalized path
	KPath normalizedPath;
	if (normalizedPath.InitCheck() != B_OK)
		RETURN_ERROR(normalizedPath.InitCheck());

	char* normalizedPathBuffer = normalizedPath.LockBuffer();
	status_t error = vfs_entry_ref_to_path(fNodeRef.device, fNodeRef.node, NULL,
		true, normalizedPathBuffer, normalizedPath.BufferSize());
	if (error != B_OK)
		RETURN_ERROR(error);

	fPath = strdup(normalizedPathBuffer);
	if (fPath == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	return B_OK;
}
