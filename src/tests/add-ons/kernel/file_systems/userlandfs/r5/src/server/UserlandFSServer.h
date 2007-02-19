// UserlandFSServer.h

#ifndef USERLAND_FS_SERVER_H
#define USERLAND_FS_SERVER_H

#include <Application.h>

namespace UserlandFS {

class RequestThread;
class UserFileSystem;

class UserlandFSServer : public BApplication {
public:
								UserlandFSServer(const char* signature);
	virtual						~UserlandFSServer();

			status_t			Init(const char* fileSystem);

	static	RequestPort*		GetNotificationRequestPort();
	static	UserFileSystem*		GetFileSystem();

private:
			status_t			_RegisterWithDispatcher(const char* fsName);
private:
			image_id			fAddOnImage;
			UserFileSystem*		fFileSystem;
			RequestPort*		fNotificationRequestPort;
			RequestThread*		fRequestThreads;
			bool				fBlockCacheInitialized;
};

}	// namespace UserlandFS

using UserlandFS::RequestThread;
using UserlandFS::UserFileSystem;
using UserlandFS::UserlandFSServer;

#endif	// USERLAND_FS_SERVER_H
