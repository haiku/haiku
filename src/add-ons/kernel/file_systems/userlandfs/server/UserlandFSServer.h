/*
 * Copyright 2001-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef USERLAND_FS_SERVER_H
#define USERLAND_FS_SERVER_H

#include <Application.h>
#include <image.h>

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

			status_t			Init(const char* fileSystem, port_id port);

	static	RequestPort*		GetNotificationRequestPort();
	static	FileSystem*			GetFileSystem();

private:
			status_t			_Announce(const char* fsName, port_id port);

private:
			image_id			fAddOnImage;
			FileSystem*			fFileSystem;
			RequestPort*		fNotificationRequestPort;
			RequestThread*		fRequestThreads;
};

}	// namespace UserlandFS

using UserlandFS::FileSystem;
using UserlandFS::RequestThread;
using UserlandFS::UserlandFSServer;

#endif	// USERLAND_FS_SERVER_H
