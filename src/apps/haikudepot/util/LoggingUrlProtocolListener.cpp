/*
 * Copyright 2017-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "LoggingUrlProtocolListener.h"

#include <File.h>
#include <HttpRequest.h>

#include "Logger.h"

using namespace BPrivate::Network;


LoggingUrlProtocolListener::LoggingUrlProtocolListener(
	BString traceLoggingIdentifier, bool traceLogging)
	:
	fTraceLogging(traceLogging),
	fTraceLoggingIdentifier(traceLoggingIdentifier),
	fContentLength(0)
{
}


void
LoggingUrlProtocolListener::BytesWritten(BUrlRequest* caller,
	size_t bytesWritten)
{
	fContentLength += bytesWritten;
}


void
LoggingUrlProtocolListener::DebugMessage(BUrlRequest* caller,
	BUrlProtocolDebugMessage type, const char* text)
{
	HDTRACE("url->file <%s>; %s", fTraceLoggingIdentifier.String(), text);
}


size_t
LoggingUrlProtocolListener::ContentLength()
{
	return fContentLength;
}
