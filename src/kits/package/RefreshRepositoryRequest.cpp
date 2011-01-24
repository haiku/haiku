/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <zooey@hirschkaefer.de>
 */


#include <package/RefreshRepositoryRequest.h>

#include <Directory.h>
#include <Path.h>

#include <package/ActivateRepositoryCacheJob.h>
#include <package/ChecksumAccessors.h>
#include <package/ValidateChecksumJob.h>
#include <package/FetchFileJob.h>
#include <package/JobQueue.h>
#include <package/RepositoryCache.h>
#include <package/RepositoryConfig.h>
#include <package/Roster.h>


namespace Haiku {

namespace Package {


using namespace Private;


RefreshRepositoryRequest::RefreshRepositoryRequest(const Context& context,
	const RepositoryConfig& repoConfig)
	:
	inherited(context),
	fRepoConfig(repoConfig)
{
}


RefreshRepositoryRequest::~RefreshRepositoryRequest()
{
}


status_t
RefreshRepositoryRequest::CreateInitialJobs()
{
	Roster roster;
	status_t result = fRepoConfig.InitCheck();
	if (result != B_OK)
		return result;

	// fetch the current checksum and compare with our cache's checksum,
	// if they differ, fetch the updated cache
	fFetchedChecksumFile
		= fContext.GetTempfileManager().Create("repochecksum-");
	BString repoChecksumURL
		= BString(fRepoConfig.URL()) << "/" << "repo.sha256";
	FetchFileJob* fetchChecksumJob = new (std::nothrow) FetchFileJob(
		fContext,
		BString("Fetching repository checksum from ") << fRepoConfig.URL(),
		repoChecksumURL, fFetchedChecksumFile);
	if (fetchChecksumJob == NULL)
		return B_NO_MEMORY;
	if ((result = QueueJob(fetchChecksumJob)) != B_OK) {
		delete fetchChecksumJob;
		return result;
	}

	RepositoryCache repoCache;
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
RefreshRepositoryRequest::JobSucceeded(Job* job)
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
RefreshRepositoryRequest::_FetchRepositoryCache()
{
	// download repository cache and put it in either the common/user cache
	// path, depending on where the corresponding repo-config lives

	// job fetching the cache
	BEntry tempRepoCache = fContext.GetTempfileManager().Create("repocache-");
	BString repoCacheURL = BString(fRepoConfig.URL()) << "/" << "repo";
	FetchFileJob* fetchCacheJob = new (std::nothrow) FetchFileJob(fContext,
		BString("Fetching repository-cache from ") << fRepoConfig.URL(),
		repoCacheURL, tempRepoCache);
	if (fetchCacheJob == NULL)
		return B_NO_MEMORY;
	status_t result = QueueJob(fetchCacheJob);
	if (result != B_OK) {
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
	Roster roster;
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


}	// namespace Package

}	// namespace Haiku
