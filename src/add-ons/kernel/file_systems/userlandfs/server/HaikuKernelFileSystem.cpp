// HaikuKernelFileSystem.cpp

#include "HaikuKernelFileSystem.h"

#include <new>

#include <fs_interface.h>

#include "HaikuKernelVolume.h"

using std::nothrow;

// constructor
HaikuKernelFileSystem::HaikuKernelFileSystem(file_system_module_info* fsModule)
	: FileSystem(),
	  fFSModule(fsModule)
{
	_InitCapabilities();
}

// destructor
HaikuKernelFileSystem::~HaikuKernelFileSystem()
{
	// call the kernel module uninitialization
	if (fFSModule->info.std_ops)
		fFSModule->info.std_ops(B_MODULE_UNINIT);
}

// Init
status_t
HaikuKernelFileSystem::Init()
{
	// call the kernel module initialization
	if (!fFSModule->info.std_ops)
		return B_OK;
	return fFSModule->info.std_ops(B_MODULE_INIT);
}

// CreateVolume
status_t
HaikuKernelFileSystem::CreateVolume(Volume** volume, dev_t id)
{
	// check initialization and parameters
	if (!fFSModule || !volume)
		return B_BAD_VALUE;

	// create the volume
	*volume = new(nothrow) HaikuKernelVolume(this, id, fFSModule);
	if (!*volume)
		return B_NO_MEMORY;
	return B_OK;
}

// DeleteVolume
status_t
HaikuKernelFileSystem::DeleteVolume(Volume* volume)
{
	if (!volume || !dynamic_cast<HaikuKernelVolume*>(volume))
		return B_BAD_VALUE;
	delete volume;
	return B_OK;
}

// _InitCapabilities
void
HaikuKernelFileSystem::_InitCapabilities()
{
	fCapabilities.ClearAll();

	// FS interface type
	fClientFSType = CLIENT_FS_HAIKU_KERNEL;

	// FS operations
	fCapabilities.Set(FS_CAPABILITY_MOUNT, fFSModule->mount);
}
