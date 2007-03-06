// HaikuKernelFileSystem.h

#ifndef USERLAND_FS_HAIKU_KERNEL_FILE_SYSTEM_H
#define USERLAND_FS_HAIKU_KERNEL_FILE_SYSTEM_H

#include "FileSystem.h"

struct file_system_module_info;

namespace UserlandFS {

class HaikuKernelFileSystem : public FileSystem {
public:
								HaikuKernelFileSystem(
									file_system_module_info* fsModule);
	virtual						~HaikuKernelFileSystem();

	virtual	status_t			CreateVolume(Volume** volume, mount_id id);
	virtual	status_t			DeleteVolume(Volume* volume);

private:
			void				_InitCapabilities();

private:
			file_system_module_info* fFSModule;
};

}	// namespace UserlandFS

using UserlandFS::HaikuKernelFileSystem;

#endif	// USERLAND_FS_HAIKU_KERNEL_FILE_SYSTEM_H
