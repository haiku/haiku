// UserlandFS.h

#ifndef USERLAND_FS_H
#define USERLAND_FS_H

#include <SupportDefs.h>

#include "HashMap.h"
#include "LazyInitializable.h"
#include "String.h"

namespace UserlandFSUtil {

class RequestPort;

}

using UserlandFSUtil::RequestPort;

class FileSystem;

class UserlandFS : public LazyInitializable {
private:
								UserlandFS();
								~UserlandFS();

public:
	static	status_t			RegisterUserlandFS(UserlandFS** userlandFS);
	static	void				UnregisterUserlandFS();
	static	UserlandFS*			GetUserlandFS();

			status_t			RegisterFileSystem(const char* name,
									FileSystem** fileSystem);
			status_t			UnregisterFileSystem(FileSystem* fileSystem);

			int32				CountFileSystems() const;

protected:
	virtual	status_t			FirstTimeInit();

private:
			friend class KernelDebug;
			typedef SynchronizedHashMap<String, FileSystem*> FileSystemMap;

	static	UserlandFS*	volatile sUserlandFS;
	static	spinlock			sUserlandFSLock;
	static	vint32				sMountedFileSystems;		

			RequestPort*		fPort;
			FileSystemMap*		fFileSystems;
			bool				fDebuggerCommandsAdded;
};

#endif	// USERLAND_FS_H
