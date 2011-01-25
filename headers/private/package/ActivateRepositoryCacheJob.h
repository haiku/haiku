/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__PRIVATE__ACTIVATE_REPOSITORY_CACHE_JOB_H_
#define _PACKAGE__PRIVATE__ACTIVATE_REPOSITORY_CACHE_JOB_H_


#include <Directory.h>
#include <Entry.h>
#include <String.h>

#include <package/Job.h>


namespace BPackageKit {

namespace BPrivate {


class ActivateRepositoryCacheJob : public BJob {
	typedef	BJob				inherited;

public:
								ActivateRepositoryCacheJob(
									const BContext& context,
									const BString& title,
									const BEntry& fetchedRepoCacheEntry,
									const BString& repositoryName,
									const BDirectory& targetDirectory);
	virtual						~ActivateRepositoryCacheJob();

protected:
	virtual	status_t			Execute();

private:
			BEntry				fFetchedRepoCacheEntry;
			BString				fRepositoryName;
			BDirectory			fTargetDirectory;
};


}	// namespace BPrivate

}	// namespace BPackageKit


#endif // _PACKAGE__PRIVATE__ACTIVATE_REPOSITORY_CACHE_JOB_H_
