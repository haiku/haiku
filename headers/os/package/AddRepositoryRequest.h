/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _HAIKU__PACKAGE__ADD_REPOSITORY_REQUEST_H_
#define _HAIKU__PACKAGE__ADD_REPOSITORY_REQUEST_H_


#include <String.h>

#include <package/Context.h>
#include <package/Request.h>


namespace Haiku {

namespace Package {


namespace Private {
	class ActivateRepositoryConfigJob;
}
using Private::ActivateRepositoryConfigJob;


class AddRepositoryRequest : public Request {
	typedef	Request				inherited;

public:
								AddRepositoryRequest(const Context& context,
									const BString& repositoryBaseURL,
									bool asUserRepository);
	virtual						~AddRepositoryRequest();

	virtual	status_t			CreateInitialJobs();

			const BString&		RepositoryName() const;

protected:
								// JobStateListener
	virtual	void				JobSucceeded(Job* job);

private:
			BString				fRepositoryBaseURL;
			bool				fAsUserRepository;

			ActivateRepositoryConfigJob*	fActivateJob;

			BString				fRepositoryName;
};


}	// namespace Package

}	// namespace Haiku


#endif // _HAIKU__PACKAGE__ADD_REPOSITORY_REQUEST_H_
