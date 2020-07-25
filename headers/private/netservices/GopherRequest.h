/*
 * Copyright 2014 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_GOPHER_REQUEST_H_
#define _B_GOPHER_REQUEST_H_


#include <deque>

#include <NetworkRequest.h>
#include <UrlProtocolRoster.h>


#ifndef LIBNETAPI_DEPRECATED
namespace BPrivate {

namespace Network {
#endif

class BGopherRequest : public BNetworkRequest {
public:
	virtual						~BGopherRequest();

			status_t			Stop();
	const 	BUrlResult&			Result() const;
            void                SetDisableListener(bool disable);

private:
			friend class BUrlProtocolRoster;

#ifdef LIBNETAPI_DEPRECATED
								BGopherRequest(const BUrl& url,
									BUrlProtocolListener* listener = NULL,
									BUrlContext* context = NULL);
#else
								BGopherRequest(const BUrl& url,
									BDataIO* output,
									BUrlProtocolListener* listener = NULL,
									BUrlContext* context = NULL);
#endif

			status_t			_ProtocolLoop();
			void				_SendRequest();

			bool				_NeedsParsing();
			bool				_NeedsLastDotStrip();
#ifdef LIBNETAPI_DEPRECATED
			void				_ParseInput(bool last);
#else
			status_t			_ParseInput(bool last);
#endif

			BString&			_HTMLEscapeString(BString &str);

private:
			char				fItemType;
			BString				fPath;

			ssize_t				fPosition;

			BUrlResult			fResult;
};

#ifndef LIBNETAPI_DEPRECATED
} // namespace Network

} // namespace BPrivate
#endif

#endif // _B_GOPHER_REQUEST_H_
