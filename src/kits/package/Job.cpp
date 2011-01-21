/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <zooey@hirschkaefer.de>
 */


#include <package/Job.h>

#include <Errors.h>


namespace Haiku {

namespace Package {


JobStateListener::~JobStateListener()
{
}


void
JobStateListener::JobStarted(Job* job)
{
}


void
JobStateListener::JobSucceeded(Job* job)
{
}


void
JobStateListener::JobFailed(Job* job)
{
}


void
JobStateListener::JobAborted(Job* job)
{
}


Job::Job(const BString& title)
	:
	fTitle(title),
	fState(JOB_STATE_WAITING_TO_RUN)
{
	if (fTitle.Length() == 0)
		fInitStatus = B_BAD_VALUE;
	else
		fInitStatus = B_OK;
}


Job::~Job()
{
}


status_t
Job::InitCheck() const
{
	return fInitStatus;
}


const BString&
Job::Title() const
{
	return fTitle;
}


JobState
Job::State() const
{
	return fState;
}


status_t
Job::Result() const
{
	return fResult;
}


status_t
Job::Run()
{
	if (fState != JOB_STATE_WAITING_TO_RUN)
		return B_NOT_ALLOWED;

	fState = JOB_STATE_RUNNING;
	NotifyStateListeners();

	fResult = Execute();
	Cleanup(fResult);

	fState = fResult == B_OK
		? JOB_STATE_SUCCEEDED
		: fResult == B_INTERRUPTED
			? JOB_STATE_ABORTED
			: JOB_STATE_FAILED;
	NotifyStateListeners();

	return fResult;
}


void
Job::Cleanup(status_t /*jobResult*/)
{
}


status_t
Job::AddStateListener(JobStateListener* listener)
{
	return fStateListeners.AddItem(listener) ? B_OK : B_ERROR;
}


status_t
Job::RemoveStateListener(JobStateListener* listener)
{
	return fStateListeners.RemoveItem(listener) ? B_OK : B_ERROR;
}


status_t
Job::AddDependency(Job* job)
{
	if (fDependencies.HasItem(job))
		return B_ERROR;

	if (fDependencies.AddItem(job) && job->fDependantJobs.AddItem(this))
		return B_OK;

	return B_ERROR;
}


status_t
Job::RemoveDependency(Job* job)
{
	if (!fDependencies.HasItem(job))
		return B_ERROR;

	if (fDependencies.RemoveItem(job) && job->fDependantJobs.RemoveItem(this))
		return B_OK;

	return B_ERROR;
}


int32
Job::CountDependencies() const
{
	return fDependencies.CountItems();
}


Job*
Job::DependantJobAt(int32 index) const
{
	return fDependantJobs.ItemAt(index);
}


void
Job::NotifyStateListeners()
{
	int32 count = fStateListeners.CountItems();
	for (int i = 0; i < count; ++i) {
		JobStateListener* listener = fStateListeners.ItemAt(i);
		if (listener == NULL)
			continue;
		switch (fState) {
			case JOB_STATE_RUNNING:
				listener->JobStarted(this);
				break;
			case JOB_STATE_SUCCEEDED:
				listener->JobSucceeded(this);
				break;
			case JOB_STATE_FAILED:
				listener->JobFailed(this);
				break;
			case JOB_STATE_ABORTED:
				listener->JobAborted(this);
				break;
			default:
				break;
		}
	}
}


}	// namespace Package

}	// namespace Haiku
