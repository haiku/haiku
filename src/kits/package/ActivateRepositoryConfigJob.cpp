/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <zooey@hirschkaefer.de>
 */


#include <package/ActivateRepositoryConfigJob.h>

#include <package/Context.h>
#include <package/RepositoryConfig.h>
#include <package/RepositoryInfo.h>


namespace BPackageKit {

namespace BPrivate {


ActivateRepositoryConfigJob::ActivateRepositoryConfigJob(
	const BContext& context, const BString& title,
	const BEntry& archivedRepoInfoEntry, const BString& repositoryBaseURL,
	const BDirectory& targetDirectory)
	:
	inherited(context, title),
	fArchivedRepoInfoEntry(archivedRepoInfoEntry),
	fRepositoryBaseURL(repositoryBaseURL),
	fTargetDirectory(targetDirectory)
{
}


ActivateRepositoryConfigJob::~ActivateRepositoryConfigJob()
{
}


status_t
ActivateRepositoryConfigJob::Execute()
{
	BRepositoryInfo repoInfo(fArchivedRepoInfoEntry);
	status_t result = repoInfo.InitCheck();
	if (result != B_OK)
		return result;

	fTargetEntry.SetTo(&fTargetDirectory, repoInfo.Name().String());
	if (fTargetEntry.Exists()) {
		BString description = BString("A repository configuration for ")
			<< repoInfo.Name() << " already exists.";
		BString question("overwrite?");
		bool yes = fContext.DecisionProvider().YesNoDecisionNeeded(
			description, question, "yes", "no", "no");
		if (!yes) {
			fTargetEntry.Unset();
			return B_CANCELED;
		}
	}

	// create and store the configuration (injecting the BaseURL that was
	// actually used)
	BRepositoryConfig repoConfig;
	repoConfig.SetName(repoInfo.Name());
	repoConfig.SetBaseURL(fRepositoryBaseURL);
	repoConfig.SetPriority(repoInfo.Priority());
	if ((result = repoConfig.Store(fTargetEntry)) != B_OK)
		return result;

	// store name of activated repository as result
	fRepositoryName = repoConfig.Name();

	return B_OK;
}


void
ActivateRepositoryConfigJob::Cleanup(status_t jobResult)
{
	if (jobResult != B_OK && State() != JOB_STATE_ABORTED
		&& fTargetEntry.InitCheck() == B_OK)
		fTargetEntry.Remove();
}


const BString&
ActivateRepositoryConfigJob::RepositoryName() const
{
	return fRepositoryName;
}


}	// namespace BPrivate

}	// namespace BPackageKit
