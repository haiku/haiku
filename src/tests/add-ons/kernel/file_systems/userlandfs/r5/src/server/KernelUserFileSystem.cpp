// KernelUserFileSystem.cpp

#include <new>

#include "KernelUserFileSystem.h"
#include "KernelUserVolume.h"

// constructor
KernelUserFileSystem::KernelUserFileSystem(vnode_ops* fsOps)
	: UserFileSystem(),
	  fFSOps(fsOps)
{
}

// destructor
KernelUserFileSystem::~KernelUserFileSystem()
{
}

// CreateVolume
status_t
KernelUserFileSystem::CreateVolume(UserVolume** volume, nspace_id id)
{
	// check initialization and parameters
	if (!fFSOps)
		return B_BAD_VALUE;
	if (!volume)
		return B_BAD_VALUE;
	// create the volume
	*volume = new(nothrow) KernelUserVolume(this, id, fFSOps);
	if (!*volume)
		return B_NO_MEMORY;
	return B_OK;
}

// DeleteVolume
status_t
KernelUserFileSystem::DeleteVolume(UserVolume* volume)
{
	if (!volume || !dynamic_cast<KernelUserVolume*>(volume))
		return B_BAD_VALUE;
	delete volume;
	return B_OK;
}

