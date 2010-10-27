/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 */


#include <UrlProtocolDispatchingListener.h>
#include <Debug.h>


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


void
BUrlProtocolDispatchingListener::ConnectionOpened(BUrlProtocol* caller)
{
	BMessage message(B_URL_PROTOCOL_NOTIFICATION);
	_SendMessage(&message, B_URL_PROTOCOL_CONNECTION_OPENED, caller);
}


void
BUrlProtocolDispatchingListener::HostnameResolved(BUrlProtocol* caller, 
	const char* ip)
{
	BMessage message(B_URL_PROTOCOL_NOTIFICATION);
	message.AddString("url:hostIp", ip);
	
	_SendMessage(&message, B_URL_PROTOCOL_HOSTNAME_RESOLVED, caller);
}


void
BUrlProtocolDispatchingListener::ResponseStarted(BUrlProtocol* caller)
{
	BMessage message(B_URL_PROTOCOL_NOTIFICATION);
	_SendMessage(&message, B_URL_PROTOCOL_RESPONSE_STARTED, caller);
}


void
BUrlProtocolDispatchingListener::HeadersReceived(BUrlProtocol* caller)
{
	BMessage message(B_URL_PROTOCOL_NOTIFICATION);
	_SendMessage(&message, B_URL_PROTOCOL_HEADERS_RECEIVED, caller);
}


void
BUrlProtocolDispatchingListener::DataReceived(BUrlProtocol* caller, 
	const char* data, ssize_t size)
{
	BMessage message(B_URL_PROTOCOL_NOTIFICATION);
	message.AddData("url:data", B_STRING_TYPE, data, size, true, 1);
	
	_SendMessage(&message, B_URL_PROTOCOL_DATA_RECEIVED, caller);
}


void
BUrlProtocolDispatchingListener::DownloadProgress(BUrlProtocol* caller, 
	ssize_t bytesReceived, ssize_t bytesTotal)
{
	BMessage message(B_URL_PROTOCOL_NOTIFICATION);
	message.AddInt32("url:bytesReceived", bytesReceived);
	message.AddInt32("url:bytesTotal", bytesTotal);
	
	_SendMessage(&message, B_URL_PROTOCOL_DOWNLOAD_PROGRESS, caller);
}


void
BUrlProtocolDispatchingListener::UploadProgress(BUrlProtocol* caller, 
	ssize_t bytesSent, ssize_t bytesTotal)
{
	BMessage message(B_URL_PROTOCOL_NOTIFICATION);
	message.AddInt32("url:bytesSent", bytesSent);
	message.AddInt32("url:bytesTotal", bytesTotal);
	
	_SendMessage(&message, B_URL_PROTOCOL_UPLOAD_PROGRESS, caller);
}



void
BUrlProtocolDispatchingListener::RequestCompleted(BUrlProtocol* caller,
	bool success)
{
	BMessage message(B_URL_PROTOCOL_NOTIFICATION);
	message.AddBool("url:success", success);
	
	_SendMessage(&message, B_URL_PROTOCOL_REQUEST_COMPLETED, caller);
}


void
BUrlProtocolDispatchingListener::_SendMessage(BMessage* message, 
	int8 notification, BUrlProtocol* caller)
{
	ASSERT(message != NULL);
		
	message->AddPointer(kUrlProtocolCaller, caller);
	message->AddInt8(kUrlProtocolMessageType, notification);

	ASSERT(fMessenger.SendMessage(message) == B_OK);
}
