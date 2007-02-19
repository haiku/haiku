// KernelUserFileSystem.h

#ifndef USERLAND_FS_KERNEL_USER_FILE_SYSTEM_H
#define USERLAND_FS_KERNEL_USER_FILE_SYSTEM_H

#include "UserFileSystem.h"

namespace UserlandFS {

class KernelUserFileSystem : public UserFileSystem {
public:
								KernelUserFileSystem(vnode_ops* fsOps);
	virtual						~KernelUserFileSystem();

	virtual	status_t			CreateVolume(UserVolume** volume, nspace_id id);
	virtual	status_t			DeleteVolume(UserVolume* volume);

private:
			vnode_ops*			fFSOps;
};

}	// namespace UserlandFS

using UserlandFS::KernelUserFileSystem;

#endif	// USERLAND_FS_KERNEL_USER_FILE_SYSTEM_H
