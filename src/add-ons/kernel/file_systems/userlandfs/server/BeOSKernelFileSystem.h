// BeOSKernelFileSystem.h

#ifndef USERLAND_FS_BEOS_KERNEL_FILE_SYSTEM_H
#define USERLAND_FS_BEOS_KERNEL_FILE_SYSTEM_H

#include "FileSystem.h"

struct beos_vnode_ops;

namespace UserlandFS {

class BeOSKernelFileSystem : public FileSystem {
public:
								BeOSKernelFileSystem(beos_vnode_ops* fsOps);
	virtual						~BeOSKernelFileSystem();

	virtual	status_t			CreateVolume(Volume** volume, mount_id id);
	virtual	status_t			DeleteVolume(Volume* volume);

private:
			void				_InitCapabilities(beos_vnode_ops* fsOps);

private:
			beos_vnode_ops*		fFSOps;
};

}	// namespace UserlandFS

using UserlandFS::BeOSKernelFileSystem;

#endif	// USERLAND_FS_BEOS_KERNEL_FILE_SYSTEM_H
