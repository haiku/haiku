/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _HAIKU__PACKAGE__JOB_QUEUE_H_
#define _HAIKU__PACKAGE__JOB_QUEUE_H_


#include <Locker.h>
#include <SupportDefs.h>

#include <package/Job.h>


namespace Haiku {

namespace Package {


class JobQueue : public JobStateListener {
public:
								JobQueue();
								~JobQueue();

			status_t			AddJob(Job* job);
			status_t			RemoveJob(Job* job);

			Job*				Pop();

								// JobStateListener
	virtual	void				JobSucceeded(Job* job);
	virtual	void				JobFailed(Job* job);

private:
			struct JobPriorityLess;
			class JobPriorityQueue;

private:
			void				_UpdateDependantJobsOf(Job* job);

			BLocker				fLock;
			JobPriorityQueue*	fQueuedJobs;
};


}	// namespace Package

}	// namespace Haiku


#endif // _HAIKU__PACKAGE__JOB_QUEUE_H_
