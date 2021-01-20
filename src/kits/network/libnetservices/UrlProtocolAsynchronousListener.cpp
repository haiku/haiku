/*
 * Copyright 2010-2017 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 */


#include <UrlProtocolAsynchronousListener.h>

#include <new>

#include <AppKit.h>
#include <Archivable.h>
#include <Debug.h>
#include <String.h>
#include <UrlResult.h>


extern const char* kUrlProtocolMessageType;
extern const char* kUrlProtocolCaller;


BUrlProtocolAsynchronousListener::BUrlProtocolAsynchronousListener(
	bool transparent)
	:
	BHandler("UrlProtocolAsynchronousListener"),
	BUrlProtocolListener(),
	fSynchronousListener(NULL)
{
	if (be_app->Lock()) {
		be_app->AddHandler(this);
		be_app->Unlock();
	} else
		PRINT(("Cannot lock be_app\n"));

	if (transparent) {
		fSynchronousListener
			= new(std::nothrow) BUrlProtocolDispatchingListener(this);
	}
}


BUrlProtocolAsynchronousListener::~BUrlProtocolAsynchronousListener()
{
	if (be_app->Lock()) {
		be_app->RemoveHandler(this);
		be_app->Unlock();
	}
	delete fSynchronousListener;
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

	BUrlRequest* caller;
	if (message->FindPointer(kUrlProtocolCaller, (void**)&caller) != B_OK)
		return;

	int8 notification;
	if (message->FindInt8(kUrlProtocolMessageType, &notification) != B_OK)
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
			{
				BMessage archive;
				message->FindMessage("url:result", &archive);
				BUrlResult* result = dynamic_cast<BUrlResult*>(
					instantiate_object(&archive));
				if (result == NULL) {
					debugger("Failed to unarchive BUrlResult");
					result = new BUrlResult();
				}
				HeadersReceived(caller, *result);
				delete result;
			}
			break;

		case B_URL_PROTOCOL_DATA_RECEIVED:
			{
				const char* data;
				ssize_t		size = 0;
				if (message->FindData("url:data", B_STRING_TYPE,
					reinterpret_cast<const void**>(&data), &size) != B_OK) {
					printf("BOGUS DATA MESSAGE\n");
					message->PrintToStream();
					return;
				}

				off_t position = message->FindInt32("url:position");

				DataReceived(caller, data, position, size);
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

		case B_URL_PROTOCOL_DEBUG_MESSAGE:
			{
				BUrlProtocolDebugMessage type
					= (BUrlProtocolDebugMessage)message->FindInt32("url:type");
				BString text = message->FindString("url:text");

				DebugMessage(caller, type, text);
			}
			break;

		case B_URL_PROTOCOL_CERTIFICATE_VERIFICATION_FAILED:
			{
				const char* error = message->FindString("url:error");
				BCertificate* certificate;
				message->FindPointer("url:certificate", (void**)&certificate);
				bool result = CertificateVerificationFailed(caller,
					*certificate, error);

				BMessage reply;
				reply.AddBool("url:continue", result);
				message->SendReply(&reply);
			}
			break;

		default:
			PRINT(("BUrlProtocolAsynchronousListener: Unknown notification %d\n",
				notification));
			break;
	}
}
