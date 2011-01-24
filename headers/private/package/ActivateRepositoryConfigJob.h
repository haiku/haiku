/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef _HAIKU__PACKAGE__PRIVATE__ACTIVATE_REPOSITORY_CONFIG_JOB_H_
#define _HAIKU__PACKAGE__PRIVATE__ACTIVATE_REPOSITORY_CONFIG_JOB_H_


#include <Directory.h>
#include <Entry.h>
#include <String.h>

#include <package/Job.h>


namespace Haiku {

namespace Package {

namespace Private {


class ActivateRepositoryConfigJob : public Job {
	typedef	Job					inherited;

public:
								ActivateRepositoryConfigJob(
									const Context& context,
									const BString& title,
									const BEntry& archivedRepoConfigEntry,
									const BString& repositoryBaseURL,
									const BDirectory& targetDirectory);
	virtual						~ActivateRepositoryConfigJob();

			const BString&		RepositoryName() const;

protected:
	virtual	status_t			Execute();
	virtual	void				Cleanup(status_t jobResult);

private:
			BEntry				fArchivedRepoConfigEntry;
			BString				fRepositoryBaseURL;
			BDirectory			fTargetDirectory;
			BEntry				fTargetEntry;

			BString				fRepositoryName;
};


}	// namespace Private

}	// namespace Package

}	// namespace Haiku


#endif // _HAIKU__PACKAGE__PRIVATE__ACTIVATE_REPOSITORY_CONFIG_JOB_H_
