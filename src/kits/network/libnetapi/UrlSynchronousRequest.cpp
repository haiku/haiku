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


BUrlSynchronousRequest::BUrlSynchronousRequest(BUrlRequest& request)
	:
	BUrlRequest(request.Url(), NULL, request.Context(),
		"BUrlSynchronousRequest", request.Protocol()),
	fRequestComplete(false),
	fWrappedRequest(request)
{
}


status_t
BUrlSynchronousRequest::Perform()
{
	fWrappedRequest.SetListener(this);
	fRequestComplete = false;

	thread_id worker = fWrappedRequest.Run();
		// TODO something to do with the thread_id maybe ?

	if (worker < B_OK)
		return worker;
	else
		return B_OK;
}


status_t
BUrlSynchronousRequest::WaitUntilCompletion()
{
	while (!fRequestComplete)
		snooze(10000);

	return B_OK;
}


void
BUrlSynchronousRequest::ConnectionOpened(BUrlRequest*)
{
	PRINT(("SynchronousRequest::ConnectionOpened()\n"));
}


void
BUrlSynchronousRequest::HostnameResolved(BUrlRequest*, const char* ip)
{
	PRINT(("SynchronousRequest::HostnameResolved(%s)\n", ip));
}


void
BUrlSynchronousRequest::ResponseStarted(BUrlRequest*)
{
	PRINT(("SynchronousRequest::ResponseStarted()\n"));
}


void
BUrlSynchronousRequest::HeadersReceived(BUrlRequest*, const BUrlResult& result)
{
	PRINT(("SynchronousRequest::HeadersReceived()\n"));
}


void
BUrlSynchronousRequest::DataReceived(BUrlRequest*, const char*,
	off_t, ssize_t size)
{
	PRINT(("SynchronousRequest::DataReceived(%zd)\n", size));
}


void
BUrlSynchronousRequest::DownloadProgress(BUrlRequest*,
	ssize_t bytesReceived, ssize_t bytesTotal)
{
	PRINT(("SynchronousRequest::DownloadProgress(%zd, %zd)\n", bytesReceived,
		bytesTotal));
}


void
BUrlSynchronousRequest::UploadProgress(BUrlRequest*, ssize_t bytesSent,
	ssize_t bytesTotal)
{
	PRINT(("SynchronousRequest::UploadProgress(%zd, %zd)\n", bytesSent,
		bytesTotal));
}


void
BUrlSynchronousRequest::RequestCompleted(BUrlRequest* caller, bool success)
{
	PRINT(("SynchronousRequest::RequestCompleted(%s) : %s\n", (success?"true":"false"),
		strerror(caller->Status())));
	fRequestComplete = true;
}
