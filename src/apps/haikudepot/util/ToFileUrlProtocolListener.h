/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <UrlProtocolListener.h>
#include <UrlRequest.h>

class ToFileUrlProtocolListener : public BUrlProtocolListener {
public:
								ToFileUrlProtocolListener(BPath path,
									BString traceLoggingIdentifier,
									bool traceLogging);
			virtual				~ToFileUrlProtocolListener();

			void				ConnectionOpened(BUrlRequest* caller);
			void				HostnameResolved(BUrlRequest* caller,
									const char* ip);
			void				ResponseStarted(BUrlRequest* caller);
			void				HeadersReceived(BUrlRequest* caller,
									const BUrlResult& result);
			void				DataReceived(BUrlRequest* caller,
									const char* data, off_t position,
									ssize_t size);
			void				DownloadProgress(BUrlRequest* caller,
									ssize_t bytesReceived, ssize_t bytesTotal);
			void				UploadProgress(BUrlRequest* caller,
									ssize_t bytesSent, ssize_t bytesTotal);
			void				RequestCompleted(BUrlRequest* caller,
									bool success);
			void				DebugMessage(BUrlRequest* caller,
									BUrlProtocolDebugMessage type,
									const char* text);

private:
			bool				fShouldDownload;
			bool				fTraceLogging;
			BString				fTraceLoggingIdentifier;
			BPositionIO*		fDownloadIO;


};
