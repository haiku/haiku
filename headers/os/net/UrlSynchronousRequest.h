/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_URL_SYNCHRONOUS_REQUEST_H_
#define _B_URL_SYNCHRONOUS_REQUEST_H_


#include <UrlRequest.h>
#include <UrlProtocolListener.h>


class BUrlSynchronousRequest : public BUrlRequest, public BUrlProtocolListener {
public:
								BUrlSynchronousRequest(BUrl& url);
	virtual						~BUrlSynchronousRequest() { };
								
	// Synchronous wait
	virtual	status_t			Perform();
	virtual	status_t			WaitUntilCompletion();

	// Protocol hooks
	virtual	void				ConnectionOpened(BUrlProtocol* caller);
	virtual void				HostnameResolved(BUrlProtocol* caller,
									const char* ip);
	virtual void				ResponseStarted(BUrlProtocol* caller);
	virtual void				HeadersReceived(BUrlProtocol* caller);
	virtual void				DataReceived(BUrlProtocol* caller,
									const char* data, ssize_t size);
	virtual	void				DownloadProgress(BUrlProtocol* caller,
									ssize_t bytesReceived, ssize_t bytesTotal);
	virtual void				UploadProgress(BUrlProtocol* caller,
									ssize_t bytesSent, ssize_t bytesTotal);
	virtual void				RequestCompleted(BUrlProtocol* caller, 
									bool success);
									
									
protected:
			bool				fRequestComplete;
};


#endif // _B_URL_SYNCHRONOUS_REQUEST_H_
