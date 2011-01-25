/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__DROP_REPOSITORY_REQUEST_H_
#define _PACKAGE__DROP_REPOSITORY_REQUEST_H_


#include <String.h>

#include <package/Context.h>
#include <package/Request.h>


namespace BPackageKit {


class DropRepositoryRequest : public BRequest {
	typedef	BRequest				inherited;

public:
								DropRepositoryRequest(const BContext& context,
									const BString& repositoryName);
	virtual						~DropRepositoryRequest();

	virtual	status_t			CreateInitialJobs();

private:
			BString				fRepositoryName;
};


}	// namespace BPackageKit


#endif // _PACKAGE__ADD_REPOSITORY_REQUEST_H_
