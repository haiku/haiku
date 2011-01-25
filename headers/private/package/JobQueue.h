/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__PRIVATE__JOB_QUEUE_H_
#define _PACKAGE__PRIVATE__JOB_QUEUE_H_


#include <Locker.h>
#include <SupportDefs.h>

#include <package/Job.h>


namespace BPackageKit {

namespace BPrivate {


class JobQueue : private BJobStateListener {
public:
								JobQueue();
	virtual						~JobQueue();

			status_t			InitCheck() const;

			status_t			AddJob(BJob* job);
									// takes ownership
			status_t			RemoveJob(BJob* job);
									// gives up ownership

			BJob*				Pop();
									// caller owns job

			void				Close();

private:
								// BJobStateListener
	virtual	void				JobSucceeded(BJob* job);
	virtual	void				JobFailed(BJob* job);

private:
			struct JobPriorityLess;
			class JobPriorityQueue;

private:
			status_t			_Init();

			void				_RequeueDependantJobsOf(BJob* job);
			void				_RemoveDependantJobsOf(BJob* job);

			BLocker				fLock;
			uint32				fNextTicketNumber;
			JobPriorityQueue*	fQueuedJobs;
			sem_id				fHaveRunnableJobSem;

			status_t			fInitStatus;
};


}	// namespace BPrivate

}	// namespace BPackageKit


#endif // _PACKAGE__PRIVATE__JOB_QUEUE_H_
