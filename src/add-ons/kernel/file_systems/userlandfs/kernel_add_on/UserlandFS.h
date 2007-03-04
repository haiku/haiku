// UserlandFS.h

#ifndef USERLAND_FS_H
#define USERLAND_FS_H

#include <SupportDefs.h>

#include "AutoLocker.h"
#include "HashMap.h"
#include "String.h"

namespace UserlandFSUtil {

class RequestPort;

}

using UserlandFSUtil::RequestPort;

class FileSystem;

class UserlandFS {
private:
								UserlandFS();
								~UserlandFS();

public:
	static	status_t			InitUserlandFS(UserlandFS** userlandFS);
	static	void				UninitUserlandFS();
	static	UserlandFS*			GetUserlandFS();

			status_t			RegisterFileSystem(const char* name,
									FileSystem** fileSystem);
			status_t			UnregisterFileSystem(FileSystem* fileSystem);

			int32				CountFileSystems() const;

private:
			status_t			_Init();

private:
			friend class KernelDebug;
			typedef SynchronizedHashMap<String, FileSystem*> FileSystemMap;
			typedef AutoLocker<UserlandFS::FileSystemMap> FileSystemLocker;

	static	UserlandFS*			sUserlandFS;

			RequestPort*		fPort;
			FileSystemMap*		fFileSystems;
			bool				fDebuggerCommandsAdded;
};

#endif	// USERLAND_FS_H
