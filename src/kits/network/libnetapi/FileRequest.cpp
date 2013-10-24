/*
 * Copyright 2013 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Adrien Destugues, pulkomandy@pulkomandy.tk
 */


#include <new>
#include <stdlib.h>

#include <arpa/inet.h>

#include <File.h>
#include <FileRequest.h>


BFileRequest::BFileRequest(const BUrl& url, BUrlProtocolListener* listener,
	BUrlContext* context)
	:
	BUrlRequest(url, listener, context, "BUrlProtocol.File", "file")
{
}


BFileRequest::~BFileRequest()
{
}


status_t
BFileRequest::_ProtocolLoop()
{
	// FIXME error handling (file does not exists, etc.)
	BFile file(fUrl.Path().String(), B_READ_ONLY); 

	if(file.InitCheck() != B_OK)
		return B_PROT_CONNECTION_FAILED;

	// Send all notifications to listener, if any
	if (fListener != NULL) {
		fListener->ConnectionOpened(this);
		off_t size = 0;
		file.GetSize(&size);
		fListener->DownloadProgress(this, size, size);

		size_t chunkSize;
		char chunk[4096];
		while((chunkSize = file.Read(chunk, sizeof(chunk))) > 0)
			fListener->DataReceived(this, chunk, chunkSize);
	}

	return B_PROT_SUCCESS;
}

