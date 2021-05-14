/*
 * Copyright 2010-2017 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_HTTP_RESULT_H_
#define _B_HTTP_RESULT_H_


#include <HttpHeaders.h>
#include <String.h>
#include <Url.h>
#include <UrlResult.h>


#ifndef LIBNETAPI_DEPRECATED
namespace BPrivate {

namespace Network {
#endif

class BUrlRequest;


class BHttpResult: public BUrlResult {
			friend class 				BHttpRequest;

public:
										BHttpResult(const BUrl& url);
#ifdef LIBNETAPI_DEPRECATED
										BHttpResult(BMessage*);
#endif
										BHttpResult(const BHttpResult& other);
										~BHttpResult();

	// Result parameters modifications
			void						SetUrl(const BUrl& url);

	// Result parameters access
			const BUrl&					Url() const;
			BString						ContentType() const;
#ifdef LIBNETAPI_DEPRECATED
			size_t						Length() const;
#else
			off_t						Length() const;
#endif

	// HTTP-Specific stuff
			const BHttpHeaders&			Headers() const;
			const BString&				StatusText() const;
			int32						StatusCode() const;

	// Result tests
			bool						HasHeaders() const;

	// Overloaded members
			BHttpResult&				operator=(const BHttpResult& other);

#ifdef LIBNETAPI_DEPRECATED
	virtual	status_t					Archive(BMessage*, bool) const;
	static	BArchivable*				Instantiate(BMessage*);
#endif
private:
			BUrl						fUrl;

			BHttpHeaders 				fHeaders;
			int32						fStatusCode;
			BString						fStatusString;
};

#ifndef LIBNETAPI_DEPRECATED
} // namespace Network

} // namespace BPrivate
#endif

#endif // _B_URL_RESULT_H_
