// UserlandFSDispatcher.h

#ifndef USERLAND_FS_DISPATCHER_H
#define USERLAND_FS_DISPATCHER_H

#include <Application.h>
#include <Locker.h>
#include <OS.h>

#include "HashMap.h"

namespace UserlandFSUtil {

class RequestPort;
class String;

}

using UserlandFSUtil::RequestPort;
using UserlandFSUtil::String;

namespace UserlandFS {

class FileSystem;

class UserlandFSDispatcher : public BApplication {
public:
								UserlandFSDispatcher(const char* signature);
	virtual						~UserlandFSDispatcher();

			status_t			Init();

	virtual	void				MessageReceived(BMessage* message);

private:
			status_t			_GetFileSystem(const char* name,
									FileSystem** fileSystem);
			status_t			_GetFileSystemNoInit(const char* name,
									FileSystem** fileSystem);
			FileSystem*			_GetFileSystemNoInit(team_id team);
			status_t			_PutFileSystem(FileSystem* fileSystem);

			bool				_WaitForConnection();
			status_t			_ProcessRequests();

	static	int32				_RequestProcessorEntry(void* data);
			int32				_RequestProcessor();

private:
			typedef SynchronizedHashMap<String, FileSystem*> FileSystemMap;

			bool				fTerminating;
			thread_id			fRequestProcessor;
			port_id				fConnectionPort;
			port_id				fConnectionReplyPort;
			BLocker				fRequestLock;
			RequestPort*		fRequestPort;
			FileSystemMap		fFileSystems;
};

}	// namespace UserlandFS

using UserlandFS::UserlandFSDispatcher;

#endif	// USERLAND_FS_DISPATCHER_H
