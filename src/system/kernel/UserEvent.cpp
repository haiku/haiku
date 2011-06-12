/*
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
		fInUse(false)
	{
	}

	bool IsInUse() const
	{
		return fInUse;
	}

	void SetInUse(bool inUse)
	{
		fInUse = inUse;
	}

	virtual void Handled()
	{
		// mark not-in-use
		InterruptsSpinLocker schedulerLocker(gSchedulerLock);
		fInUse = false;
		schedulerLocker.Unlock();

		Signal::Handled();
	}

private:
	bool				fInUse;
};


SignalEvent::SignalEvent(EventSignal* signal)
	:
	fSignal(signal)
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
	TeamSignalEvent* event = new TeamSignalEvent(team, signal);
	if (event == NULL) {
		delete signal;
		return NULL;
	}

	return event;
}


status_t
TeamSignalEvent::Fire()
{
	// called with the scheduler lock held
	if (fSignal->IsInUse())
		return B_BUSY;

	fSignal->AcquireReference();
		// one reference is transferred to send_signal_to_team_locked
	status_t error = send_signal_to_team_locked(fTeam, fSignal->Number(),
		fSignal, B_DO_NOT_RESCHEDULE);
	if (error == B_OK) {
		// Mark the signal in-use. There are situations (for certain signals),
		// in which send_signal_to_team_locked() succeeds without queuing the
		// signal.
		fSignal->SetInUse(fSignal->IsPending());
	}

	return error;
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
	ThreadSignalEvent* event = new ThreadSignalEvent(thread, signal);
	if (event == NULL) {
		delete signal;
		return NULL;
	}

	return event;
}


status_t
ThreadSignalEvent::Fire()
{
	// called with the scheduler lock held
	if (fSignal->IsInUse())
		return B_BUSY;

	fSignal->AcquireReference();
		// one reference is transferred to send_signal_to_team_locked
	status_t error = send_signal_to_thread_locked(fThread, fSignal->Number(),
		fSignal, B_DO_NOT_RESCHEDULE);
	if (error == B_OK) {
		// Mark the signal in-use. There are situations (for certain signals),
		// in which send_signal_to_team_locked() succeeds without queuing the
		// signal.
		fSignal->SetInUse(fSignal->IsPending());
	}

	return error;
}


// #pragma mark - UserEvent


CreateThreadEvent::CreateThreadEvent(const ThreadCreationAttributes& attributes)
	:
	fCreationAttributes(attributes),
	fPendingDPC(false)
{
	// attributes.name is a pointer to a temporary buffer. Copy the name into
	// our own buffer and replace the name pointer.
	strlcpy(fThreadName, attributes.name, sizeof(fThreadName));
	fCreationAttributes.name = fThreadName;
}


CreateThreadEvent::~CreateThreadEvent()
{
	// cancel the DPC to be on the safe side
	DPCQueue::DefaultQueue(B_NORMAL_PRIORITY)->Cancel(this);
}


/*static*/ CreateThreadEvent*
CreateThreadEvent::Create(const ThreadCreationAttributes& attributes)
{
	return new(std::nothrow) CreateThreadEvent(attributes);
}


status_t
CreateThreadEvent::Fire()
{
	if (fPendingDPC)
		return B_BUSY;

	fPendingDPC = true;

	DPCQueue::DefaultQueue(B_NORMAL_PRIORITY)->Add(this, true);

	return B_OK;
}


void
CreateThreadEvent::DoDPC(DPCQueue* queue)
{
	// We're no longer queued in the DPC queue, so we can be reused.
	{
		InterruptsSpinLocker schedulerLocker(gSchedulerLock);
		fPendingDPC = false;
	}

	// create the thread
	thread_id threadID = thread_create_thread(fCreationAttributes, false);
	if (threadID >= 0)
		resume_thread(threadID);
}
