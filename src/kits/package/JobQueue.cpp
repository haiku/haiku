/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <zooey@hirschkaefer.de>
 */


#include <package/JobQueue.h>

#include <set>

#include <Autolock.h>

#include <package/Job.h>


namespace BPackageKit {

namespace BPrivate {


struct JobQueue::JobPriorityLess {
	bool operator()(const BJob* left, const BJob* right) const;
};


// sort jobs by:
//     1. descending count of dependencies (only jobs without dependencies are
//        runnable)
//     2. job ticket number (order in which jobs were added to the queue)
//
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


JobQueue::JobQueue()
	:
	fLock("job queue"),
	fNextTicketNumber(1)
{
	fInitStatus = _Init();
}


JobQueue::~JobQueue()
{
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
	if (lock.IsLocked()) {
		try {
			if (!fQueuedJobs->insert(job).second)
				return B_NAME_IN_USE;
		} catch (const std::bad_alloc& e) {
			return B_NO_MEMORY;
		} catch (...) {
			return B_ERROR;
		}
		job->_SetTicketNumber(fNextTicketNumber++);
		job->AddStateListener(this);
	}

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
		job->_ClearTicketNumber();
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
	BAutolock lock(&fLock);
	if (lock.IsLocked()) {
		JobPriorityQueue::iterator head = fQueuedJobs->begin();
		if (head == fQueuedJobs->end())
			return NULL;
		while (!(*head)->IsRunnable()) {
			// we need to wait until a job becomes runnable
			status_t result;
			do {
				lock.Unlock();
				result = acquire_sem(fHaveRunnableJobSem);
				if (!lock.Lock())
					return NULL;
			} while (result == B_INTERRUPTED);
			if (result != B_OK)
				return NULL;

			// fetch current head, it must be runnable now
			head = fQueuedJobs->begin();
			if (head == fQueuedJobs->end())
				return NULL;
		}
		fQueuedJobs->erase(head);
		return *head;
	}

	return NULL;
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
		try {
			fQueuedJobs->erase(dependantJob);
		} catch (...) {
		}
		dependantJob->RemoveDependency(job);
		try {
			fQueuedJobs->insert(dependantJob);
		} catch (...) {
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
		_RemoveDependantJobsOf(dependantJob);
		delete job;
	}
}


}	// namespace BPrivate

}	// namespace BPackageKit
