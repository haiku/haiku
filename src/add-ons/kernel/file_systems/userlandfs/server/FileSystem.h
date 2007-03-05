// FileSystem.h

#ifndef USERLAND_FS_FILE_SYSTEM_H
#define USERLAND_FS_FILE_SYSTEM_H

#include <fs_interface.h>
#include <SupportDefs.h>

#include "FSCapabilities.h"

namespace UserlandFS {

class Volume;

class FileSystem {
public:
								FileSystem();
	virtual						~FileSystem();

	virtual	status_t			CreateVolume(Volume** volume, mount_id id) = 0;
	virtual	status_t			DeleteVolume(Volume* volume) = 0;

			void				GetCapabilities(
									FSCapabilities& capabilities) const
									{ capabilities = fCapabilities; }

protected:
			FSCapabilities		fCapabilities;
};

}	// namespace UserlandFS

using UserlandFS::FileSystem;

#endif	// USERLAND_FS_FILE_SYSTEM_H
