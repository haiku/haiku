/*
 * Copyright 2011-2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <zooey@hirschkaefer.de>
 */


#include <package/RefreshRepositoryRequest.h>

#include <Directory.h>
#include <Path.h>

#include <JobQueue.h>

#include <package/ActivateRepositoryCacheJob.h>
#include <package/ChecksumAccessors.h>
#include <package/ValidateChecksumJob.h>
#include <package/RepositoryCache.h>
#include <package/RepositoryConfig.h>
#include <package/PackageRoster.h>

#include "FetchFileJob.h"


namespace BPackageKit {


using namespace BPrivate;


BRefreshRepositoryRequest::BRefreshRepositoryRequest(const BContext& context,
	const BRepositoryConfig& repoConfig)
	:
	inherited(context),
	fRepoConfig(repoConfig)
{
}


BRefreshRepositoryRequest::~BRefreshRepositoryRequest()
{
}


status_t
BRefreshRepositoryRequest::CreateInitialJobs()
{
	status_t result = InitCheck();
	if (result != B_OK)
		return B_NO_INIT;

	if ((result = fRepoConfig.InitCheck()) != B_OK)
		return result;

	// fetch the current checksum and compare with our cache's checksum,
	// if they differ, fetch the updated cache
	result = fContext.GetNewTempfile("repochecksum-", &fFetchedChecksumFile);
	if (result != B_OK)
		return result;
	BString repoChecksumURL
		= BString(fRepoConfig.BaseURL()) << "/" << "repo.sha256";
	FetchFileJob* fetchChecksumJob = new (std::nothrow) FetchFileJob(
		fContext,
		BString("Fetching repository checksum from ") << fRepoConfig.BaseURL(),
		repoChecksumURL, fFetchedChecksumFile);
	if (fetchChecksumJob == NULL)
		return B_NO_MEMORY;
	if ((result = QueueJob(fetchChecksumJob)) != B_OK) {
		delete fetchChecksumJob;
		return result;
	}

	BRepositoryCache repoCache;
	BPackageRoster roster;
	roster.GetRepositoryCache(fRepoConfig.Name(), &repoCache);

	ValidateChecksumJob* validateChecksumJob
		= new (std::nothrow) ValidateChecksumJob(fContext,
			BString("Validating checksum for ") << fRepoConfig.Name(),
			new (std::nothrow) ChecksumFileChecksumAccessor(
				fFetchedChecksumFile),
			new (std::nothrow) GeneralFileChecksumAccessor(repoCache.Entry(),
				true),
			false);
	if (validateChecksumJob == NULL)
		return B_NO_MEMORY;
	validateChecksumJob->AddDependency(fetchChecksumJob);
	if ((result = QueueJob(validateChecksumJob)) != B_OK) {
		delete validateChecksumJob;
		return result;
	}
	fValidateChecksumJob = validateChecksumJob;

	return B_OK;
}


void
BRefreshRepositoryRequest::JobSucceeded(BSupportKit::BJob* job)
{
	if (job == fValidateChecksumJob
		&& !fValidateChecksumJob->ChecksumsMatch()) {
		// the remote repo cache has a different checksum, we fetch it
		fValidateChecksumJob = NULL;
			// don't re-trigger fetching if anything goes wrong, fail instead
		_FetchRepositoryCache();
	}
}


status_t
BRefreshRepositoryRequest::_FetchRepositoryCache()
{
	// download repository cache and put it in either the common/user cache
	// path, depending on where the corresponding repo-config lives

	// job fetching the cache
	BEntry tempRepoCache;
	status_t result = fContext.GetNewTempfile("repocache-", &tempRepoCache);
	if (result != B_OK)
		return result;
	BString repoCacheURL = BString(fRepoConfig.BaseURL()) << "/" << "repo";
	FetchFileJob* fetchCacheJob = new (std::nothrow) FetchFileJob(fContext,
		BString("Fetching repository-cache from ") << fRepoConfig.BaseURL(),
		repoCacheURL, tempRepoCache);
	if (fetchCacheJob == NULL)
		return B_NO_MEMORY;
	if ((result = QueueJob(fetchCacheJob)) != B_OK) {
		delete fetchCacheJob;
		return result;
	}

	// job validating the cache's checksum
	ValidateChecksumJob* validateChecksumJob
		= new (std::nothrow) ValidateChecksumJob(fContext,
			BString("Validating checksum for ") << fRepoConfig.Name(),
			new (std::nothrow) ChecksumFileChecksumAccessor(
				fFetchedChecksumFile),
			new (std::nothrow) GeneralFileChecksumAccessor(tempRepoCache));
	if (validateChecksumJob == NULL)
		return B_NO_MEMORY;
	validateChecksumJob->AddDependency(fetchCacheJob);
	if ((result = QueueJob(validateChecksumJob)) != B_OK) {
		delete validateChecksumJob;
		return result;
	}

	// job activating the cache
	BPath targetRepoCachePath;
	BPackageRoster roster;
	result = fRepoConfig.IsUserSpecific()
		? roster.GetUserRepositoryCachePath(&targetRepoCachePath, true)
		: roster.GetCommonRepositoryCachePath(&targetRepoCachePath, true);
	if (result != B_OK)
		return result;
	BDirectory targetDirectory(targetRepoCachePath.Path());
	ActivateRepositoryCacheJob* activateJob
		= new (std::nothrow) ActivateRepositoryCacheJob(fContext,
			BString("Activating repository cache for ") << fRepoConfig.Name(),
			tempRepoCache, fRepoConfig.Name(), targetDirectory);
	if (activateJob == NULL)
		return B_NO_MEMORY;
	activateJob->AddDependency(validateChecksumJob);
	if ((result = QueueJob(activateJob)) != B_OK) {
		delete activateJob;
		return result;
	}

	return B_OK;
}


}	// namespace BPackageKit
