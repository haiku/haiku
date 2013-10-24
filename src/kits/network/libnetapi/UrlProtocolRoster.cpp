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
#include <FileRequest.h>
#include <HttpRequest.h>
#include <Debug.h>


/* static */ BUrlRequest*
BUrlProtocolRoster::MakeRequest(const BUrl& url,
	BUrlProtocolListener* listener, BUrlContext* context)
{
	// TODO: instanciate the correct BUrlProtocol using add-on interface
	if (url.Protocol() == "http") {
		return new(std::nothrow) BHttpRequest(url, false, "HTTP", listener,
			context);
	} else if (url.Protocol() == "https") {
		return new(std::nothrow) BHttpRequest(url, true, "HTTPS", listener,
			context);
	} else if (url.Protocol() == "file") {
		puts("*** FILE URL");
		return new(std::nothrow) BFileRequest(url, listener, context);
	}

	puts("*** UNKNOWN protocol");

	return NULL;
}
