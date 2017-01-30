/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "ToFileUrlProtocolListener.h"

#include <File.h>

#include <stdio.h>


ToFileUrlProtocolListener::ToFileUrlProtocolListener(BPath path,
	BString traceLoggingIdentifier, bool traceLogging)
{
	fDownloadIO = new BFile(path.Path(), O_WRONLY | O_CREAT);
	fTraceLoggingIdentifier = traceLoggingIdentifier;
	fTraceLogging = traceLogging;
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
}


void
ToFileUrlProtocolListener::DataReceived(BUrlRequest* caller, const char* data,
	off_t position, ssize_t size)
{
	if (fDownloadIO != NULL && size > 0) {
		size_t remaining = size;
		size_t written = 0;

		do {
			written = fDownloadIO->Write(&data[size - remaining], remaining);
			remaining -= written;
		} while (remaining > 0 && written > 0);

		if (remaining > 0)
			fprintf(stdout, "unable to write all of the data to the file\n");
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
	if (fTraceLogging) {
		fprintf(stdout, "url->file <%s>; %s\n",
			fTraceLoggingIdentifier.String(), text);
	}
}

