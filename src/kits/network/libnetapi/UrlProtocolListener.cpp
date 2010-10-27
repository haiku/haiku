/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 */

#include <iostream>
#include <cstdio>

#include <UrlProtocol.h>
#include <UrlProtocolListener.h>

using namespace std;


void
BUrlProtocolListener::ConnectionOpened(BUrlProtocol*)
{
}


void
BUrlProtocolListener::HostnameResolved(BUrlProtocol*, const char*)
{
}


void
BUrlProtocolListener::ResponseStarted(BUrlProtocol*)
{
}


void
BUrlProtocolListener::HeadersReceived(BUrlProtocol*)
{
}


void
BUrlProtocolListener::DataReceived(BUrlProtocol*, const char*, ssize_t)
{
}


void
BUrlProtocolListener::DownloadProgress(BUrlProtocol*, ssize_t, ssize_t)
{
}


void
BUrlProtocolListener::UploadProgress(BUrlProtocol*, ssize_t, ssize_t)
{
}


void
BUrlProtocolListener::RequestCompleted(BUrlProtocol*, bool)
{
}


void
BUrlProtocolListener::DebugMessage(BUrlProtocol* caller,
	BUrlProtocolDebugMessage type, const char* text)
{
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
}
