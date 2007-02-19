// RequestHandler.h

#ifndef NET_FS_REQUEST_HANDLER_H
#define NET_FS_REQUEST_HANDLER_H

#include "Requests.h"
#include "ThreadLocal.h"

class RequestChannel;

class RequestHandler : protected RequestVisitor {
public:
								RequestHandler();
	virtual						~RequestHandler();

	virtual	status_t			HandleRequest(Request* request,
									RequestChannel* channel);

protected:
	virtual	status_t			VisitAny(Request* request);

			RequestChannel*		GetChannel() const;

private:
			ThreadLocal			fChannels;
};

#endif	// NET_FS_REQUEST_HANDLER_H
