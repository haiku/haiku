/*
 * Copyright 2010-2017 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 */


#include <UrlProtocolDispatchingListener.h>

#include <Debug.h>
#include <UrlResult.h>

#include <assert.h>

using namespace BPrivate::Network;


const char* kUrlProtocolMessageType = "be:urlProtocolMessageType";
const char* kUrlProtocolCaller = "be:urlProtocolCaller";


BUrlProtocolDispatchingListener::BUrlProtocolDispatchingListener
	(BHandler* handler)
	:
	fMessenger(handler)
{
}


BUrlProtocolDispatchingListener::BUrlProtocolDispatchingListener
	(const BMessenger& messenger)
	:
	fMessenger(messenger)
{
}


BUrlProtocolDispatchingListener::~BUrlProtocolDispatchingListener()
{
}


void
BUrlProtocolDispatchingListener::ConnectionOpened(BUrlRequest* caller)
{
	BMessage message(B_URL_PROTOCOL_NOTIFICATION);
	_SendMessage(&message, B_URL_PROTOCOL_CONNECTION_OPENED, caller);
}


void
BUrlProtocolDispatchingListener::HostnameResolved(BUrlRequest* caller,
	const char* ip)
{
	BMessage message(B_URL_PROTOCOL_NOTIFICATION);
	message.AddString("url:hostIp", ip);

	_SendMessage(&message, B_URL_PROTOCOL_HOSTNAME_RESOLVED, caller);
}


void
BUrlProtocolDispatchingListener::ResponseStarted(BUrlRequest* caller)
{
	BMessage message(B_URL_PROTOCOL_NOTIFICATION);
	_SendMessage(&message, B_URL_PROTOCOL_RESPONSE_STARTED, caller);
}


void
BUrlProtocolDispatchingListener::HeadersReceived(BUrlRequest* caller)
{
	BMessage message(B_URL_PROTOCOL_NOTIFICATION);
	_SendMessage(&message, B_URL_PROTOCOL_HEADERS_RECEIVED, caller);
}


void
BUrlProtocolDispatchingListener::BytesWritten(BUrlRequest* caller,
	size_t bytesWritten)
{
	BMessage message(B_URL_PROTOCOL_NOTIFICATION);
	message.AddInt32("url:bytesWritten", bytesWritten);

	_SendMessage(&message, B_URL_PROTOCOL_BYTES_WRITTEN, caller);
}


void
BUrlProtocolDispatchingListener::DownloadProgress(BUrlRequest* caller,
	off_t bytesReceived, off_t bytesTotal)
{
	BMessage message(B_URL_PROTOCOL_NOTIFICATION);
	message.AddInt64("url:bytesReceived", bytesReceived);
	message.AddInt64("url:bytesTotal", bytesTotal);

	_SendMessage(&message, B_URL_PROTOCOL_DOWNLOAD_PROGRESS, caller);
}


void
BUrlProtocolDispatchingListener::UploadProgress(BUrlRequest* caller,
	off_t bytesSent, off_t bytesTotal)
{
	BMessage message(B_URL_PROTOCOL_NOTIFICATION);
	message.AddInt64("url:bytesSent", bytesSent);
	message.AddInt64("url:bytesTotal", bytesTotal);

	_SendMessage(&message, B_URL_PROTOCOL_UPLOAD_PROGRESS, caller);
}


void
BUrlProtocolDispatchingListener::RequestCompleted(BUrlRequest* caller,
	bool success)
{
	BMessage message(B_URL_PROTOCOL_NOTIFICATION);
	message.AddBool("url:success", success);

	_SendMessage(&message, B_URL_PROTOCOL_REQUEST_COMPLETED, caller);
}


void
BUrlProtocolDispatchingListener::DebugMessage(BUrlRequest* caller,
	BUrlProtocolDebugMessage type, const char* text)
{
	BMessage message(B_URL_PROTOCOL_NOTIFICATION);
	message.AddInt32("url:type", type);
	message.AddString("url:text", text);

	_SendMessage(&message, B_URL_PROTOCOL_DEBUG_MESSAGE, caller);
}


bool
BUrlProtocolDispatchingListener::CertificateVerificationFailed(
	BUrlRequest* caller, BCertificate& certificate, const char* error)
{
	BMessage message(B_URL_PROTOCOL_NOTIFICATION);
	message.AddString("url:error", error);
	message.AddPointer("url:certificate", &certificate);
	message.AddInt8(kUrlProtocolMessageType,
		B_URL_PROTOCOL_CERTIFICATE_VERIFICATION_FAILED);
	message.AddPointer(kUrlProtocolCaller, caller);

	// Warning: synchronous reply
	BMessage reply;
	fMessenger.SendMessage(&message, &reply);

	return reply.FindBool("url:continue");
}


void
BUrlProtocolDispatchingListener::_SendMessage(BMessage* message,
	int8 notification, BUrlRequest* caller)
{
	ASSERT(message != NULL);

	message->AddPointer(kUrlProtocolCaller, caller);
	message->AddInt8(kUrlProtocolMessageType, notification);

#ifdef DEBUG
	status_t result = fMessenger.SendMessage(message);
	ASSERT(result == B_OK);
#else
	fMessenger.SendMessage(message);
#endif
}
