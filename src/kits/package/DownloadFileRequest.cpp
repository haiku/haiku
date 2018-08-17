/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include <package/DownloadFileRequest.h>

#include <package/ValidateChecksumJob.h>

#include "FetchFileJob.h"


namespace BPackageKit {


using namespace BPrivate;


DownloadFileRequest::DownloadFileRequest(const BContext& context,
	const BString& fileURL, const BEntry& targetEntry, const BString& checksum)
	:
	inherited(context),
	fFileURL(fileURL),
	fTargetEntry(targetEntry),
	fChecksum(checksum)
{
	if (fInitStatus == B_OK) {
		if (fFileURL.IsEmpty())
			fInitStatus = B_BAD_VALUE;
		else
			fInitStatus = targetEntry.InitCheck();
	}
}


DownloadFileRequest::~DownloadFileRequest()
{
}


status_t
DownloadFileRequest::CreateInitialJobs()
{
	status_t error = InitCheck();
	if (error != B_OK)
		return B_NO_INIT;

	// create the download job
	FetchFileJob* fetchJob = new (std::nothrow) FetchFileJob(fContext,
		BString("Downloading ") << fFileURL, fFileURL, fTargetEntry);
	if (fetchJob == NULL)
		return B_NO_MEMORY;

	if ((error = QueueJob(fetchJob)) != B_OK) {
		delete fetchJob;
		return error;
	}

	// create the checksum validation job
	if (fChecksum.IsEmpty())
		return B_OK;

	ValidateChecksumJob* validateJob = new (std::nothrow) ValidateChecksumJob(
		fContext, BString("Validating checksum for ") << fFileURL,
		new (std::nothrow) StringChecksumAccessor(fChecksum),
		new (std::nothrow) GeneralFileChecksumAccessor(fTargetEntry, true));

	if (validateJob == NULL)
		return B_NO_MEMORY;

	if ((error = QueueJob(validateJob)) != B_OK) {
		delete validateJob;
		return error;
	}

	return B_OK;
}


}	// namespace BPackageKit
