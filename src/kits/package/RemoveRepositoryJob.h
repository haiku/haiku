/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__PRIVATE__REMOVE_REPOSITORY_JOB_H_
#define _PACKAGE__PRIVATE__REMOVE_REPOSITORY_JOB_H_


#include <String.h>

#include <package/Job.h>


namespace BPackageKit {

namespace BPrivate {


class RemoveRepositoryJob : public BJob {
	typedef	BJob				inherited;

public:
								RemoveRepositoryJob(
									const BContext& context,
									const BString& title,
									const BString& repositoryName);
	virtual						~RemoveRepositoryJob();

protected:
	virtual	status_t			Execute();

private:
			BString				fRepositoryName;
};


}	// namespace BPrivate

}	// namespace BPackageKit


#endif // _PACKAGE__PRIVATE__REMOVE_REPOSITORY_JOB_H_
