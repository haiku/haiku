/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef _HAIKU__PACKAGE__PRIVATE__FETCH_FILE_JOB_H_
#define _HAIKU__PACKAGE__PRIVATE__FETCH_FILE_JOB_H_


#include <Entry.h>
#include <String.h>

#include <package/Job.h>


namespace Haiku {

namespace Package {

namespace Private {


class FetchFileJob : public Job {
	typedef	Job					inherited;

public:
								FetchFileJob(const Context& context,
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


}	// namespace Private

}	// namespace Package

}	// namespace Haiku


#endif // _HAIKU__PACKAGE__PRIVATE__FETCH_FILE_JOB_H_
