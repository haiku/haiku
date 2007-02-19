// UserFileSystem.h

#ifndef USERLAND_FS_USER_FILE_SYSTEM_H
#define USERLAND_FS_USER_FILE_SYSTEM_H

#include <fsproto.h>
#include <SupportDefs.h>

namespace UserlandFS {

class UserVolume;

class UserFileSystem {
public:
								UserFileSystem();
	virtual						~UserFileSystem();

	virtual	status_t			CreateVolume(UserVolume** volume,
									nspace_id id) = 0;
	virtual	status_t			DeleteVolume(UserVolume* volume) = 0;
};

}	// namespace UserlandFS

using UserlandFS::UserFileSystem;

#endif	// USERLAND_FS_FILE_SYSTEM_H
