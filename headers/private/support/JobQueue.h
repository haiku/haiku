/*
 * Copyright 2011-2015, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SUPPORT_PRIVATE_JOB_QUEUE_H_
#define _SUPPORT_PRIVATE_JOB_QUEUE_H_


#include <Locker.h>
#include <SupportDefs.h>

#include <Job.h>


namespace BSupportKit {

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
			status_t			Pop(bigtime_t timeout, bool returnWhenEmpty,
									BJob** _job);
									// caller owns job

			size_t				CountJobs() const;

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

	mutable	BLocker				fLock;
			uint32				fNextTicketNumber;
			JobPriorityQueue*	fQueuedJobs;
			sem_id				fHaveRunnableJobSem;

			status_t			fInitStatus;
};


}	// namespace BPrivate

}	// namespace BSupportKit


#endif // _SUPPORT_PRIVATE_JOB_QUEUE_H_
