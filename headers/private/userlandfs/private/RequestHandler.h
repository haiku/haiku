// RequestHandler.h

#ifndef USERLAND_FS_REQUEST_HANDLER_H
#define USERLAND_FS_REQUEST_HANDLER_H

#include <SupportDefs.h>

namespace UserlandFSUtil {

class Request;
class RequestPort;

class RequestHandler {
public:
								RequestHandler();
	virtual						~RequestHandler();

			void				SetPort(RequestPort* port);

			bool				IsDone() const;

	virtual	status_t			HandleRequest(Request* request) = 0;

protected:
			RequestPort*		fPort;
			bool				fDone;
};

}	// namespace UserlandFSUtil

using UserlandFSUtil::Request;
using UserlandFSUtil::RequestHandler;
using UserlandFSUtil::RequestPort;

#endif	// USERLAND_FS_REQUEST_HANDLER_H
