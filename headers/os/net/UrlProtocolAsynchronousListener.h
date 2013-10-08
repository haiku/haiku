/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_URL_PROTOCOL_ASYNCHRONOUS_LISTENER_H_
#define _B_URL_PROTOCOL_ASYNCHRONOUS_LISTENER_H_


#include <Handler.h>
#include <Message.h>
#include <UrlProtocolDispatchingListener.h>


class BUrlProtocolAsynchronousListener : public BHandler {
public:
								BUrlProtocolAsynchronousListener(
									bool transparent = false);
	virtual						~BUrlProtocolAsynchronousListener();

	virtual	void				ConnectionOpened(BUrlRequest* caller);
	virtual void				HostnameResolved(BUrlRequest* caller,
									const char* ip);
	virtual void				ResponseStarted(BUrlRequest* caller);
	virtual void				HeadersReceived(BUrlRequest* caller);
	virtual void				DataReceived(BUrlRequest* caller,
									const char* data, ssize_t size);
	virtual	void				DownloadProgress(BUrlRequest* caller,
									ssize_t bytesReceived, ssize_t bytesTotal);
	virtual void				UploadProgress(BUrlRequest* caller,
									ssize_t bytesSent, ssize_t bytesTotal);
	virtual void				RequestCompleted(BUrlRequest* caller,
									bool success);
									
	// Synchronous listener access
			BUrlProtocolListener* SynchronousListener();
									
	// BHandler interface
	virtual void				MessageReceived(BMessage* message);

private:
			BUrlProtocolDispatchingListener*
						 		fSynchronousListener;
};

#endif // _B_URL_PROTOCOL_ASYNCHRONOUS_LISTENER_H_

