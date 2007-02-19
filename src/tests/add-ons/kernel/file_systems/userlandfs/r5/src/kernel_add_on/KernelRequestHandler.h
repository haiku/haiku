// KernelRequestHandler.h

#ifndef USERLAND_FS_KERNEL_REQUEST_HANDLER_H
#define USERLAND_FS_KERNEL_REQUEST_HANDLER_H

#include <fsproto.h>

#include "RequestHandler.h"

namespace UserlandFSUtil {

class GetVNodeRequest;
class IsVNodeRemovedRequest;
class NewVNodeRequest;
class NotifyListenerRequest;
class NotifySelectEventRequest;
class PutVNodeRequest;
class RemoveVNodeRequest;
class SendNotificationRequest;
class UnremoveVNodeRequest;

}

using UserlandFSUtil::GetVNodeRequest;
using UserlandFSUtil::IsVNodeRemovedRequest;
using UserlandFSUtil::NewVNodeRequest;
using UserlandFSUtil::NotifyListenerRequest;
using UserlandFSUtil::NotifySelectEventRequest;
using UserlandFSUtil::PutVNodeRequest;
using UserlandFSUtil::RemoveVNodeRequest;
using UserlandFSUtil::SendNotificationRequest;
using UserlandFSUtil::UnremoveVNodeRequest;
using UserlandFSUtil::GetVNodeRequest;

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
			status_t			_HandleRequest(
									SendNotificationRequest* request);
			// vnodes
			status_t			_HandleRequest(GetVNodeRequest* request);
			status_t			_HandleRequest(PutVNodeRequest* request);
			status_t			_HandleRequest(NewVNodeRequest* request);
			status_t			_HandleRequest(RemoveVNodeRequest* request);
			status_t			_HandleRequest(UnremoveVNodeRequest* request);
			status_t			_HandleRequest(IsVNodeRemovedRequest* request);

			status_t			_GetVolume(nspace_id id, Volume** volume);

private:
			FileSystem*			fFileSystem;
			Volume*				fVolume;
			uint32				fExpectedReply;
};

#endif	// USERLAND_FS_KERNEL_REQUEST_HANDLER_H
