/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__PRIVATE__FETCH_FILE_JOB_H_
#define _PACKAGE__PRIVATE__FETCH_FILE_JOB_H_


#include <Entry.h>
#include <String.h>

#include <package/Job.h>


namespace BPackageKit {

namespace BPrivate {


class FetchFileJob : public BJob {
	typedef	BJob				inherited;

public:
								FetchFileJob(const BContext& context,
									const BString& title,
									const BString& fileURL,
									const BEntry& targetEntry);
	virtual						~FetchFileJob();

protected:
	virtual	status_t			Execute();
	virtual	void				Cleanup(status_t jobResult);

private:
			BString				fFileURL;
			BEntry				fTargetEntry;
};


}	// namespace BPrivate

}	// namespace BPackageKit


#endif // _PACKAGE__PRIVATE__FETCH_FILE_JOB_H_
