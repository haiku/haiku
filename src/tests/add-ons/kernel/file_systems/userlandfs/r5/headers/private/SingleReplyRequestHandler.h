// SingleReplyRequestHandler.h

#ifndef USERLAND_FS_SINGLE_REPLY_REQUEST_HANDLER_H
#define USERLAND_FS_SINGLE_REPLY_REQUEST_HANDLER_H

#include "RequestHandler.h"

namespace UserlandFSUtil {

class SingleReplyRequestHandler : public RequestHandler {
public:
								SingleReplyRequestHandler();
								SingleReplyRequestHandler(uint32 expectedReply);

	virtual	status_t			HandleRequest(Request* request);

private:
			bool				fAcceptAnyRequest;
			uint32				fExpectedReply;
};

}	// namespace UserlandFSUtil

using UserlandFSUtil::SingleReplyRequestHandler;

#endif	// USERLAND_FS_SINGLE_REPLY_REQUEST_HANDLER_H
