/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "ConnectionTest.h"

#include <stdio.h>

#include <HttpRequest.h>
#include <Url.h>
#include <UrlContext.h>
#include <UrlProtocolListener.h>
#include <UrlProtocolRoster.h>


class ProtocolListener : public BUrlProtocolListener {
	virtual	void ConnectionOpened(BUrlRequest* caller)
	{
		printf("ConnectionOpened(%p)\n", caller);
	}
	
	virtual void HostnameResolved(BUrlRequest* caller, const char* ip)
	{
		printf("HostnameResolved(%p): %s\n", caller, ip);
	}
									
	virtual void ResponseStarted(BUrlRequest* caller)
	{
		printf("ResponseStarted(%p)\n", caller);
	}
	
	virtual void HeadersReceived(BUrlRequest* caller)
	{
		printf("HeadersReceived(%p)\n", caller);
	}
	
	virtual void DataReceived(BUrlRequest* caller, const char* data,
		ssize_t size)
	{
		printf("DataReceived(%p): %ld bytes\n", caller, size);
	}
									
	virtual	void DownloadProgress(BUrlRequest* caller, ssize_t bytesReceived,
		ssize_t bytesTotal)
	{
		printf("DownloadProgress(%p): %ld/%ld\n", caller, bytesReceived,
			bytesTotal);
	}
									
	virtual void UploadProgress(BUrlRequest* caller, ssize_t bytesSent,
		ssize_t bytesTotal)
	{
		printf("UploadProgress(%p): %ld/%ld\n", caller, bytesSent, bytesTotal);
	}
																	
	virtual void RequestCompleted(BUrlRequest* caller, bool success)
	{
		printf("RequestCompleted(%p): %d\n", caller, success);
	}
									
	virtual void DebugMessage(BUrlRequest* caller,
		BUrlProtocolDebugMessage type, const char* text)
	{
		printf("DebugMessage(%p): %s\n", caller, text);
	}
};


ConnectionTest::ConnectionTest()
	:
	BApplication("application/x-vnd.Haiku-HaikuDepot-ConnectionTest")
{
}


ConnectionTest::~ConnectionTest()
{
}


void
ConnectionTest::ReadyToRun()
{
	printf("Connecting...\n");
	
	BUrl url("https://depot.haiku-os.org");
	
	ProtocolListener listener;
	BUrlContext context;
	
	BHttpRequest request(url, true, "HTTP", &listener, &context);


	thread_id thread = request.Run();
	wait_for_thread(thread, NULL);

	PostMessage(B_QUIT_REQUESTED, this);
}


int
main(int argc, char* argv[])
{
	ConnectionTest().Run();
	return 0;
}


