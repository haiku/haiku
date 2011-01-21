/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <zooey@hirschkaefer.de>
 */


#include <package/AddRepositoryRequest.h>

#include <Directory.h>
#include <Path.h>

#include <package/ActivateRepositoryConfigJob.h>
#include <package/FetchFileJob.h>
#include <package/JobQueue.h>
#include <package/Roster.h>


namespace Haiku {

namespace Package {


AddRepositoryRequest::AddRepositoryRequest(const Context& context,
	const BString& repositoryURL, bool asUserRepository)
	:
	inherited(context),
	fRepositoryURL(repositoryURL),
	fAsUserRepository(asUserRepository)
{
}


AddRepositoryRequest::~AddRepositoryRequest()
{
}


status_t
AddRepositoryRequest::CreateJobsToRun(JobQueue& jobQueue)
{
	BEntry tempEntry
		= GetContext().GetTempEntryManager().Create("repoconfig-");
	FetchFileJob* fetchJob = new (std::nothrow) FetchFileJob(
		BString("Fetching repository-config from ") << fRepositoryURL,
		fRepositoryURL, tempEntry);
	if (fetchJob == NULL)
		return B_NO_MEMORY;
	status_t result = QueueJob(fetchJob, jobQueue);
	if (result != B_OK) {
		delete fetchJob;
		return result;
	}

	Roster roster;
	BPath targetRepoConfigPath;
	result = fAsUserRepository
		? roster.GetUserRepositoryConfigPath(&targetRepoConfigPath, true)
		: roster.GetCommonRepositoryConfigPath(&targetRepoConfigPath, true);
	if (result != B_OK)
		return result;
	BDirectory targetDirectory(targetRepoConfigPath.Path());
	ActivateRepositoryConfigJob* activateJob
		= new (std::nothrow) ActivateRepositoryConfigJob(
			BString("Activating repository-config from ") << fRepositoryURL,
			tempEntry, targetDirectory);
	if (activateJob == NULL)
		return B_NO_MEMORY;
	activateJob->AddDependency(fetchJob);
	if ((result = QueueJob(activateJob, jobQueue)) != B_OK) {
		delete activateJob;
		return result;
	}

	return B_OK;
}


}	// namespace Package

}	// namespace Haiku
