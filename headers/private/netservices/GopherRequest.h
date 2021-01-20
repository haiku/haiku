/*
 * Copyright 2014 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_GOPHER_REQUEST_H_
#define _B_GOPHER_REQUEST_H_


#include <deque>

#include <NetworkRequest.h>
#include <UrlProtocolRoster.h>


class BGopherRequest : public BNetworkRequest {
public:
	virtual						~BGopherRequest();

			status_t			Stop();
	const 	BUrlResult&			Result() const;
            void                SetDisableListener(bool disable);

private:
			friend class BUrlProtocolRoster;

								BGopherRequest(const BUrl& url,
									BUrlProtocolListener* listener = NULL,
									BUrlContext* context = NULL);

			status_t			_ProtocolLoop();
			void				_SendRequest();

			bool				_NeedsParsing();
			bool				_NeedsLastDotStrip();
			void				_ParseInput(bool last);

			BString&			_HTMLEscapeString(BString &str);

private:
			char				fItemType;
			BString				fPath;

			ssize_t				fPosition;

			BUrlResult			fResult;
};


#endif // _B_GOPHER_REQUEST_H_
