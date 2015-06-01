/*
 * Copyright 2011-2015, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__REQUEST_H_
#define _PACKAGE__REQUEST_H_


#include <SupportDefs.h>

#include <package/Job.h>


namespace BSupportKit {
	namespace BPrivate {
		class JobQueue;
	}
}


namespace BPackageKit {


class BContext;


class BRequest : protected BSupportKit::BJobStateListener {
public:
								BRequest(const BContext& context);
	virtual						~BRequest();

			status_t			InitCheck() const;

	virtual	status_t			CreateInitialJobs() = 0;

			BSupportKit::BJob*	PopRunnableJob();

			status_t			Process(bool failIfCanceledOnly = false);

protected:
			status_t			QueueJob(BSupportKit::BJob* job);

			const BContext&		fContext;

protected:
			status_t			fInitStatus;
			BSupportKit::BPrivate::JobQueue*
								fJobQueue;
};


}	// namespace BPackageKit


#endif // _PACKAGE__REQUEST_H_
