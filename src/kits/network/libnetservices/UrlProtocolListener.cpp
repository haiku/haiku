/*
 * Copyright 2010-2017 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 */

#include <cstdio>

#include <UrlRequest.h>
#include <UrlProtocolListener.h>

using namespace std;

#ifndef LIBNETAPI_DEPRECATED
using namespace BPrivate::Network;
#endif

void
BUrlProtocolListener::ConnectionOpened(BUrlRequest*)
{
}


void
BUrlProtocolListener::HostnameResolved(BUrlRequest*, const char*)
{
}


bool
BUrlProtocolListener::CertificateVerificationFailed(BUrlRequest* caller,
	BCertificate& certificate, const char* message)
{
	return false;
}


void
BUrlProtocolListener::ResponseStarted(BUrlRequest*)
{
}


#ifdef LIBNETAPI_DEPRECATED
void
BUrlProtocolListener::HeadersReceived(BUrlRequest*, const BUrlResult& result)
{
}


void
BUrlProtocolListener::DataReceived(BUrlRequest*, const char*, off_t, ssize_t)
{
}


void
BUrlProtocolListener::DownloadProgress(BUrlRequest*, ssize_t, ssize_t)
{
}


void
BUrlProtocolListener::UploadProgress(BUrlRequest*, ssize_t, ssize_t)
{
}

#else

void
BUrlProtocolListener::HeadersReceived(BUrlRequest*)
{
}


void
BUrlProtocolListener::BytesWritten(BUrlRequest*, size_t)
{
}


void
BUrlProtocolListener::DownloadProgress(BUrlRequest*, off_t, off_t)
{
}


void
BUrlProtocolListener::UploadProgress(BUrlRequest*, off_t, off_t)
{
}
#endif // LIBNETAPI_DEPRECATED


void
BUrlProtocolListener::RequestCompleted(BUrlRequest*, bool)
{
}


void
BUrlProtocolListener::DebugMessage(BUrlRequest* caller,
	BUrlProtocolDebugMessage type, const char* text)
{
#ifdef DEBUG
	switch (type) {
		case B_URL_PROTOCOL_DEBUG_TEXT:
			fprintf(stderr, "   ");
			break;

		case B_URL_PROTOCOL_DEBUG_ERROR:
			fprintf(stderr, "!!!");
			break;

		case B_URL_PROTOCOL_DEBUG_TRANSFER_IN:
		case B_URL_PROTOCOL_DEBUG_HEADER_IN:
			fprintf(stderr, "<--");
			break;

		case B_URL_PROTOCOL_DEBUG_TRANSFER_OUT:
		case B_URL_PROTOCOL_DEBUG_HEADER_OUT:
			fprintf(stderr, "-->");
			break;
	}

	fprintf(stderr, " %s: %s\n", caller->Protocol().String(), text);
#endif
}
