/*
 * Copyright 2013 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Adrien Destugues, pulkomandy@pulkomandy.tk
 */


#include <stdlib.h>

#include <File.h>
#include <FileRequest.h>
#include <NodeInfo.h>


BFileRequest::BFileRequest(const BUrl& url, BUrlProtocolListener* listener,
	BUrlContext* context)
	:
	BUrlRequest(url, listener, context, "BUrlProtocol.File", "file"),
	fResult()
{
}


BFileRequest::~BFileRequest()
{
}


const BUrlResult&
BFileRequest::Result() const
{
	return fResult;
}


status_t
BFileRequest::_ProtocolLoop()
{
	// FIXME error handling (file does not exists, etc.)
	BFile file(fUrl.Path().String(), B_READ_ONLY);

	if (file.InitCheck() != B_OK || !file.IsFile())
		return B_PROT_CONNECTION_FAILED;

	// Send all notifications to listener, if any
	if (fListener != NULL) {
		fListener->ConnectionOpened(this);
		off_t size = 0;
		file.GetSize(&size);
		fListener->DownloadProgress(this, size, size);
		fResult.SetLength(size);

		ssize_t chunkSize;
		char chunk[4096];
		while ((chunkSize = file.Read(chunk, sizeof(chunk))) > 0)
			fListener->DataReceived(this, chunk, chunkSize);
	}

	BNodeInfo info(&file);
	char mimeType[B_MIME_TYPE_LENGTH + 1];
	if (info.GetType(mimeType) == B_OK)
		fResult.SetContentType(mimeType);

	return B_PROT_SUCCESS;
}
