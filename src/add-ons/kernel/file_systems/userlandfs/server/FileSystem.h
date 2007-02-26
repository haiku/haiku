// FileSystem.h

#ifndef USERLAND_FS_FILE_SYSTEM_H
#define USERLAND_FS_FILE_SYSTEM_H

#include <fsproto.h>
#include <SupportDefs.h>

namespace UserlandFS {

class Volume;

class FileSystem {
public:
								FileSystem();
	virtual						~FileSystem();

	virtual	status_t			CreateVolume(Volume** volume,
									nspace_id id) = 0;
	virtual	status_t			DeleteVolume(Volume* volume) = 0;
};

}	// namespace UserlandFS

using UserlandFS::FileSystem;

#endif	// USERLAND_FS_FILE_SYSTEM_H
