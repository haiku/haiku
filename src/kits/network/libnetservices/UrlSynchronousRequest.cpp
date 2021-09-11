/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 */

#include <cstdio>

#include <UrlSynchronousRequest.h>

using namespace BPrivate::Network;


#define PRINT(x) printf x;


BUrlSynchronousRequest::BUrlSynchronousRequest(BUrlRequest& request)
	:
	BUrlRequest(request.Url(), request.Output(), NULL, request.Context(),
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
BUrlSynchronousRequest::HeadersReceived(BUrlRequest*)
{
	PRINT(("SynchronousRequest::HeadersReceived()\n"));
}


void
BUrlSynchronousRequest::BytesWritten(BUrlRequest* caller, size_t bytesWritten)
{
	PRINT(("SynchronousRequest::BytesWritten(%" B_PRIdSSIZE ")\n",
		bytesWritten));
}


void
BUrlSynchronousRequest::DownloadProgress(BUrlRequest*,
	off_t bytesReceived, off_t bytesTotal)
{
	PRINT(("SynchronousRequest::DownloadProgress(%" B_PRIdOFF ", %" B_PRIdOFF
		")\n", bytesReceived, bytesTotal));
}


void
BUrlSynchronousRequest::UploadProgress(BUrlRequest*, off_t bytesSent,
	off_t bytesTotal)
{
	PRINT(("SynchronousRequest::UploadProgress(%" B_PRIdOFF ", %" B_PRIdOFF
		")\n", bytesSent, bytesTotal));
}


void
BUrlSynchronousRequest::RequestCompleted(BUrlRequest* caller, bool success)
{
	PRINT(("SynchronousRequest::RequestCompleted(%s) : %s\n", (success?"true":"false"),
		strerror(caller->Status())));
	fRequestComplete = true;
}
