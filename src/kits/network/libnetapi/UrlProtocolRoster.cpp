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
#include <HttpRequest.h>
#include <UrlRequest.h>


static BReference<BUrlContext> gDefaultContext = new(std::nothrow) BUrlContext();


/* static */ BUrlRequest*
BUrlProtocolRoster::MakeRequest(const BUrl& url,
	BUrlProtocolListener* listener, BUrlContext* context)
{
	if (context == NULL)
		context = gDefaultContext;

	if (context == NULL) {
		// Allocation of the gDefaultContext failed. Don't allow creating
		// requests without a context.
		return NULL;
	}

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
	}

	return NULL;
}
