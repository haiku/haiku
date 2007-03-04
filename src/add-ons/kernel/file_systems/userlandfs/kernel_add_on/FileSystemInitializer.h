// FileSystemInitializer.h

#ifndef USERLAND_FS_FILE_SYSTEM_INITIALIZER_H
#define USERLAND_FS_FILE_SYSTEM_INITIALIZER_H

#include "LazyInitializable.h"
#include "Referencable.h"


namespace UserlandFSUtil {

class RequestPort;

}

using UserlandFSUtil::RequestPort;

class FileSystem;

class FileSystemInitializer : public LazyInitializable, public Referencable {
public:
								FileSystemInitializer(const char* name,
									RequestPort* initPort);
								~FileSystemInitializer();

	inline	FileSystem*			GetFileSystem()	{ return fFileSystem; }

protected:
	virtual	status_t			FirstTimeInit();

private:
			const char*			fName;		// valid only until FirstTimeInit()
			RequestPort*		fInitPort;
			FileSystem*			fFileSystem;
};

#endif	// USERLAND_FS_FILE_SYSTEM_INITIALIZER_H
