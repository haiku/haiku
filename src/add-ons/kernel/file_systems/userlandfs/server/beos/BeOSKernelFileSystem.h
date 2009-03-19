// BeOSKernelFileSystem.h

#ifndef USERLAND_FS_BEOS_KERNEL_FILE_SYSTEM_H
#define USERLAND_FS_BEOS_KERNEL_FILE_SYSTEM_H

#include "../FileSystem.h"

struct beos_vnode_ops;

namespace UserlandFS {

class BeOSKernelFileSystem : public FileSystem {
public:
								BeOSKernelFileSystem(const char* fsName,
									beos_vnode_ops* fsOps);
	virtual						~BeOSKernelFileSystem();

			status_t			Init();

	virtual	status_t			CreateVolume(Volume** volume, dev_t id);
	virtual	status_t			DeleteVolume(Volume* volume);

			void				GetVolumeCapabilities(
									FSVolumeCapabilities& capabilities) const
									{ capabilities = fVolumeCapabilities; }
			void				GetNodeCapabilities(
									FSVNodeCapabilities& capabilities) const
									{ capabilities = fNodeCapabilities; }

private:
			void				_InitCapabilities();

private:
			beos_vnode_ops*		fFSOps;
			FSVolumeCapabilities fVolumeCapabilities;
			FSVNodeCapabilities	fNodeCapabilities;
			bool				fBlockCacheInitialized;
};

}	// namespace UserlandFS

using UserlandFS::BeOSKernelFileSystem;

#endif	// USERLAND_FS_BEOS_KERNEL_FILE_SYSTEM_H
