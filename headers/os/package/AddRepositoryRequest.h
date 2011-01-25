/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__ADD_REPOSITORY_REQUEST_H_
#define _PACKAGE__ADD_REPOSITORY_REQUEST_H_


#include <String.h>

#include <package/Context.h>
#include <package/Request.h>


namespace BPackageKit {


namespace BPrivate {
	class ActivateRepositoryConfigJob;
}
using BPrivate::ActivateRepositoryConfigJob;


class AddRepositoryRequest : public BRequest {
	typedef	BRequest				inherited;

public:
								AddRepositoryRequest(const BContext& context,
									const BString& repositoryBaseURL,
									bool asUserRepository);
	virtual						~AddRepositoryRequest();

	virtual	status_t			CreateInitialJobs();

			const BString&		RepositoryName() const;

protected:
								// BJobStateListener
	virtual	void				JobSucceeded(BJob* job);

private:
			BString				fRepositoryBaseURL;
			bool				fAsUserRepository;

			ActivateRepositoryConfigJob*	fActivateJob;

			BString				fRepositoryName;
};


}	// namespace BPackageKit


#endif // _PACKAGE__ADD_REPOSITORY_REQUEST_H_
