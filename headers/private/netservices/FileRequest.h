/*
 * Copyright 2013 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_FILE_REQUEST_H_
#define _B_FILE_REQUEST_H_


#include <deque>


#include <UrlRequest.h>
#include <UrlProtocolRoster.h>


#ifndef LIBNETAPI_DEPRECATED
namespace BPrivate {

namespace Network {
#endif

class BFileRequest : public BUrlRequest {
public:
	virtual						~BFileRequest();

	const 	BUrlResult&			Result() const;
			void				SetDisableListener(bool disable);

private:
			friend class BUrlProtocolRoster;

								BFileRequest(const BUrl& url,
									BUrlProtocolListener* listener = NULL,
									BUrlContext* context = NULL);

			status_t			_ProtocolLoop();
private:
			BUrlResult			fResult;
};

#ifndef LIBNETAPI_DEPRECATED
} // namespace Network

} // namespace BPrivate
#endif

#endif // _B_FILE_REQUEST_H_
