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

#include <DataRequest.h>
#include <Debug.h>
#include <FileRequest.h>
#include <GopherRequest.h>
#include <HttpRequest.h>
#include <UrlRequest.h>


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
		return new(std::nothrow) BFileRequest(url, listener, context);
	} else if (url.Protocol() == "data") {
		return new(std::nothrow) BDataRequest(url, listener, context);
	} else if (url.Protocol() == "gopher") {
		return new(std::nothrow) BGopherRequest(url, listener, context);
	}

	return NULL;
}
