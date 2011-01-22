/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <zooey@hirschkaefer.de>
 */


#include <package/ActivateRepositoryConfigJob.h>

#include <stdlib.h>

#include <File.h>

#include <package/Context.h>
#include <package/RepositoryConfig.h>


namespace Haiku {

namespace Package {


ActivateRepositoryConfigJob::ActivateRepositoryConfigJob(const Context& context,
	const BString& title, const BEntry& archivedRepoConfigEntry,
	const BDirectory& targetDirectory)
	:
	inherited(context, title),
	fArchivedRepoConfigEntry(archivedRepoConfigEntry),
	fTargetDirectory(targetDirectory)
{
}


ActivateRepositoryConfigJob::~ActivateRepositoryConfigJob()
{
}


status_t
ActivateRepositoryConfigJob::Execute()
{
	BFile archiveFile(&fArchivedRepoConfigEntry, B_READ_ONLY);
	status_t result = archiveFile.InitCheck();
	if (result != B_OK)
		return result;

	BMessage archive;
	if ((result = archive.Unflatten(&archiveFile)) != B_OK)
		return result;

	RepositoryConfig* repoConfig = RepositoryConfig::Instantiate(&archive);
	if (repoConfig == NULL)
		return B_BAD_DATA;
	if ((result = repoConfig->InitCheck()) != B_OK)
		return result;

	fTargetEntry.SetTo(&fTargetDirectory, repoConfig->Name().String());
	if (fTargetEntry.Exists()) {
		BString description = BString("A repository configuration for ")
			<< repoConfig->Name() << " already exists.";
		BString question("overwrite?");
		bool yes = fContext.GetDecisionProvider().YesNoDecisionNeeded(
			description, question, "yes", "no", "no");
		if (!yes) {
			fTargetEntry.Unset();
			return B_CANCELED;
		}
	}

	if ((result = repoConfig->StoreAsConfigFile(fTargetEntry)) != B_OK)
		return result;

	return B_OK;
}


void
ActivateRepositoryConfigJob::Cleanup(status_t jobResult)
{
	if (jobResult != B_OK && State() != JOB_STATE_ABORTED
		&& fTargetEntry.InitCheck() == B_OK)
		fTargetEntry.Remove();
}


}	// namespace Package

}	// namespace Haiku
