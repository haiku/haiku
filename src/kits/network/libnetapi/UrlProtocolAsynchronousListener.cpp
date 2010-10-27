/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 */


#include <new>

#include <AppKit.h>
#include <UrlProtocolAsynchronousListener.h>
#include <Debug.h>

extern const char* kUrlProtocolMessageType;
extern const char* kUrlProtocolCaller;

BUrlProtocolAsynchronousListener::BUrlProtocolAsynchronousListener(
	bool transparent) 
	:
	BHandler("UrlProtocolAsynchronousListener"),
	fSynchronousListener(NULL)
{
	if (be_app->Lock()) {
		be_app->AddHandler(this);
		be_app->Unlock();
	}
	else
		PRINT(("Cannot lock be_app\n"));
		
	if (transparent) 
		fSynchronousListener 
			= new(std::nothrow) BUrlProtocolDispatchingListener(this);
}


BUrlProtocolAsynchronousListener::~BUrlProtocolAsynchronousListener()
{
	if (be_app->Lock()) {
		be_app->RemoveHandler(this);
		be_app->Unlock();
	}
	delete fSynchronousListener;
}


void
BUrlProtocolAsynchronousListener::ConnectionOpened(BUrlProtocol*)
{
}


void
BUrlProtocolAsynchronousListener::HostnameResolved(BUrlProtocol*, const char*)
{
}


void
BUrlProtocolAsynchronousListener::ResponseStarted(BUrlProtocol*)
{
}


void
BUrlProtocolAsynchronousListener::HeadersReceived(BUrlProtocol*)
{
}


void
BUrlProtocolAsynchronousListener::DataReceived(BUrlProtocol*, const char*, 
	ssize_t)
{
}


void
BUrlProtocolAsynchronousListener::DownloadProgress(BUrlProtocol*, ssize_t, 
	ssize_t)
{
}


void
BUrlProtocolAsynchronousListener::UploadProgress(BUrlProtocol*, ssize_t, 
	ssize_t)
{
}


void
BUrlProtocolAsynchronousListener::RequestCompleted(BUrlProtocol*, bool)
{
}


// #pragma mark Synchronous listener access


BUrlProtocolListener*
BUrlProtocolAsynchronousListener::SynchronousListener()
{
	return fSynchronousListener;
}


void
BUrlProtocolAsynchronousListener::MessageReceived(BMessage* message)
{
	if (message->what != B_URL_PROTOCOL_NOTIFICATION) {
		BHandler::MessageReceived(message);
		return;
	}
	
	BUrlProtocol* caller;
	if (message->FindPointer(kUrlProtocolCaller,
		reinterpret_cast<void**>(&caller)) != B_OK)
		return;
		
	int8 notification;
	if (message->FindInt8(kUrlProtocolMessageType, &notification) 
		!= B_OK)
		return;
		
	switch (notification) {
		case B_URL_PROTOCOL_CONNECTION_OPENED:
			ConnectionOpened(caller);
			break;
			
		case B_URL_PROTOCOL_HOSTNAME_RESOLVED:
			{
				const char* ip;
				message->FindString("url:ip", &ip);
				
				HostnameResolved(caller, ip);
			}
			break;
			
		case B_URL_PROTOCOL_RESPONSE_STARTED:
			ResponseStarted(caller);
			break;
			
		case B_URL_PROTOCOL_HEADERS_RECEIVED:
			HeadersReceived(caller);
			break;
			
		case B_URL_PROTOCOL_DATA_RECEIVED:
			{
				const char* data;
				ssize_t		size;
				message->FindData("url:data", B_STRING_TYPE, 
					reinterpret_cast<const void**>(&data), &size);
				
				DataReceived(caller, data, size);
			}
			break;
			
		case B_URL_PROTOCOL_DOWNLOAD_PROGRESS:
			{
				int32 bytesReceived;
				int32 bytesTotal;
				message->FindInt32("url:bytesReceived", &bytesReceived);
				message->FindInt32("url:bytesTotal", &bytesTotal);
				
				DownloadProgress(caller, bytesReceived, bytesTotal);
			}
			break;
			
		case B_URL_PROTOCOL_UPLOAD_PROGRESS:
			{
				int32 bytesSent;
				int32 bytesTotal;
				message->FindInt32("url:bytesSent", &bytesSent);
				message->FindInt32("url:bytesTotal", &bytesTotal);
				
				UploadProgress(caller, bytesSent, bytesTotal);
			}
			break;
			
		case B_URL_PROTOCOL_REQUEST_COMPLETED:
			{
				bool success;
				message->FindBool("url:success", &success);
				
				RequestCompleted(caller, success);
			}
			break;
			
		default:
			PRINT(("BUrlProtocolAsynchronousListener: Unknown notification %d\n",
				notification));
			break;
	}
}
