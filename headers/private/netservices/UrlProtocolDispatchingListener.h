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
	B_URL_PROTOCOL_REQUEST_COMPLETED,
	B_URL_PROTOCOL_CERTIFICATE_VERIFICATION_FAILED,
	B_URL_PROTOCOL_DEBUG_MESSAGE
};


class BUrlProtocolDispatchingListener : public BUrlProtocolListener {
public:
								BUrlProtocolDispatchingListener(
									BHandler* handler);
								BUrlProtocolDispatchingListener(
									const BMessenger& messenger);
	virtual						~BUrlProtocolDispatchingListener();

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
	virtual void				DebugMessage(BUrlRequest* caller,
									BUrlProtocolDebugMessage type,
									const char* text);
	virtual bool				CertificateVerificationFailed(
									BUrlRequest* caller,
									BCertificate& certificate,
									const char* message);

private:
			void				_SendMessage(BMessage* message,
									int8 notification, BUrlRequest* caller);

private:
			BMessenger	 		fMessenger;
};

#endif // _B_URL_PROTOCOL_DISPATCHING_LISTENER_H_

