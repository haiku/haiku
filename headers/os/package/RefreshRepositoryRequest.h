/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _HAIKU__PACKAGE__REFRESH_REPOSITORY_REQUEST_H_
#define _HAIKU__PACKAGE__REFRESH_REPOSITORY_REQUEST_H_


#include <Entry.h>
#include <String.h>

#include <package/Context.h>
#include <package/RepositoryConfig.h>
#include <package/Request.h>


namespace Haiku {

namespace Package {


namespace Private {
	class ValidateChecksumJob;
}
using Private::ValidateChecksumJob;

class RefreshRepositoryRequest : public Request {
	typedef	Request				inherited;

public:
								RefreshRepositoryRequest(const Context& context,
									const RepositoryConfig& repoConfig);
	virtual						~RefreshRepositoryRequest();

	virtual	status_t			CreateInitialJobs();

protected:
								// JobStateListener
	virtual	void				JobSucceeded(Job* job);

private:
			status_t			_FetchRepositoryCache();

			BEntry				fFetchedChecksumFile;
			RepositoryConfig	fRepoConfig;

			ValidateChecksumJob*	fValidateChecksumJob;
};


}	// namespace Package

}	// namespace Haiku


#endif // _HAIKU__PACKAGE__REFRESH_REPOSITORY_REQUEST_H_
