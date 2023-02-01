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


namespace BPrivate {

namespace Network {


class BNetworkRequest: public BUrlRequest
{
public:
								BNetworkRequest(const BUrl& url,
									BDataIO* output,
									BUrlProtocolListener* listener,
									BUrlContext* context,
									const char* threadName,
									const char* protocolName);

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


} // namespace Network

} // namespace BPrivate

#endif
