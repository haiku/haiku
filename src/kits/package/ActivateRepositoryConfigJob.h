/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__PRIVATE__ACTIVATE_REPOSITORY_CONFIG_JOB_H_
#define _PACKAGE__PRIVATE__ACTIVATE_REPOSITORY_CONFIG_JOB_H_


#include <Directory.h>
#include <Entry.h>
#include <String.h>

#include <package/Job.h>


namespace BPackageKit {

namespace BPrivate {


class ActivateRepositoryConfigJob : public BJob {
	typedef	BJob				inherited;

public:
								ActivateRepositoryConfigJob(
									const BContext& context,
									const BString& title,
									const BEntry& archivedRepoInfoEntry,
									const BString& repositoryBaseURL,
									const BDirectory& targetDirectory);
	virtual						~ActivateRepositoryConfigJob();

			const BString&		RepositoryName() const;

protected:
	virtual	status_t			Execute();
	virtual	void				Cleanup(status_t jobResult);

private:
			BEntry				fArchivedRepoInfoEntry;
			BString				fRepositoryBaseURL;
			BDirectory			fTargetDirectory;
			BEntry				fTargetEntry;

			BString				fRepositoryName;
};


}	// namespace BPrivate

}	// namespace BPackageKit


#endif // _PACKAGE__PRIVATE__ACTIVATE_REPOSITORY_CONFIG_JOB_H_
