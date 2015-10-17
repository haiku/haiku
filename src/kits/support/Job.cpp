/*
 * Copyright 2011-2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <zooey@hirschkaefer.de>
 *		Rene Gollent <rene@gollent.com>
 */


#include <Job.h>

#include <Errors.h>


namespace BSupportKit {


BJobStateListener::~BJobStateListener()
{
}


void
BJobStateListener::JobStarted(BJob* job)
{
}


void
BJobStateListener::JobProgress(BJob* job)
{
}


void
BJobStateListener::JobSucceeded(BJob* job)
{
}


void
BJobStateListener::JobFailed(BJob* job)
{
}


void
BJobStateListener::JobAborted(BJob* job)
{
}


// #pragma mark -


BJob::BJob(const BString& title)
	:
	fTitle(title),
	fState(B_JOB_STATE_WAITING_TO_RUN),
	fTicketNumber(0xFFFFFFFFUL)
{
	if (fTitle.Length() == 0)
		fInitStatus = B_BAD_VALUE;
	else
		fInitStatus = B_OK;
}


BJob::~BJob()
{
}


status_t
BJob::InitCheck() const
{
	return fInitStatus;
}


const BString&
BJob::Title() const
{
	return fTitle;
}


BJobState
BJob::State() const
{
	return fState;
}


status_t
BJob::Result() const
{
	return fResult;
}


const BString&
BJob::ErrorString() const
{
	return fErrorString;
}


uint32
BJob::TicketNumber() const
{
	return fTicketNumber;
}


void
BJob::_SetTicketNumber(uint32 ticketNumber)
{
	fTicketNumber = ticketNumber;
}


void
BJob::_ClearTicketNumber()
{
	fTicketNumber = 0xFFFFFFFFUL;
}


void
BJob::SetErrorString(const BString& error)
{
	fErrorString = error;
}


status_t
BJob::Run()
{
	if (fState != B_JOB_STATE_WAITING_TO_RUN)
		return B_NOT_ALLOWED;

	fState = B_JOB_STATE_STARTED;
	NotifyStateListeners();

	fState = B_JOB_STATE_IN_PROGRESS;
	fResult = Execute();
	Cleanup(fResult);

	fState = fResult == B_OK
		? B_JOB_STATE_SUCCEEDED
		: fResult == B_CANCELED
			? B_JOB_STATE_ABORTED
			: B_JOB_STATE_FAILED;
	NotifyStateListeners();

	return fResult;
}


void
BJob::Cleanup(status_t /*jobResult*/)
{
}


status_t
BJob::AddStateListener(BJobStateListener* listener)
{
	return fStateListeners.AddItem(listener) ? B_OK : B_ERROR;
}


status_t
BJob::RemoveStateListener(BJobStateListener* listener)
{
	return fStateListeners.RemoveItem(listener) ? B_OK : B_ERROR;
}


status_t
BJob::AddDependency(BJob* job)
{
	if (fDependencies.HasItem(job))
		return B_ERROR;

	if (fDependencies.AddItem(job) && job->fDependantJobs.AddItem(this))
		return B_OK;

	return B_ERROR;
}


status_t
BJob::RemoveDependency(BJob* job)
{
	if (!fDependencies.HasItem(job))
		return B_ERROR;

	if (fDependencies.RemoveItem(job) && job->fDependantJobs.RemoveItem(this))
		return B_OK;

	return B_ERROR;
}


bool
BJob::IsRunnable() const
{
	return fDependencies.IsEmpty();
}


int32
BJob::CountDependencies() const
{
	return fDependencies.CountItems();
}


BJob*
BJob::DependantJobAt(int32 index) const
{
	return fDependantJobs.ItemAt(index);
}


void
BJob::SetState(BJobState state)
{
	fState = state;
}


void
BJob::NotifyStateListeners()
{
	int32 count = fStateListeners.CountItems();
	for (int i = 0; i < count; ++i) {
		BJobStateListener* listener = fStateListeners.ItemAt(i);
		if (listener == NULL)
			continue;
		switch (fState) {
			case B_JOB_STATE_STARTED:
				listener->JobStarted(this);
				break;
			case B_JOB_STATE_IN_PROGRESS:
				listener->JobProgress(this);
				break;
			case B_JOB_STATE_SUCCEEDED:
				listener->JobSucceeded(this);
				break;
			case B_JOB_STATE_FAILED:
				listener->JobFailed(this);
				break;
			case B_JOB_STATE_ABORTED:
				listener->JobAborted(this);
				break;
			default:
				break;
		}
	}
}


}	// namespace BPackageKit
