/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <UrlProtocolListener.h>
#include <UrlRequest.h>

using BPrivate::Network::BUrlProtocolDebugMessage;
using BPrivate::Network::BUrlProtocolListener;
using BPrivate::Network::BUrlRequest;


class LoggingUrlProtocolListener : public BUrlProtocolListener {
public:
								LoggingUrlProtocolListener(
									BString traceLoggingIdentifier,
									bool traceLogging);

			size_t				ContentLength();

			void				BytesWritten(BUrlRequest* caller,
									size_t bytesWritten);
			void				DebugMessage(BUrlRequest* caller,
									BUrlProtocolDebugMessage type,
									const char* text);

private:
			bool				fTraceLogging;
			BString				fTraceLoggingIdentifier;
			size_t				fContentLength;


};
