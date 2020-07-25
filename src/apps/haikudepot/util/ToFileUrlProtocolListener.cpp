/*
 * Copyright 2017-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "ToFileUrlProtocolListener.h"

#include <File.h>
#include <HttpRequest.h>

#include "Logger.h"

ToFileUrlProtocolListener::ToFileUrlProtocolListener(BPath path,
	BString traceLoggingIdentifier, bool traceLogging)
{
	fDownloadIO = new BFile(path.Path(), O_WRONLY | O_CREAT);
	fTraceLoggingIdentifier = traceLoggingIdentifier;
	fTraceLogging = traceLogging;
	fShouldDownload = true;
	fContentLength = 0;
}


ToFileUrlProtocolListener::~ToFileUrlProtocolListener()
{
	delete fDownloadIO;
}


void
ToFileUrlProtocolListener::ConnectionOpened(BUrlRequest* caller)
{
}


void
ToFileUrlProtocolListener::HostnameResolved(BUrlRequest* caller,
	const char* ip)
{
}


void
ToFileUrlProtocolListener::ResponseStarted(BUrlRequest* caller)
{
}


void
ToFileUrlProtocolListener::HeadersReceived(BUrlRequest* caller,
	const BUrlResult& result)
{

	// check that the status code is success.  Only if it is successful
	// should the payload be streamed to the file.

	const BHttpResult& httpResult = dynamic_cast<const BHttpResult&>(result);
	int32 statusCode = httpResult.StatusCode();

	if (!BHttpRequest::IsSuccessStatusCode(statusCode)) {
		HDINFO("received http status %" B_PRId32
			" --> will not store download to file", statusCode);
		fShouldDownload = false;
	}

}


void
ToFileUrlProtocolListener::DataReceived(BUrlRequest* caller, const char* data,
	off_t position, ssize_t size)
{
	fContentLength += size;

	if (fShouldDownload && fDownloadIO != NULL && size > 0) {
		size_t remaining = size;
		size_t written = 0;

		do {
			written = fDownloadIO->WriteAt(position, &data[size - remaining],
				remaining);
			remaining -= written;
		} while (remaining > 0 && written > 0);

		if (remaining > 0)
			HDERROR("unable to write all of the data to the file");
	}
}


void
ToFileUrlProtocolListener::DownloadProgress(BUrlRequest* caller,
	ssize_t bytesReceived, ssize_t bytesTotal)
{
}


void
ToFileUrlProtocolListener::UploadProgress(BUrlRequest* caller,
	ssize_t bytesSent, ssize_t bytesTotal)
{
}


void
ToFileUrlProtocolListener::RequestCompleted(BUrlRequest* caller, bool success)
{
}


void
ToFileUrlProtocolListener::DebugMessage(BUrlRequest* caller,
	BUrlProtocolDebugMessage type, const char* text)
{
	HDTRACE("url->file <%s>; %s", fTraceLoggingIdentifier.String(), text);
}


ssize_t
ToFileUrlProtocolListener::ContentLength()
{
	return fContentLength;
}
