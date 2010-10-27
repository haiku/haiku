/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_URL_PROTOCOL_DISPATCHING_LISTENER_H_
#define _B_URL_PROTOCOL_DISPATCHING_LISTENER_H_


#include <Messenger.h>
#include <Message.h>
#include <UrlProtocolListener.h>


//! To be in AppTypes.h
enum {
	B_URL_PROTOCOL_NOTIFICATION = '_UPN'
};


// Notification types
enum {
	B_URL_PROTOCOL_CONNECTION_OPENED,
	B_URL_PROTOCOL_HOSTNAME_RESOLVED,
	B_URL_PROTOCOL_RESPONSE_STARTED,
	B_URL_PROTOCOL_HEADERS_RECEIVED,
	B_URL_PROTOCOL_DATA_RECEIVED,
	B_URL_PROTOCOL_DOWNLOAD_PROGRESS,
	B_URL_PROTOCOL_UPLOAD_PROGRESS,
	B_URL_PROTOCOL_REQUEST_COMPLETED
};


class BUrlProtocolDispatchingListener : public BUrlProtocolListener {
public:
								BUrlProtocolDispatchingListener(
									BHandler* handler);
								BUrlProtocolDispatchingListener(
									const BMessenger& messenger);

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
	virtual void				DebugMessage(BUrlProtocol*,
									BUrlProtocolDebugMessage, 
									const char*) { }

private:
			void				_SendMessage(BMessage* message, 
									int8 notification, BUrlProtocol* caller);

private:
			BMessenger	 		fMessenger;
};

#endif // _B_URL_PROTOCOL_DISPATCHING_LISTENER_H_

