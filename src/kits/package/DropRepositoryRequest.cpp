/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <zooey@hirschkaefer.de>
 */


#include <package/DropRepositoryRequest.h>

#include <Directory.h>
#include <Path.h>

#include <package/RemoveRepositoryJob.h>
#include <package/JobQueue.h>


namespace BPackageKit {


using namespace BPrivate;


DropRepositoryRequest::DropRepositoryRequest(const BContext& context,
	const BString& repositoryName)
	:
	inherited(context),
	fRepositoryName(repositoryName)
{
}


DropRepositoryRequest::~DropRepositoryRequest()
{
}


status_t
DropRepositoryRequest::CreateInitialJobs()
{
	status_t result = InitCheck();
	if (result != B_OK)
		return B_NO_INIT;

	RemoveRepositoryJob* removeRepoJob
		= new (std::nothrow) RemoveRepositoryJob(fContext,
			BString("Removing repository ") << fRepositoryName,
			fRepositoryName);
	if (removeRepoJob == NULL)
		return B_NO_MEMORY;
	if ((result = QueueJob(removeRepoJob)) != B_OK) {
		delete removeRepoJob;
		return result;
	}

	return B_OK;
}


}	// namespace BPackageKit
