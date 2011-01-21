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


namespace Haiku {

namespace Package {


struct JobQueue::JobPriorityLess {
	bool operator()(const Job* left, const Job* right) const;
};


bool
JobQueue::JobPriorityLess::operator()(const Job* left, const Job* right) const
{
	int32 difference = left->CountDependencies() - right->CountDependencies();
	if (difference < 0)
		return true;
	if (difference > 0)
		return false;

	return left->Title() < right->Title();
};


class JobQueue::JobPriorityQueue
	: public std::set<Job*, JobPriorityLess> {

};


JobQueue::JobQueue()
	:
	fLock("job queue"),
	fQueuedJobs(new (std::nothrow) JobPriorityQueue())
{
}


JobQueue::~JobQueue()
{
}


status_t
JobQueue::AddJob(Job* job)
{
	if (fQueuedJobs == NULL)
		return B_NO_INIT;

	BAutolock lock(&fLock);
	if (lock.IsLocked()) {
		try {
			fQueuedJobs->insert(job);
		} catch (const std::bad_alloc& e) {
			return B_NO_MEMORY;
		} catch (...) {
			return B_NO_MEMORY;
		}
		job->AddStateListener(this);
	}

	return B_OK;
}


status_t
JobQueue::RemoveJob(Job* job)
{
	if (fQueuedJobs == NULL)
		return B_NO_INIT;

	BAutolock lock(&fLock);
	if (lock.IsLocked()) {
		try {
			fQueuedJobs->erase(job);
		} catch (...) {
			return B_ERROR;
		}
		job->RemoveStateListener(this);
	}

	return B_OK;
}


void
JobQueue::JobSucceeded(Job* job)
{
	_UpdateDependantJobsOf(job);
}


void
JobQueue::JobFailed(Job* job)
{
	_UpdateDependantJobsOf(job);
}


Job*
JobQueue::Pop()
{
	BAutolock lock(&fLock);
	if (lock.IsLocked()) {
		JobPriorityQueue::iterator head = fQueuedJobs->begin();
		if (head == fQueuedJobs->end())
			return NULL;
		fQueuedJobs->erase(head);
		return *head;
	}

	return NULL;
}


void
JobQueue::_UpdateDependantJobsOf(Job* job)
{
	BAutolock lock(&fLock);
	if (lock.IsLocked()) {
		while (Job* dependantJob = job->DependantJobAt(0)) {
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
}


}	// namespace Package

}	// namespace Haiku
