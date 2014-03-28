/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "WebAppInterface.h"

#include <stdio.h>

#include <File.h>
#include <HttpHeaders.h>
#include <HttpRequest.h>
#include <Url.h>
#include <UrlContext.h>
#include <UrlProtocolListener.h>
#include <UrlProtocolRoster.h>

#include "PackageInfo.h"


class ProtocolListener : public BUrlProtocolListener {
public:
	ProtocolListener::ProtocolListener()
		:
		fDownloadIO(NULL)
	{
	}

	virtual	void ConnectionOpened(BUrlRequest* caller)
	{
//		printf("ConnectionOpened(%p)\n", caller);
	}
	
	virtual void HostnameResolved(BUrlRequest* caller, const char* ip)
	{
//		printf("HostnameResolved(%p): %s\n", caller, ip);
	}
									
	virtual void ResponseStarted(BUrlRequest* caller)
	{
//		printf("ResponseStarted(%p)\n", caller);
	}
	
	virtual void HeadersReceived(BUrlRequest* caller)
	{
//		printf("HeadersReceived(%p)\n", caller);
	}
	
	virtual void DataReceived(BUrlRequest* caller, const char* data,
		ssize_t size)
	{
//		printf("DataReceived(%p): %ld bytes\n", caller, size);

		if (fDownloadIO != NULL)
			fDownloadIO->Write(data, size);
	}
									
	virtual	void DownloadProgress(BUrlRequest* caller, ssize_t bytesReceived,
		ssize_t bytesTotal)
	{
//		printf("DownloadProgress(%p): %ld/%ld\n", caller, bytesReceived,
//			bytesTotal);
	}
									
	virtual void UploadProgress(BUrlRequest* caller, ssize_t bytesSent,
		ssize_t bytesTotal)
	{
//		printf("UploadProgress(%p): %ld/%ld\n", caller, bytesSent, bytesTotal);
	}
																	
	virtual void RequestCompleted(BUrlRequest* caller, bool success)
	{
//		printf("RequestCompleted(%p): %d\n", caller, success);
	}
									
	virtual void DebugMessage(BUrlRequest* caller,
		BUrlProtocolDebugMessage type, const char* text)
	{
//		printf("DebugMessage(%p): %s\n", caller, text);
	}

	void SetDownloadIO(BDataIO* downloadIO)
	{
		fDownloadIO = downloadIO;
	}

private:
	BDataIO*		fDownloadIO;
};


WebAppInterface::WebAppInterface()
{
}


WebAppInterface::~WebAppInterface()
{
}


void
WebAppInterface::SetAuthorization(const BString& username,
	const BString& password)
{
	fUsername = username;
	fPassword = password;
}


//void
//WebAppInterface::_TestGetPackage()
//{
//	BUrl url("https://depot.haiku-os.org/api/v1/pkg");
//	
//	ProtocolListener listener;
//	BUrlContext context;
//	BHttpHeaders headers;	
//
//	// Authentication
//	if (!fUsername.IsEmpty() && !fPassword.IsEmpty()) {
//		BString plain;
//		plain << fUsername << ':' << fPassword;
//
//		char base64[1024];
//		ssize_t base64Size = encode_base64(base64, plain, plain.Length(),
//			false);
//
//		if (base64Size > 0 && base64Size <= (ssize_t)sizeof(base64)) {
//			base64[base64Size] = '\0';
//
//			BString authorization("Basic ");
//			authorization << base64;
//			
//			headers.AddHeader("Authorization", authorization);
//		}
//	}
//	// Content-Type
//	headers.AddHeader("Content-Type", "application/json");
//
//	BHttpRequest request(url, true, "HTTP", &listener, &context);
//	request.SetMethod(B_HTTP_POST);
//	request.SetHeaders(headers);
//
//	BMallocIO* data = new BMallocIO();
//	
//	BString jsonString
//		=	"{"
//				"\"jsonrpc\":\"2.0\","
//				"\"id\":4143431,"
//				"\"method\":\"getPkg\","
//				"\"params\":[{"
//					"\"name\":\"apr\","
//					"\"architectureCode\":\"x86\","
//					"\"versionType\":\"NONE\""
//				"}]"
//			"}"
//		;
//	
//	data->WriteAt(0, jsonString.String(), jsonString.Length());
//	data->Seek(0, SEEK_SET);
//	request.AdoptInputData(data);
//
//	thread_id thread = request.Run();
//	wait_for_thread(thread, NULL);
//}


status_t
WebAppInterface::RetrievePackageIcon(const BString& packageName,
	BDataIO* stream)
{
	BString urlString = "https://depot.haiku-os.org/pkgicon/";
	urlString << packageName << ".hvif";
	
	BUrl url(urlString);
	
	ProtocolListener listener;
	listener.SetDownloadIO(stream);
	
	BHttpRequest request(url, true, "HTTP", &listener);
	request.SetMethod(B_HTTP_GET);

	thread_id thread = request.Run();
	wait_for_thread(thread, NULL);

	const BHttpResult& result = dynamic_cast<const BHttpResult&>(
		request.Result());

	int32 statusCode = result.StatusCode();
	
	if (statusCode == 200)
		return B_OK;

	return B_ERROR;
}
