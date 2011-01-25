/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__REQUEST_H_
#define _PACKAGE__REQUEST_H_


#include <SupportDefs.h>

#include <package/Job.h>


namespace BPackageKit {


class BContext;
namespace BPrivate {
	class JobQueue;
}
using BPrivate::JobQueue;


class BRequest : protected BJobStateListener {
public:
								BRequest(const BContext& context);
	virtual						~BRequest();

			status_t			InitCheck() const;

	virtual	status_t			CreateInitialJobs() = 0;

			BJob*				PopRunnableJob();

protected:
			status_t			QueueJob(BJob* job);

			const BContext&		fContext;

private:
			status_t			fInitStatus;
			JobQueue*			fJobQueue;
};


}	// namespace BPackageKit


#endif // _PACKAGE__REQUEST_H_
