// RequestFactory.h

#ifndef NET_FS_REQUEST_FACTORY_H
#define NET_FS_REQUEST_FACTORY_H

#include <SupportDefs.h>

class Request;

class RequestFactory {
private:
								RequestFactory();
								~RequestFactory();
public:
	static	status_t			CreateRequest(uint32 type, Request** request);
};

#endif	// NET_FS_REQUEST_FACTORY_H
