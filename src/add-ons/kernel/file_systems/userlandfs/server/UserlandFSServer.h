// UserlandFSServer.h

#ifndef USERLAND_FS_SERVER_H
#define USERLAND_FS_SERVER_H

#include <Application.h>

namespace UserlandFSUtil {
	class RequestPort;
}
using UserlandFSUtil::RequestPort;

namespace UserlandFS {

class FileSystem;
class RequestThread;

class UserlandFSServer : public BApplication {
public:
								UserlandFSServer(const char* signature);
	virtual						~UserlandFSServer();

			status_t			Init(const char* fileSystem);

	static	RequestPort*		GetNotificationRequestPort();
	static	FileSystem*			GetFileSystem();

private:
			status_t			_RegisterWithDispatcher(const char* fsName);
private:
			image_id			fAddOnImage;
			FileSystem*			fFileSystem;
			RequestPort*		fNotificationRequestPort;
			RequestThread*		fRequestThreads;
			bool				fBlockCacheInitialized;
};

}	// namespace UserlandFS

using UserlandFS::FileSystem;
using UserlandFS::RequestThread;
using UserlandFS::UserlandFSServer;

#endif	// USERLAND_FS_SERVER_H
