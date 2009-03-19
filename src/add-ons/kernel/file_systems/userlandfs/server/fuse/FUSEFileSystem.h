/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef USERLAND_FS_FUSE_FILE_SYSTEM_H
#define USERLAND_FS_FUSE_FILE_SYSTEM_H

#include "../FileSystem.h"


namespace UserlandFS {

class FUSEFileSystem : public FileSystem {
public:
								FUSEFileSystem(
									int (*mainFunction)(int,
										const char* const*));
	virtual						~FUSEFileSystem();

	virtual	status_t			CreateVolume(Volume** _volume, dev_t id);
	virtual	status_t			DeleteVolume(Volume* volume);

			status_t			InitClientFS(const char* parameters);

private:
			class ArgumentVector;

private:
			int					(*fMainFunction)(int, const char* const*);
};

}	// namespace UserlandFS

using UserlandFS::FUSEFileSystem;


#endif	// USERLAND_FS_FUSE_FILE_SYSTEM_H
