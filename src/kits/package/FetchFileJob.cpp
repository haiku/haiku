/*
 * Copyright 2011-2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler <axeld@pinc-software.de>
 *		Rene Gollent <rene@gollent.com>
 *		Oliver Tappe <zooey@hirschkaefer.de>
 */


#include "FetchFileJob.h"

#include <stdio.h>
#include <sys/wait.h>

#include <Path.h>

#ifdef HAIKU_TARGET_PLATFORM_HAIKU
#	include <UrlRequest.h>
#	include <UrlProtocolRoster.h>
#endif


namespace BPackageKit {

namespace BPrivate {


#ifdef HAIKU_TARGET_PLATFORM_HAIKU

FetchFileJob::FetchFileJob(const BContext& context, const BString& title,
	const BString& fileURL, const BEntry& targetEntry)
	:
	inherited(context, title),
	fFileURL(fileURL),
	fTargetEntry(targetEntry),
	fTargetFile(&targetEntry, B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY),
	fDownloadProgress(0.0)
{
}


FetchFileJob::~FetchFileJob()
{
}


float
FetchFileJob::DownloadProgress() const
{
	return fDownloadProgress;
}


const char*
FetchFileJob::DownloadURL() const
{
	return fFileURL.String();
}


const char*
FetchFileJob::DownloadFileName() const
{
	return fTargetEntry.Name();
}


off_t
FetchFileJob::DownloadBytes() const
{
	return fBytes;
}


off_t
FetchFileJob::DownloadTotalBytes() const
{
	return fTotalBytes;
}


status_t
FetchFileJob::Execute()
{
	status_t result = fTargetFile.InitCheck();
	if (result != B_OK)
		return result;

	BUrlRequest* request = BUrlProtocolRoster::MakeRequest(fFileURL.String(),
		this);
	if (request == NULL)
		return B_BAD_VALUE;

	thread_id thread = request->Run();
	wait_for_thread(thread, NULL);

	if (fSuccess == true)
		return B_OK;
	else
		return B_ERROR;

	//TODO: More detailed error codes?
#if 0
	const BHttpResult& outResult = dynamic_cast<const BHttpResult&>
		(request->Result());

	switch (outResult.StatusCode()) {
		case B_HTTP_STATUS_OK:
			return B_OK;
		case B_HTTP_STATUS_PARTIAL_CONTENT:
			return B_PARTIAL_READ;
		case B_HTTP_STATUS_REQUEST_TIMEOUT:
		case B_HTTP_STATUS_GATEWAY_TIMEOUT:
			return B_TIMED_OUT;
		case B_HTTP_STATUS_NOT_IMPLEMENTED:
		case B_HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE:
			return B_NOT_SUPPORTED;
		case B_HTTP_STATUS_UNAUTHORIZED:
			return B_PERMISSION_DENIED;
		case B_HTTP_STATUS_FORBIDDEN:
		case B_HTTP_STATUS_METHOD_NOT_ALLOWED:
		case B_HTTP_STATUS_NOT_ACCEPTABLE:
			return B_NOT_ALLOWED;
		case B_HTTP_STATUS_NOT_FOUND:
			return B_NAME_NOT_FOUND;
		case B_HTTP_STATUS_BAD_GATEWAY:
			return B_BAD_DATA;
		default:
			return B_ERROR;
	}
#endif
}


void
FetchFileJob::DataReceived(BUrlRequest*, const char* data, off_t position,
	ssize_t size)
{
	fTargetFile.WriteAt(position, data, size);
}


void
FetchFileJob::DownloadProgress(BUrlRequest*, ssize_t bytesReceived,
	ssize_t bytesTotal)
{
	if (bytesTotal != 0) {
		fBytes = bytesReceived;
		fTotalBytes = bytesTotal;
		fDownloadProgress = (float)bytesReceived/bytesTotal;
		NotifyStateListeners();
	}
}


void
FetchFileJob::RequestCompleted(BUrlRequest*, bool success)
{
	fSuccess = success;
}


void
FetchFileJob::Cleanup(status_t jobResult)
{
	if (jobResult != B_OK)
		fTargetEntry.Remove();
}


#else // HAIKU_TARGET_PLATFORM_HAIKU


FetchFileJob::FetchFileJob(const BContext& context, const BString& title,
	const BString& fileURL, const BEntry& targetEntry)
	:
	inherited(context, title),
	fFileURL(fileURL),
	fTargetEntry(targetEntry),
	fTargetFile(&targetEntry, B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY),
	fDownloadProgress(0.0)
{
}


FetchFileJob::~FetchFileJob()
{
}


float
FetchFileJob::DownloadProgress() const
{
	return fDownloadProgress;
}


const char*
FetchFileJob::DownloadURL() const
{
	return fFileURL.String();
}


const char*
FetchFileJob::DownloadFileName() const
{
	return fTargetEntry.Name();
}


off_t
FetchFileJob::DownloadBytes() const
{
	return fBytes;
}


off_t
FetchFileJob::DownloadTotalBytes() const
{
	return fTotalBytes;
}


status_t
FetchFileJob::Execute()
{
	return B_UNSUPPORTED;
}


void
FetchFileJob::Cleanup(status_t jobResult)
{
	if (jobResult != B_OK)
		fTargetEntry.Remove();
}


#endif // HAIKU_TARGET_PLATFORM_HAIKU

}	// namespace BPrivate

}	// namespace BPackageKit
