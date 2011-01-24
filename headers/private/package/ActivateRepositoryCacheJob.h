/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef _HAIKU__PACKAGE__PRIVATE__ACTIVATE_REPOSITORY_CACHE_JOB_H_
#define _HAIKU__PACKAGE__PRIVATE__ACTIVATE_REPOSITORY_CACHE_JOB_H_


#include <Directory.h>
#include <Entry.h>
#include <String.h>

#include <package/Job.h>


namespace Haiku {

namespace Package {

namespace Private {


class ActivateRepositoryCacheJob : public Job {
	typedef	Job					inherited;

public:
								ActivateRepositoryCacheJob(
									const Context& context,
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


}	// namespace Private

}	// namespace Package

}	// namespace Haiku


#endif // _HAIKU__PACKAGE__PRIVATE__ACTIVATE_REPOSITORY_CACHE_JOB_H_
