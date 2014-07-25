/*
 * Copyright 2014 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_GOPHER_REQUEST_H_
#define _B_GOPHER_REQUEST_H_


#include <deque>

#include <NetBuffer.h>
#include <NetworkAddress.h>
#include <UrlRequest.h>


class BAbstractSocket;


class BGopherRequest : public BUrlRequest {
public:
								BGopherRequest(const BUrl& url,
									BUrlProtocolListener* listener = NULL,
									BUrlContext* context = NULL);
	virtual						~BGopherRequest();

			status_t			Stop();
	const 	BUrlResult&			Result() const;
            void                SetDisableListener(bool disable);

private:
			status_t			_ProtocolLoop();
			bool 				_ResolveHostName();
			void				_SendRequest();

			bool				_NeedsParsing();
			bool				_NeedsLastDotStrip();
			void				_ParseInput(bool last);

			status_t			_GetLine(BString& destString);
			BString&			_HTMLEscapeString(BString &str);

private:
			char				fItemType;
			BString				fPath;
			BAbstractSocket*	fSocket;
			BNetworkAddress		fRemoteAddr;

			BNetBuffer			fInputBuffer;
			ssize_t				fPosition;

			BUrlResult			fResult;
};


#endif // _B_GOPHER_REQUEST_H_
