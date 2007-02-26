// BeOSKernelFileSystem.h

#ifndef USERLAND_FS_BEOS_KERNEL_FILE_SYSTEM_H
#define USERLAND_FS_BEOS_KERNEL_FILE_SYSTEM_H

#include "FileSystem.h"

namespace UserlandFS {

class BeOSKernelFileSystem : public FileSystem {
public:
								BeOSKernelFileSystem(vnode_ops* fsOps);
	virtual						~BeOSKernelFileSystem();

	virtual	status_t			CreateVolume(Volume** volume, nspace_id id);
	virtual	status_t			DeleteVolume(Volume* volume);

private:
			vnode_ops*			fFSOps;
};

}	// namespace UserlandFS

using UserlandFS::BeOSKernelFileSystem;

#endif	// USERLAND_FS_BEOS_KERNEL_FILE_SYSTEM_H
