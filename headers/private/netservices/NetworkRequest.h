/*
 * Copyright 2014 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_NET_REQUEST_H_
#define _B_NET_REQUEST_H_


#include <NetBuffer.h>
#include <NetworkAddress.h>
#include <UrlRequest.h>


class BAbstractSocket;


#ifndef LIBNETAPI_DEPRECATED
namespace BPrivate {

namespace Network {
#endif

class BNetworkRequest: public BUrlRequest
{
public:
#ifdef LIBNETAPI_DEPRECATED
								BNetworkRequest(const BUrl& url,
									BUrlProtocolListener* listener,
									BUrlContext* context,
									const char* threadName,
									const char* protocolName);
#else
								BNetworkRequest(const BUrl& url,
									BDataIO* output,
									BUrlProtocolListener* listener,
									BUrlContext* context,
									const char* threadName,
									const char* protocolName);
#endif

	virtual	status_t			Stop();
	virtual void				SetTimeout(bigtime_t timeout);

protected:
			bool 				_ResolveHostName(BString host, uint16_t port);

			void				_ProtocolSetup();
			status_t			_GetLine(BString& destString);

protected:
			BAbstractSocket*	fSocket;
			BNetworkAddress		fRemoteAddr;

			BNetBuffer			fInputBuffer;
};

#ifndef LIBNETAPI_DEPRECATED
} // namespace Network

} // namespace BPrivate
#endif

#endif
