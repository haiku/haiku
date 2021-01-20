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
								BUrlSynchronousRequest(BUrlRequest& asynchronousRequest);
	virtual						~BUrlSynchronousRequest() { };
								
	// Synchronous wait
	virtual	status_t			Perform();
	virtual	status_t			WaitUntilCompletion();

	// Protocol hooks
	virtual	void				ConnectionOpened(BUrlRequest* caller);
	virtual void				HostnameResolved(BUrlRequest* caller,
									const char* ip);
	virtual void				ResponseStarted(BUrlRequest* caller);
	virtual void				HeadersReceived(BUrlRequest* caller,
									const BUrlResult& result);
	virtual void				DataReceived(BUrlRequest* caller,
									const char* data, off_t position,
									ssize_t size);
	virtual	void				DownloadProgress(BUrlRequest* caller,
									ssize_t bytesReceived, ssize_t bytesTotal);
	virtual void				UploadProgress(BUrlRequest* caller,
									ssize_t bytesSent, ssize_t bytesTotal);
	virtual void				RequestCompleted(BUrlRequest* caller,
									bool success);
									
									
protected:
			bool				fRequestComplete;
			BUrlRequest&		fWrappedRequest;
};


#endif // _B_URL_SYNCHRONOUS_REQUEST_H_
