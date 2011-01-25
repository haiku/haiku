/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <zooey@hirschkaefer.de>
 */


#include <package/ActivateRepositoryCacheJob.h>

#include <File.h>

#include <package/Context.h>


namespace BPackageKit {

namespace BPrivate {


ActivateRepositoryCacheJob::ActivateRepositoryCacheJob(const BContext& context,
	const BString& title, const BEntry& fetchedRepoCacheEntry,
	const BString& repositoryName, const BDirectory& targetDirectory)
	:
	inherited(context, title),
	fFetchedRepoCacheEntry(fetchedRepoCacheEntry),
	fRepositoryName(repositoryName),
	fTargetDirectory(targetDirectory)
{
}


ActivateRepositoryCacheJob::~ActivateRepositoryCacheJob()
{
}


status_t
ActivateRepositoryCacheJob::Execute()
{
	status_t result = fFetchedRepoCacheEntry.MoveTo(&fTargetDirectory,
		fRepositoryName.String(), true);
	if (result != B_OK)
		return result;

	// TODO: propagate some repository attributes to file attributes

	return B_OK;
}


}	// namespace BPrivate

}	// namespace BPackageKit
