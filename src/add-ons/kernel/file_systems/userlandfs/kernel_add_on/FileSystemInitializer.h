/*
 * Copyright 2001-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef USERLAND_FS_FILE_SYSTEM_INITIALIZER_H
#define USERLAND_FS_FILE_SYSTEM_INITIALIZER_H

#include <Referenceable.h>

#include "LazyInitializable.h"


namespace UserlandFSUtil {

class RequestPort;

}

using UserlandFSUtil::RequestPort;

class FileSystem;

class FileSystemInitializer : public LazyInitializable, public BReferenceable {
public:
								FileSystemInitializer(const char* name);
								~FileSystemInitializer();

	inline	FileSystem*			GetFileSystem()	{ return fFileSystem; }

protected:
	virtual	status_t			FirstTimeInit();

	virtual	void				LastReferenceReleased();

private:
			status_t			_Init(port_id port);

private:
			const char*			fName;		// valid only until FirstTimeInit()
			FileSystem*			fFileSystem;
};

#endif	// USERLAND_FS_FILE_SYSTEM_INITIALIZER_H
