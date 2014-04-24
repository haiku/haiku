/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "WebAppInterface.h"

#include <stdio.h>

#include <File.h>
#include <HttpHeaders.h>
#include <HttpRequest.h>
#include <Message.h>
#include <Url.h>
#include <UrlContext.h>
#include <UrlProtocolListener.h>
#include <UrlProtocolRoster.h>

#include "List.h"
#include "PackageInfo.h"


class JsonBuilder {
public:
	JsonBuilder()
		:
		fString("{"),
		fInList(false)
	{
	}

	JsonBuilder& AddObject()
	{
		fString << '{';
		fInList = false;
		return *this;
	}

	JsonBuilder& AddObject(const char* name)
	{
		_StartName(name);
		fString << '{';
		fInList = false;
		return *this;
	}

	JsonBuilder& EndObject()
	{
		fString << '}';
		fInList = true;
		return *this;
	}

	JsonBuilder& AddArray(const char* name)
	{
		_StartName(name);
		fString << '[';
		fInList = false;
		return *this;
	}

	JsonBuilder& EndArray()
	{
		fString << ']';
		fInList = true;
		return *this;
	}

	JsonBuilder& AddValue(const char* name, const char* value)
	{
		_StartName(name);
		fString << '\"';
		// TODO: Escape value
		fString << value;
		fString << '\"';
		fInList = true;
		return *this;
	}

	JsonBuilder& AddValue(const char* name, int value)
	{
		_StartName(name);
		fString << value;
		fInList = true;
		return *this;
	}

	JsonBuilder& AddValue(const char* name, bool value)
	{
		_StartName(name);
		if (value)
			fString << "true";
		else
			fString << "false";
		fInList = true;
		return *this;
	}

	const BString& End()
	{
		fString << "}\n";
		return fString;
	}

private:
	void _StartName(const char* name)
	{
		if (fInList)
			fString << ",\"";
		else
			fString << '"';
		// TODO: Escape name
		fString << name;
		fString << "\":";
	}

private:
	BString		fString;
	bool		fInList;
};


class ProtocolListener : public BUrlProtocolListener {
public:
	ProtocolListener()
		:
		fDownloadIO(NULL),
		fDebug(false)
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
		if (fDebug)
			printf("ResponseStarted(%p)\n", caller);
	}
	
	virtual void HeadersReceived(BUrlRequest* caller)
	{
		if (fDebug)
			printf("HeadersReceived(%p)\n", caller);
	}
	
	virtual void DataReceived(BUrlRequest* caller, const char* data,
		off_t position, ssize_t size)
	{
		if (fDebug) {
			printf("DataReceived(%p): %ld bytes\n", caller, size);
			printf("%.*s", (int)size, data);
		}

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
		if (fDebug)
			printf("UploadProgress(%p): %ld/%ld\n", caller, bytesSent, bytesTotal);
	}
																	
	virtual void RequestCompleted(BUrlRequest* caller, bool success)
	{
		if (fDebug)
			printf("RequestCompleted(%p): %d\n", caller, success);
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

	void SetDebug(bool debug)
	{
		fDebug = debug;
	}

private:
	BDataIO*		fDownloadIO;
	bool			fDebug;
};


int
WebAppInterface::fRequestIndex = 0;



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


status_t
WebAppInterface::RetrievePackageInfo(const BString& packageName,
	BMessage& message)
{
	BUrl url("https://depot.haiku-os.org/api/v1/pkg");
	
	ProtocolListener listener;
	BUrlContext context;
	BHttpHeaders headers;	
	// Content-Type
	headers.AddHeader("Content-Type", "application/json");

	BHttpRequest request(url, true, "HTTP", &listener, &context);

	// Authentication
	if (!fUsername.IsEmpty() && !fPassword.IsEmpty()) {
		request.SetUserName(fUsername);
		request.SetPassword(fPassword);
	}

	request.SetMethod(B_HTTP_POST);
	request.SetHeaders(headers);

	BString jsonString = JsonBuilder()
		.AddValue("jsonrpc", "2.0")
		.AddValue("id", ++fRequestIndex)
		.AddValue("method", "getPkg")
		.AddArray("params")
			.AddObject()
				.AddValue("name", packageName)
				.AddValue("architectureCode", "x86_gcc2")
				.AddValue("naturalLanguageCode", "en")
				.AddValue("versionType", "NONE")
			.EndObject()
		.EndArray()
	.End();

	printf("Sending JSON:\n%s\n", jsonString.String());
	
	BMemoryIO* data = new BMemoryIO(
		jsonString.String(), jsonString.Length() - 1);

	request.AdoptInputData(data, jsonString.Length() - 1);

	BMallocIO replyData;
	listener.SetDownloadIO(&replyData);
//	listener.SetDebug(true);

	thread_id thread = request.Run();
	wait_for_thread(thread, NULL);

	const BHttpResult& result = dynamic_cast<const BHttpResult&>(
		request.Result());

	int32 statusCode = result.StatusCode();
	
	if (statusCode == 200)
		return B_OK;

       printf("Response code: %" B_PRId32 "\n", statusCode);

	return B_ERROR;
}


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
