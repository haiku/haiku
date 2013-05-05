/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 */

#include <cstdio>

#include <UrlSynchronousRequest.h>

#define PRINT(x) printf x;


BUrlSynchronousRequest::BUrlSynchronousRequest(BUrl& url)
	:
	BUrlRequest(url, this),
	fRequestComplete(false)
{
}


status_t
BUrlSynchronousRequest::Perform()
{
	SetProtocolListener(this);
	fRequestComplete = false;

	return Start();
}


status_t
BUrlSynchronousRequest::WaitUntilCompletion()
{
	while (!fRequestComplete)
		snooze(10000);

	return B_OK;
}


void
BUrlSynchronousRequest::ConnectionOpened(BUrlProtocol*)
{
	PRINT(("SynchronousRequest::ConnectionOpened()\n"));
}


void
BUrlSynchronousRequest::HostnameResolved(BUrlProtocol*, const char* ip)
{
	PRINT(("SynchronousRequest::HostnameResolved(%s)\n", ip));
}


void
BUrlSynchronousRequest::ResponseStarted(BUrlProtocol*)
{
	PRINT(("SynchronousRequest::ResponseStarted()\n"));
}


void
BUrlSynchronousRequest::HeadersReceived(BUrlProtocol*)
{
	PRINT(("SynchronousRequest::HeadersReceived()\n"));
}


void
BUrlSynchronousRequest::DataReceived(BUrlProtocol*, const char*,
	ssize_t size)
{
	PRINT(("SynchronousRequest::DataReceived(%zd)\n", size));
}


void
BUrlSynchronousRequest::DownloadProgress(BUrlProtocol*,
	ssize_t bytesReceived, ssize_t bytesTotal)
{
	PRINT(("SynchronousRequest::DownloadProgress(%zd, %zd)\n", bytesReceived,
		bytesTotal));
}


void
BUrlSynchronousRequest::UploadProgress(BUrlProtocol*, ssize_t bytesSent,
	ssize_t bytesTotal)
{
	PRINT(("SynchronousRequest::UploadProgress(%zd, %zd)\n", bytesSent,
		bytesTotal));
}


void
BUrlSynchronousRequest::RequestCompleted(BUrlProtocol* caller, bool success)
{
	PRINT(("SynchronousRequest::RequestCompleted(%s) : %s\n", (success?"true":"false"),
		caller->StatusString(caller->Status())));
	fRequestComplete = true;
}
