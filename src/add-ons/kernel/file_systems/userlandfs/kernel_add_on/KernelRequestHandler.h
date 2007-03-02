// KernelRequestHandler.h

#ifndef USERLAND_FS_KERNEL_REQUEST_HANDLER_H
#define USERLAND_FS_KERNEL_REQUEST_HANDLER_H

#include <fs_interface.h>

#include "RequestHandler.h"

namespace UserlandFSUtil {

class GetVNodeRemovedRequest;
class GetVNodeRequest;
class NewVNodeRequest;
class NotifyListenerRequest;
class NotifyQueryRequest;
class NotifySelectEventRequest;
class PublishVNodeRequest;
class PutVNodeRequest;
class RemoveVNodeRequest;
class UnremoveVNodeRequest;

}

using UserlandFSUtil::GetVNodeRemovedRequest;
using UserlandFSUtil::GetVNodeRequest;
using UserlandFSUtil::NewVNodeRequest;
using UserlandFSUtil::NotifyListenerRequest;
using UserlandFSUtil::NotifyQueryRequest;
using UserlandFSUtil::NotifySelectEventRequest;
using UserlandFSUtil::PublishVNodeRequest;
using UserlandFSUtil::PutVNodeRequest;
using UserlandFSUtil::RemoveVNodeRequest;
using UserlandFSUtil::UnremoveVNodeRequest;

class Volume;

class KernelRequestHandler : public RequestHandler {
public:
								KernelRequestHandler(Volume* volume,
									uint32 expectedReply);
								KernelRequestHandler(FileSystem* fileSystem,
									uint32 expectedReply);
	virtual						~KernelRequestHandler();

	virtual	status_t			HandleRequest(Request* request);

private:
			// notifications
			status_t			_HandleRequest(NotifyListenerRequest* request);
			status_t			_HandleRequest(
									NotifySelectEventRequest* request);
			status_t			_HandleRequest(NotifyQueryRequest* request);
			// vnodes
			status_t			_HandleRequest(GetVNodeRequest* request);
			status_t			_HandleRequest(PutVNodeRequest* request);
			status_t			_HandleRequest(NewVNodeRequest* request);
			status_t			_HandleRequest(PublishVNodeRequest* request);
			status_t			_HandleRequest(RemoveVNodeRequest* request);
			status_t			_HandleRequest(UnremoveVNodeRequest* request);
			status_t			_HandleRequest(GetVNodeRemovedRequest* request);

			status_t			_GetVolume(mount_id id, Volume** volume);

private:
			FileSystem*			fFileSystem;
			Volume*				fVolume;
			uint32				fExpectedReply;
};

#endif	// USERLAND_FS_KERNEL_REQUEST_HANDLER_H
