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


class BNetworkRequest: public BUrlRequest
{
public:
								BNetworkRequest(const BUrl& url,
									BUrlProtocolListener* listener,
									BUrlContext* context,
									const char* threadName,
									const char* protocolName);
protected:
			bool 				_ResolveHostName(uint16_t port);

			status_t			_GetLine(BString& destString);

protected:
			BAbstractSocket*	fSocket;
			BNetworkAddress		fRemoteAddr;

			BNetBuffer			fInputBuffer;
};


#endif
