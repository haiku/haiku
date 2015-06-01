/*
 * Copyright 2011-2015, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__JOB_H_
#define _PACKAGE__JOB_H_


#include <support/Job.h>


namespace BPackageKit {


class BContext;


namespace BPrivate {
	class JobQueue;
}


class BJob : public BSupportKit::BJob {
public:
								BJob(const BContext& context,
									const BString& title);
	virtual						~BJob();

protected:
			const BContext&		fContext;
};


}	// namespace BPackageKit


#endif // _PACKAGE__JOB_H_
