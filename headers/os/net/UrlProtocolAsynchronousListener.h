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
									
	// Synchronous listener access
			BUrlProtocolListener* SynchronousListener();
									
	// BHandler interface
	virtual void				MessageReceived(BMessage* message);

private:
			BUrlProtocolDispatchingListener*
						 		fSynchronousListener;
};

#endif // _B_URL_PROTOCOL_ASYNCHRONOUS_LISTENER_H_

