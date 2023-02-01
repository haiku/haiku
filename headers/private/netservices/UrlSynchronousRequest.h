/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_URL_SYNCHRONOUS_REQUEST_H_
#define _B_URL_SYNCHRONOUS_REQUEST_H_


#include <UrlRequest.h>
#include <UrlProtocolListener.h>


namespace BPrivate {

namespace Network {


class BUrlSynchronousRequest : public BUrlRequest, public BUrlProtocolListener {
public:
								BUrlSynchronousRequest(BUrlRequest& asynchronousRequest);
	virtual						~BUrlSynchronousRequest() { };

	// Synchronous wait
	virtual	status_t			Perform();
	virtual	status_t			WaitUntilCompletion();

	// Protocol hooks
	virtual	void				ConnectionOpened(BUrlRequest* caller);
	virtual	void				HostnameResolved(BUrlRequest* caller,
									const char* ip);
	virtual	void				ResponseStarted(BUrlRequest* caller);
	virtual	void				HeadersReceived(BUrlRequest* caller);
	virtual	void				BytesWritten(BUrlRequest* caller,
									size_t bytesWritten);
	virtual	void				DownloadProgress(BUrlRequest* caller,
									off_t bytesReceived, off_t bytesTotal);
	virtual void				UploadProgress(BUrlRequest* caller,
									off_t bytesSent, off_t bytesTotal);

	virtual	void				RequestCompleted(BUrlRequest* caller,
									bool success);

protected:
			bool				fRequestComplete;
			BUrlRequest&		fWrappedRequest;
};


} // namespace Network

} // namespace BPrivate

#endif // _B_URL_SYNCHRONOUS_REQUEST_H_
