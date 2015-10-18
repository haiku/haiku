/*
 * Copyright 2011-2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler <axeld@pinc-software.de>
 *		Oliver Tappe <zooey@hirschkaefer.de>
 */


#include <JobQueue.h>

#include <set>

#include <Autolock.h>
#include <Job.h>

#include <JobPrivate.h>


namespace BSupportKit {

namespace BPrivate {


struct JobQueue::JobPriorityLess {
	bool operator()(const BJob* left, const BJob* right) const;
};


/*!	Sort jobs by:
		1. descending count of dependencies (only jobs without dependencies are
		   runnable)
		2. job ticket number (order in which jobs were added to the queue)
*/
bool
JobQueue::JobPriorityLess::operator()(const BJob* left, const BJob* right) const
{
	int32 difference = left->CountDependencies() - right->CountDependencies();
	if (difference < 0)
		return true;
	if (difference > 0)
		return false;

	return left->TicketNumber() < right->TicketNumber();
};


class JobQueue::JobPriorityQueue
	: public std::set<BJob*, JobPriorityLess> {
};


// #pragma mark -


JobQueue::JobQueue()
	:
	fLock("job queue"),
	fNextTicketNumber(1)
{
	fInitStatus = _Init();
}


JobQueue::~JobQueue()
{
	Close();
	delete fQueuedJobs;
}


status_t
JobQueue::InitCheck() const
{
	return fInitStatus;
}


status_t
JobQueue::AddJob(BJob* job)
{
	if (fQueuedJobs == NULL)
		return B_NO_INIT;

	BAutolock lock(&fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	try {
		if (!fQueuedJobs->insert(job).second)
			return B_NAME_IN_USE;
	} catch (const std::bad_alloc& e) {
		return B_NO_MEMORY;
	} catch (...) {
		return B_ERROR;
	}
	BJob::Private(*job).SetTicketNumber(fNextTicketNumber++);
	job->AddStateListener(this);
	if (job->IsRunnable())
		release_sem(fHaveRunnableJobSem);

	return B_OK;
}


status_t
JobQueue::RemoveJob(BJob* job)
{
	if (fQueuedJobs == NULL)
		return B_NO_INIT;

	BAutolock lock(&fLock);
	if (lock.IsLocked()) {
		try {
			if (fQueuedJobs->erase(job) == 0)
				return B_NAME_NOT_FOUND;
		} catch (...) {
			return B_ERROR;
		}
		BJob::Private(*job).ClearTicketNumber();
		job->RemoveStateListener(this);
	}

	return B_OK;
}


void
JobQueue::JobSucceeded(BJob* job)
{
	BAutolock lock(&fLock);
	if (lock.IsLocked())
		_RequeueDependantJobsOf(job);
}


void
JobQueue::JobFailed(BJob* job)
{
	BAutolock lock(&fLock);
	if (lock.IsLocked())
		_RemoveDependantJobsOf(job);
}


BJob*
JobQueue::Pop()
{
	BJob* job;
	if (Pop(B_INFINITE_TIMEOUT, true, &job) == B_OK)
		return job;

	return NULL;
}


status_t
JobQueue::Pop(bigtime_t timeout, bool returnWhenEmpty, BJob** _job)
{
	BAutolock lock(&fLock);
	if (lock.IsLocked()) {
		while (true) {
			JobPriorityQueue::iterator head = fQueuedJobs->begin();
			if (head != fQueuedJobs->end()) {
				if ((*head)->IsRunnable()) {
					*_job = *head;
					fQueuedJobs->erase(head);
					return B_OK;
				}
			} else if (returnWhenEmpty)
				return B_ENTRY_NOT_FOUND;

			// we need to wait until a job becomes available/runnable
			status_t result;
			do {
				lock.Unlock();
				result = acquire_sem_etc(fHaveRunnableJobSem, 1,
					B_RELATIVE_TIMEOUT, timeout);
				if (!lock.Lock())
					return B_ERROR;
			} while (result == B_INTERRUPTED);
			if (result != B_OK)
				return result;
		}
	}

	return B_ERROR;
}


size_t
JobQueue::CountJobs() const
{
	BAutolock locker(fLock);
	return fQueuedJobs->size();
}


void
JobQueue::Close()
{
	if (fHaveRunnableJobSem < 0)
		return;

	BAutolock lock(&fLock);
	if (lock.IsLocked()) {
		delete_sem(fHaveRunnableJobSem);
		fHaveRunnableJobSem = -1;

		if (fQueuedJobs != NULL) {
			// get rid of all jobs
			for (JobPriorityQueue::iterator iter = fQueuedJobs->begin();
				iter != fQueuedJobs->end(); ++iter) {
				delete (*iter);
			}
			fQueuedJobs->clear();
		}
	}
}


status_t
JobQueue::_Init()
{
	status_t result = fLock.InitCheck();
	if (result != B_OK)
		return result;

	fQueuedJobs = new (std::nothrow) JobPriorityQueue();
	if (fQueuedJobs == NULL)
		return B_NO_MEMORY;

	fHaveRunnableJobSem = create_sem(0, "have runnable job");
	if (fHaveRunnableJobSem < 0)
		return fHaveRunnableJobSem;

	return B_OK;
}


void
JobQueue::_RequeueDependantJobsOf(BJob* job)
{
	while (BJob* dependantJob = job->DependantJobAt(0)) {
		JobPriorityQueue::iterator found = fQueuedJobs->find(dependantJob);
		bool removed = false;
		if (found != fQueuedJobs->end()) {
			try {
				fQueuedJobs->erase(dependantJob);
				removed = true;
			} catch (...) {
			}
		}
		dependantJob->RemoveDependency(job);
		if (removed) {
			// Only insert a job if it was in our queue before
			try {
				fQueuedJobs->insert(dependantJob);
				if (dependantJob->IsRunnable())
					release_sem(fHaveRunnableJobSem);
			} catch (...) {
			}
		}
	}
}


void
JobQueue::_RemoveDependantJobsOf(BJob* job)
{
	while (BJob* dependantJob = job->DependantJobAt(0)) {
		try {
			fQueuedJobs->erase(dependantJob);
		} catch (...) {
		}

		if (dependantJob->State() != B_JOB_STATE_ABORTED) {
			BJob::Private(*dependantJob).SetState(B_JOB_STATE_ABORTED);
			BJob::Private(*dependantJob).NotifyStateListeners();
		}

		_RemoveDependantJobsOf(dependantJob);
		dependantJob->RemoveDependency(job);
		// TODO: we need some sort of ownership management
		delete dependantJob;
	}
}


}	// namespace BPrivate

}	// namespace BPackageKit
