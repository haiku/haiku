/*
 * Copyright 2013-2014, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include "JobQueue.h"

#include <PthreadMutexLocker.h>


// #pragma mark - JobQueue


JobQueue::JobQueue()
	:
	fMutexInitialized(false),
	fNewJobConditionInitialized(false),
	fJobs(),
	fClosed(false)
{
}


JobQueue::~JobQueue()
{
	if (fMutexInitialized) {
		PthreadMutexLocker mutexLocker(fMutex);
		while (Job* job = fJobs.RemoveHead())
			job->ReleaseReference();
	}

	if (fNewJobConditionInitialized)
		pthread_cond_destroy(&fNewJobCondition);

	if (fMutexInitialized)
		pthread_mutex_destroy(&fMutex);
}


status_t
JobQueue::Init()
{
	status_t error = pthread_mutex_init(&fMutex, NULL);
	if (error != B_OK)
		return error;
	fMutexInitialized = true;

	error = pthread_cond_init(&fNewJobCondition, NULL);
	if (error != B_OK)
		return error;
	fNewJobConditionInitialized = true;

	return B_OK;
}


void
JobQueue::Close()
{
	if (fMutexInitialized && fNewJobConditionInitialized) {
		PthreadMutexLocker mutexLocker(fMutex);
		fClosed = true;
		pthread_cond_broadcast(&fNewJobCondition);
	}
}


bool
JobQueue::QueueJob(Job* job)
{
	PthreadMutexLocker mutexLocker(fMutex);
	if (fClosed)
		return false;

	fJobs.Add(job);
	job->AcquireReference();

	pthread_cond_signal(&fNewJobCondition);
	return true;
}


Job*
JobQueue::DequeueJob()
{
	PthreadMutexLocker mutexLocker(fMutex);

	while (!fClosed) {
		Job* job = fJobs.RemoveHead();
		if (job != NULL)
			return job;

		if (!fClosed)
			pthread_cond_wait(&fNewJobCondition, &fMutex);
	}

	return NULL;
}


void
JobQueue::DeleteJobs(Filter* filter)
{
	PthreadMutexLocker mutexLocker(fMutex);

	for (JobList::Iterator it = fJobs.GetIterator(); Job* job = it.Next();) {
		if (filter->FilterJob(job)) {
			it.Remove();
			delete job;
		}
	}
}


// #pragma mark - Filter


JobQueue::Filter::~Filter()
{
}
