/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__REFRESH_REPOSITORY_REQUEST_H_
#define _PACKAGE__REFRESH_REPOSITORY_REQUEST_H_


#include <Entry.h>
#include <String.h>

#include <package/Context.h>
#include <package/RepositoryConfig.h>
#include <package/Request.h>


namespace BPackageKit {


namespace BPrivate {
	class ValidateChecksumJob;
}
using BPrivate::ValidateChecksumJob;


class BRefreshRepositoryRequest : public BRequest {
	typedef	BRequest				inherited;

public:
								BRefreshRepositoryRequest(
									const BContext& context,
									const BRepositoryConfig& repoConfig);
	virtual						~BRefreshRepositoryRequest();

	virtual	status_t			CreateInitialJobs();

protected:
								// BJobStateListener
	virtual	void				JobSucceeded(BJob* job);

private:
			status_t			_FetchRepositoryCache();

			BEntry				fFetchedChecksumFile;
			BRepositoryConfig	fRepoConfig;

			ValidateChecksumJob*	fValidateChecksumJob;
};


}	// namespace BPackageKit


#endif // _PACKAGE__REFRESH_REPOSITORY_REQUEST_H_
