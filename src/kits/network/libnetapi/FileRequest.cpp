/*
 * Copyright 2013-2014 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Adrien Destugues, pulkomandy@pulkomandy.tk
 */


#include <assert.h>
#include <stdlib.h>

#include <Directory.h>
#include <File.h>
#include <FileRequest.h>
#include <NodeInfo.h>
#include <Path.h>


BFileRequest::BFileRequest(const BUrl& url, BUrlProtocolListener* listener,
	BUrlContext* context)
	:
	BUrlRequest(url, listener, context, "BUrlProtocol.File", "file"),
	fResult()
{
	fUrl.UrlDecode(true);
}


BFileRequest::~BFileRequest()
{
	status_t status = Stop();
	if (status == B_OK)
		wait_for_thread(fThreadId, &status);
}


const BUrlResult&
BFileRequest::Result() const
{
	return fResult;
}


status_t
BFileRequest::_ProtocolLoop()
{
	BNode node(fUrl.Path().String());

	if (node.IsSymLink()) {
		// Traverse the symlink and start over
		BEntry entry(fUrl.Path().String(), true);
		node = BNode(&entry);
	}

	ssize_t transferredSize = 0;
	if (node.IsFile()) {
		BFile file(fUrl.Path().String(), B_READ_ONLY);
		status_t error = file.InitCheck();
		if (error != B_OK)
			return error;

		BNodeInfo info(&file);
		char mimeType[B_MIME_TYPE_LENGTH + 1];
		if (info.GetType(mimeType) != B_OK)
			update_mime_info(fUrl.Path().String(), false, true, false);
		if (info.GetType(mimeType) == B_OK)
			fResult.SetContentType(mimeType);

		// Send all notifications to listener, if any
		if (fListener != NULL) {
			fListener->ConnectionOpened(this);

			off_t size = 0;
			error = file.GetSize(&size);
			if (error != B_OK)
				return error;
			fResult.SetLength(size);

			fListener->HeadersReceived(this, fResult);

			ssize_t chunkSize = 0;
			char chunk[4096];
			while (!fQuit) {
				chunkSize = file.Read(chunk, sizeof(chunk));
				if (chunkSize > 0) {
					fListener->DataReceived(this, chunk, transferredSize,
						chunkSize);
					transferredSize += chunkSize;
					fListener->DownloadProgress(this, transferredSize, size);
				} else
					break;
			}
			if (fQuit)
				return B_INTERRUPTED;
			// Return error if we didn't transfer everything
			if (transferredSize != size) {
				if (chunkSize < 0)
					return (status_t)chunkSize;
				else
					return B_IO_ERROR;
			}
		}

		return B_OK;
	}

	node_ref ref;
	status_t error = node.GetNodeRef(&ref);

	// Stop here, and don't hit the assert below, if the file doesn't exist.
	if (error != B_OK)
		return error;

	assert(node.IsDirectory());
	BDirectory directory(&ref);

	fResult.SetContentType("application/x-ftp-directory; charset=utf-8");
		// This tells WebKit to use its FTP directory rendering code.

	if (fListener != NULL) {
		fListener->ConnectionOpened(this);
		fListener->HeadersReceived(this, fResult);

		// Add a parent directory entry.
		fListener->DataReceived(this, "+/,\t..\r\n", transferredSize, 8);
		fListener->DownloadProgress(this, transferredSize, 0);
		transferredSize += 8;
	}

	char name[B_FILE_NAME_LENGTH];
	BEntry entry;
	while (!fQuit && directory.GetNextEntry(&entry) != B_ENTRY_NOT_FOUND) {
		// We read directories using the EPLF (Easily Parsed List Format)
		// This happens to be one of the formats that WebKit can understand,
		// and it is not too hard to parse or generate.
		// http://tools.ietf.org/html/draft-bernstein-eplf-02
		BString eplf("+");
		if (entry.IsFile() || entry.IsSymLink()) {
			eplf += "r,";
			off_t fileSize;
			if (entry.GetSize(&fileSize) == B_OK)
				eplf << "s" << fileSize << ",";

		} else if (entry.IsDirectory())
			eplf += "/,";

		time_t modification;
		if (entry.GetModificationTime(&modification) == B_OK)
			eplf << "m" << modification << ",";

		mode_t permissions;
		if (entry.GetPermissions(&permissions) == B_OK)
			eplf << "up" << BString().SetToFormat("%03o", permissions) << ",";

		node_ref ref;
		if (entry.GetNodeRef(&ref) == B_OK)
			eplf << "i" << ref.device << "." << ref.node << ",";

		entry.GetName(name);
		eplf << "\t" << name << "\r\n";
		if (fListener != NULL) {
			fListener->DataReceived(this, eplf.String(), transferredSize,
				eplf.Length());
			fListener->DownloadProgress(this, transferredSize, 0);
		}
		transferredSize += eplf.Length();
	}

	if (!fQuit)
		fResult.SetLength(transferredSize);

	return fQuit ? B_INTERRUPTED : B_OK;
}
