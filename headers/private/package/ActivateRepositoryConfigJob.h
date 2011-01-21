/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _HAIKU__PACKAGE__ACTIVATE_REPOSITORY_CONFIG_JOB_H_
#define _HAIKU__PACKAGE__ACTIVATE_REPOSITORY_CONFIG_JOB_H_


#include <Directory.h>
#include <Entry.h>
#include <String.h>

#include <package/Job.h>


namespace Haiku {

namespace Package {


class ActivateRepositoryConfigJob : public Job {
	typedef	Job					inherited;

public:
								ActivateRepositoryConfigJob(
									const BString& title,
									const BEntry& archivedRepoConfigEntry,
									const BDirectory& targetDirectory);
	virtual						~ActivateRepositoryConfigJob();

protected:
	virtual	status_t			Execute();
	virtual	void				Cleanup(status_t jobResult);

private:
			BEntry				fArchivedRepoConfigEntry;
			BDirectory			fTargetDirectory;
			BEntry				fTargetEntry;
};


}	// namespace Package

}	// namespace Haiku


#endif // _HAIKU__PACKAGE__ACTIVATE_REPOSITORY_CONFIG_JOB_H_
