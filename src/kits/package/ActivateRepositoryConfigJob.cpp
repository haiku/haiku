/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <zooey@hirschkaefer.de>
 */


#include <package/ActivateRepositoryConfigJob.h>

#include <File.h>
#include <Message.h>

#include <AutoDeleter.h>

#include <package/Context.h>
#include <package/RepositoryConfig.h>
#include <package/RepositoryHeader.h>


namespace BPackageKit {

namespace BPrivate {


ActivateRepositoryConfigJob::ActivateRepositoryConfigJob(
	const BContext& context, const BString& title,
	const BEntry& archivedRepoHeaderEntry, const BString& repositoryBaseURL,
	const BDirectory& targetDirectory)
	:
	inherited(context, title),
	fArchivedRepoHeaderEntry(archivedRepoHeaderEntry),
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
	BFile archiveFile(&fArchivedRepoHeaderEntry, B_READ_ONLY);
	status_t result = archiveFile.InitCheck();
	if (result != B_OK)
		return result;

	BMessage archive;
	if ((result = archive.Unflatten(&archiveFile)) != B_OK)
		return result;

	BRepositoryHeader* repoHeader = BRepositoryHeader::Instantiate(&archive);
	if (repoHeader == NULL)
		return B_BAD_DATA;
	ObjectDeleter<BRepositoryHeader> repoHeaderDeleter(repoHeader);
	if ((result = repoHeader->InitCheck()) != B_OK)
		return result;

	fTargetEntry.SetTo(&fTargetDirectory, repoHeader->Name().String());
	if (fTargetEntry.Exists()) {
		BString description = BString("A repository configuration for ")
			<< repoHeader->Name() << " already exists.";
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
	repoConfig.SetName(repoHeader->Name());
	repoConfig.SetBaseURL(fRepositoryBaseURL);
	repoConfig.SetPriority(repoHeader->Priority());
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
