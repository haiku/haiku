/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_URL_SYNCHRONOUS_REQUEST_H_
#define _B_URL_SYNCHRONOUS_REQUEST_H_


#include <UrlRequest.h>
#include <UrlProtocolListener.h>


#ifndef LIBNETAPI_DEPRECATED
namespace BPrivate {

namespace Network {
#endif

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

#ifdef LIBNETAPI_DEPRECATED
	virtual	void				DownloadProgress(BUrlRequest* caller,
									ssize_t bytesReceived, ssize_t bytesTotal);
	virtual void				UploadProgress(BUrlRequest* caller,
									ssize_t bytesSent, ssize_t bytesTotal);
#else
	virtual	void				DownloadProgress(BUrlRequest* caller,
									off_t bytesReceived, off_t bytesTotal);
	virtual void				UploadProgress(BUrlRequest* caller,
									off_t bytesSent, off_t bytesTotal);
#endif //LIBNETAPI_DEPRECATED

	virtual void				RequestCompleted(BUrlRequest* caller,
									bool success);
									
									
protected:
			bool				fRequestComplete;
			BUrlRequest&		fWrappedRequest;
};


#ifndef LIBNETAPI_DEPRECATED
} // namespace Network

} // namespace BPrivate
#endif

#endif // _B_URL_SYNCHRONOUS_REQUEST_H_
