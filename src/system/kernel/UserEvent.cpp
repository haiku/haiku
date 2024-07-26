/*
 * Copyright 2014, Paweł Dziepak, pdziepak@quarnos.org.
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <UserEvent.h>

#include <ksignal.h>
#include <thread_types.h>
#include <util/AutoLock.h>


// #pragma mark - UserEvent


UserEvent::~UserEvent()
{
}


// #pragma mark - SignalEvent


struct SignalEvent::EventSignal : Signal {
	EventSignal(uint32 number, int32 signalCode, int32 errorCode,
		pid_t sendingProcess)
		:
		Signal(number, signalCode, errorCode, sendingProcess),
		fInUse(0)
	{
	}

	bool MarkUsed()
	{
		return atomic_get_and_set(&fInUse, 1) != 0;
	}

	void SetUnused()
	{
		// mark not-in-use
		atomic_set(&fInUse, 0);
	}

	virtual void Handled()
	{
		SetUnused();

		Signal::Handled();
	}

private:
	int32				fInUse;
};


SignalEvent::SignalEvent(EventSignal* signal)
	:
	fSignal(signal),
	fPendingDPC(0)
{
}


SignalEvent::~SignalEvent()
{
	fSignal->ReleaseReference();
}


void
SignalEvent::SetUserValue(union sigval userValue)
{
	fSignal->SetUserValue(userValue);
}


status_t
SignalEvent::Fire()
{
	bool wasPending = atomic_get_and_set(&fPendingDPC, 1) != 0;
	if (wasPending)
		return B_BUSY;

	if (fSignal->MarkUsed()) {
		atomic_set(&fPendingDPC, 0);
		return B_BUSY;
	}

	AcquireReference();
	DPCQueue::DefaultQueue(B_NORMAL_PRIORITY)->Add(this);

	return B_OK;
}


// #pragma mark - TeamSignalEvent


TeamSignalEvent::TeamSignalEvent(Team* team, EventSignal* signal)
	:
	SignalEvent(signal),
	fTeam(team)
{
}


/*static*/ TeamSignalEvent*
TeamSignalEvent::Create(Team* team, uint32 signalNumber, int32 signalCode,
	int32 errorCode)
{
	// create the signal
	EventSignal* signal = new(std::nothrow) EventSignal(signalNumber,
		signalCode, errorCode, team->id);
	if (signal == NULL)
		return NULL;

	// create the event
	TeamSignalEvent* event = new(std::nothrow) TeamSignalEvent(team, signal);
	if (event == NULL) {
		delete signal;
		return NULL;
	}

	return event;
}


status_t
TeamSignalEvent::Fire()
{
	// We need a reference to the team to guarantee that it is still there when
	// the DPC actually runs.
	fTeam->AcquireReference();
	status_t result = SignalEvent::Fire();
	if (result != B_OK)
		fTeam->ReleaseReference();

	return result;
}


void
TeamSignalEvent::DoDPC(DPCQueue* queue)
{
	fSignal->AcquireReference();
		// one reference is transferred to send_signal_to_team_locked

	InterruptsSpinLocker locker(fTeam->signal_lock);
	status_t error = send_signal_to_team_locked(fTeam, fSignal->Number(),
		fSignal, B_DO_NOT_RESCHEDULE);
	locker.Unlock();
	fTeam->ReleaseReference();

	// There are situations (for certain signals), in which
	// send_signal_to_team_locked() succeeds without queuing the signal.
	if (error != B_OK || !fSignal->IsPending())
		fSignal->SetUnused();

	// We're no longer queued in the DPC queue, so we can be reused.
	atomic_set(&fPendingDPC, 0);

	ReleaseReference();
}


// #pragma mark - ThreadSignalEvent


ThreadSignalEvent::ThreadSignalEvent(Thread* thread, EventSignal* signal)
	:
	SignalEvent(signal),
	fThread(thread)
{
}


/*static*/ ThreadSignalEvent*
ThreadSignalEvent::Create(Thread* thread, uint32 signalNumber, int32 signalCode,
	int32 errorCode, pid_t sendingTeam)
{
	// create the signal
	EventSignal* signal = new(std::nothrow) EventSignal(signalNumber,
		signalCode, errorCode, sendingTeam);
	if (signal == NULL)
		return NULL;

	// create the event
	ThreadSignalEvent* event = new(std::nothrow) ThreadSignalEvent(thread, signal);
	if (event == NULL) {
		delete signal;
		return NULL;
	}

	return event;
}


status_t
ThreadSignalEvent::Fire()
{
	// We need a reference to the thread to guarantee that it is still there
	// when the DPC actually runs.
	fThread->AcquireReference();
	status_t result = SignalEvent::Fire();
	if (result != B_OK)
		fThread->ReleaseReference();

	return result;
}


void
ThreadSignalEvent::DoDPC(DPCQueue* queue)
{
	fSignal->AcquireReference();
		// one reference is transferred to send_signal_to_team_locked
	InterruptsReadSpinLocker teamLocker(fThread->team_lock);
	SpinLocker locker(fThread->team->signal_lock);
	status_t error = send_signal_to_thread_locked(fThread, fSignal->Number(),
		fSignal, B_DO_NOT_RESCHEDULE);
	locker.Unlock();
	teamLocker.Unlock();
	fThread->ReleaseReference();

	// There are situations (for certain signals), in which
	// send_signal_to_team_locked() succeeds without queuing the signal.
	if (error != B_OK || !fSignal->IsPending())
		fSignal->SetUnused();

	// We're no longer queued in the DPC queue, so we can be reused.
	atomic_set(&fPendingDPC, 0);

	ReleaseReference();
}


// #pragma mark - UserEvent


CreateThreadEvent::CreateThreadEvent(const ThreadCreationAttributes& attributes)
	:
	fCreationAttributes(attributes),
	fPendingDPC(0)
{
	// attributes.name is a pointer to a temporary buffer. Copy the name into
	// our own buffer and replace the name pointer.
	strlcpy(fThreadName, attributes.name, sizeof(fThreadName));
	fCreationAttributes.name = fThreadName;
}


/*static*/ CreateThreadEvent*
CreateThreadEvent::Create(const ThreadCreationAttributes& attributes)
{
	return new(std::nothrow) CreateThreadEvent(attributes);
}


status_t
CreateThreadEvent::Fire()
{
	bool wasPending = atomic_get_and_set(&fPendingDPC, 1) != 0;
	if (wasPending)
		return B_BUSY;

	AcquireReference();
	DPCQueue::DefaultQueue(B_NORMAL_PRIORITY)->Add(this);

	return B_OK;
}


void
CreateThreadEvent::DoDPC(DPCQueue* queue)
{
	// We're no longer queued in the DPC queue, so we can be reused.
	atomic_set(&fPendingDPC, 0);

	// create the thread
	thread_id threadID = thread_create_thread(fCreationAttributes, false);
	if (threadID >= 0)
		resume_thread(threadID);

	ReleaseReference();
}
