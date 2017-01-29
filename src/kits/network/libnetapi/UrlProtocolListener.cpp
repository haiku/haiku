/*
 * Copyright 2010-2017 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 */

#include <iostream>
#include <cstdio>

#include <UrlRequest.h>
#include <UrlProtocolListener.h>

using namespace std;


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
			cout << "   ";
			break;
			
		case B_URL_PROTOCOL_DEBUG_ERROR:
			cout << "!!!";
			break;
			
		case B_URL_PROTOCOL_DEBUG_TRANSFER_IN:
		case B_URL_PROTOCOL_DEBUG_HEADER_IN:
			cout << "<--";
			break;
			
		case B_URL_PROTOCOL_DEBUG_TRANSFER_OUT:
		case B_URL_PROTOCOL_DEBUG_HEADER_OUT:
			cout << "-->";
			break;
	}
	
	cout << " " << caller->Protocol() << ": " << text << endl;
#endif
}
