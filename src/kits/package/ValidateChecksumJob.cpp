/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <zooey@hirschkaefer.de>
 */


#include <package/ValidateChecksumJob.h>

#include <File.h>

#include <package/Context.h>


namespace BPackageKit {

namespace BPrivate {


ValidateChecksumJob::ValidateChecksumJob(const BContext& context,
	const BString& title, ChecksumAccessor* expectedChecksumAccessor,
	ChecksumAccessor* realChecksumAccessor, bool failIfChecksumsDontMatch)
	:
	inherited(context, title),
	fExpectedChecksumAccessor(expectedChecksumAccessor),
	fRealChecksumAccessor(realChecksumAccessor),
	fFailIfChecksumsDontMatch(failIfChecksumsDontMatch),
	fChecksumsMatch(false)
{
}


ValidateChecksumJob::~ValidateChecksumJob()
{
	delete fRealChecksumAccessor;
	delete fExpectedChecksumAccessor;
}


status_t
ValidateChecksumJob::Execute()
{
	if (fExpectedChecksumAccessor == NULL || fRealChecksumAccessor == NULL)
		return B_BAD_VALUE;

	BString expectedChecksum;
	BString realChecksum;

	status_t result = fExpectedChecksumAccessor->GetChecksum(expectedChecksum);
	if (result != B_OK)
		return result;

	result = fRealChecksumAccessor->GetChecksum(realChecksum);
	if (result != B_OK)
		return result;

	fChecksumsMatch = expectedChecksum.ICompare(realChecksum) == 0;

	if (fFailIfChecksumsDontMatch && !fChecksumsMatch) {
		BString error = BString("Checksum error:\n")
			<< "expected '"	<< expectedChecksum << "'\n"
			<< "got      '" << realChecksum << "'";
		SetErrorString(error);
		return B_BAD_DATA;
	}

	return B_OK;
}


bool
ValidateChecksumJob::ChecksumsMatch() const
{
	return fChecksumsMatch;
}


}	// namespace BPrivate

}	// namespace BPackageKit
