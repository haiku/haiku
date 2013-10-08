/*
 * Copyright 2010-2013 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 */

#include <UrlProtocolRoster.h>

#include <new>

#include <UrlRequest.h>
#include <HttpRequest.h>
#include <Debug.h>


/* static */ BUrlRequest*
BUrlProtocolRoster::MakeRequest(const BUrl& url,
	BUrlProtocolListener* listener, BUrlContext* context)
{
	BUrlResult* result = new BUrlResult(url);
		// FIXME this is leaked
	// TODO: instanciate the correct BUrlProtocol using add-on interface
	if (url.Protocol() == "http") {
		return new(std::nothrow) BHttpRequest(url, *result, false, "HTTP", listener,
			context);
	} else if (url.Protocol() == "https") {
		return new(std::nothrow) BHttpRequest(url, *result, true, "HTTPS", listener,
			context);
	}

	return NULL;
}
