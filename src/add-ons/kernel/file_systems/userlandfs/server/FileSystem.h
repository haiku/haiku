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

	virtual	status_t			CreateVolume(Volume** volume, dev_t id) = 0;
	virtual	status_t			DeleteVolume(Volume* volume) = 0;

			void				GetCapabilities(
									FSCapabilities& capabilities) const
									{ capabilities = fCapabilities; }
			client_fs_type		GetClientFSType() const
									{ return fClientFSType; }

protected:
			FSCapabilities		fCapabilities;
			client_fs_type		fClientFSType;
};

}	// namespace UserlandFS

using UserlandFS::FileSystem;

#endif	// USERLAND_FS_FILE_SYSTEM_H
