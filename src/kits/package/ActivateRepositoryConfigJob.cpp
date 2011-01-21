#include <stdio.h>
#include <Path.h>
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

#include <package/RepositoryConfig.h>


namespace Haiku {

namespace Package {


ActivateRepositoryConfigJob::ActivateRepositoryConfigJob(const BString& title,
	const BEntry& archivedRepoConfigEntry, const BDirectory& targetDirectory)
	:
	inherited(title),
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
BPath p;
fArchivedRepoConfigEntry.GetPath(&p);
printf("Execute(): arce=%s\n", p.Path());
	status_t result = archiveFile.InitCheck();
	if (result != B_OK)
		return result;

printf("Execute(): 2\n");
	BMessage archive;
	if ((result = archive.Unflatten(&archiveFile)) != B_OK)
		return result;

printf("Execute(): 3\n");
	RepositoryConfig* repoConfig = RepositoryConfig::Instantiate(&archive);
	if (repoConfig == NULL)
		return B_BAD_DATA;
printf("Execute(): 4\n");
	if ((result = repoConfig->InitCheck()) != B_OK)
		return result;

printf("Execute(): 5\n");
	fTargetEntry.SetTo(&fTargetDirectory, repoConfig->Name().String());
	if (fTargetEntry.Exists()) {
		// TODO: ask user whether to clobber or not
printf("Execute(): 5b\n");
		return B_INTERRUPTED;
	}

printf("Execute(): 6\n");
	if ((result = repoConfig->StoreAsConfigFile(fTargetEntry)) != B_OK)
		return result;

printf("Execute(): 7\n");
	return B_OK;
}


void
ActivateRepositoryConfigJob::Cleanup(status_t jobResult)
{
	if (jobResult != B_OK && State() != JOB_STATE_ABORTED)
		fTargetEntry.Remove();
}


}	// namespace Package

}	// namespace Haiku
