// BeOSKernelFileSystem.cpp

#include <new>

#include "BeOSKernelFileSystem.h"
#include "BeOSKernelVolume.h"

// constructor
BeOSKernelFileSystem::BeOSKernelFileSystem(vnode_ops* fsOps)
	: FileSystem(),
	  fFSOps(fsOps)
{
}

// destructor
BeOSKernelFileSystem::~BeOSKernelFileSystem()
{
}

// CreateVolume
status_t
BeOSKernelFileSystem::CreateVolume(Volume** volume, mount_id id)
{
	// check initialization and parameters
	if (!fFSOps || !volume)
		return B_BAD_VALUE;

	// create the volume
	*volume = new(nothrow) BeOSKernelVolume(this, id, fFSOps);
	if (!*volume)
		return B_NO_MEMORY;
	return B_OK;
}

// DeleteVolume
status_t
BeOSKernelFileSystem::DeleteVolume(Volume* volume)
{
	if (!volume || !dynamic_cast<BeOSKernelVolume*>(volume))
		return B_BAD_VALUE;
	delete volume;
	return B_OK;
}

