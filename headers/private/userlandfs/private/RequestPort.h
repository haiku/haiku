// RequestPort.h

#ifndef USERLAND_FS_REQUEST_PORT_H
#define USERLAND_FS_REQUEST_PORT_H

#include "Port.h"
#include "RequestAllocator.h"

namespace UserlandFSUtil {

class RequestHandler;

// RequestPort
class RequestPort {
public:
								RequestPort(int32 size);
								RequestPort(const Port::Info* info);
								~RequestPort();

			void				Close();

			status_t			InitCheck() const;

			Port*				GetPort();
			const Port::Info*	GetPortInfo() const;

			status_t			SendRequest(RequestAllocator* allocator);
			status_t			SendRequest(RequestAllocator* allocator,
									RequestHandler* handler,
									Request** reply = NULL,
									bigtime_t timeout = -1);
			status_t			ReceiveRequest(Request** request,
									bigtime_t timeout = -1);
			status_t			HandleRequests(RequestHandler* handler,
									Request** reply = NULL,
									bigtime_t timeout = -1);

			void				ReleaseRequest(Request* request);

private:
			void				_PopAllocator();

private:
			friend class ::KernelDebug;
			struct AllocatorNode;

			Port				fPort;
			AllocatorNode*		fCurrentAllocatorNode;
};

// RequestReleaser
class RequestReleaser {
public:
	inline RequestReleaser(RequestPort* port, Request* request)
		: fPort(port), fRequest(request) {}

	inline ~RequestReleaser()
	{
		if (fPort && fRequest)
			fPort->ReleaseRequest(fRequest);
	}

private:
	RequestPort*	fPort;
	Request*		fRequest;
};

}	// namespace UserlandFSUtil

using UserlandFSUtil::RequestPort;
using UserlandFSUtil::RequestReleaser;

#endif	// USERLAND_FS_REQUEST_PORT_H
