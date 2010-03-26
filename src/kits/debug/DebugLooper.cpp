/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <DebugLooper.h>

#include <new>

#include <AutoLocker.h>
#include <DebugMessageHandler.h>
#include <TeamDebugger.h>
#include <util/DoublyLinkedList.h>


struct BDebugLooper::Debugger {
	BTeamDebugger*			debugger;
	BDebugMessageHandler*	handler;

	Debugger(BTeamDebugger* debugger, BDebugMessageHandler* handler)
		:
		debugger(debugger),
		handler(handler)
	{
	}
};


struct BDebugLooper::Job : DoublyLinkedListLinkImpl<Job> {
	Job()
		:
		fDoneSemaphore(-1)
	{
	}

	virtual ~Job()
	{
	}

	status_t Wait(BLocker& lock)
	{
		fDoneSemaphore = create_sem(0, "debug looper job");

		lock.Unlock();

		while (acquire_sem(fDoneSemaphore) == B_INTERRUPTED) {
		}

		lock.Lock();

		delete_sem(fDoneSemaphore);
		fDoneSemaphore = -1;

		return fResult;
	}

	void Done(status_t result)
	{
		fResult = result;
		release_sem(fDoneSemaphore);
	}

	virtual status_t Do(BDebugLooper* looper) = 0;

protected:
	sem_id			fDoneSemaphore;
	status_t		fResult;
};


struct BDebugLooper::JobList : DoublyLinkedList<Job> {
};


struct BDebugLooper::AddDebuggerJob : Job {
	AddDebuggerJob(BTeamDebugger* debugger,
		BDebugMessageHandler* handler)
		:
		fDebugger(debugger),
		fHandler(handler)
	{
	}

	virtual status_t Do(BDebugLooper* looper)
	{
		Debugger* debugger = new(std::nothrow) Debugger(fDebugger, fHandler);
		if (debugger == NULL || !looper->fDebuggers.AddItem(debugger)) {
			delete debugger;
			return B_NO_MEMORY;
		}

		return B_OK;
	}

private:
	BTeamDebugger*			fDebugger;
	BDebugMessageHandler*	fHandler;
};


struct BDebugLooper::RemoveDebuggerJob : Job {
	RemoveDebuggerJob(team_id team)
		:
		fTeam(team)
	{
	}

	virtual status_t Do(BDebugLooper* looper)
	{
		for (int32 i = 0; Debugger* debugger = looper->fDebuggers.ItemAt(i);
				i++) {
			if (debugger->debugger->Team() == fTeam) {
				delete looper->fDebuggers.RemoveItemAt(i);
				return B_OK;
			}
		}

		return B_ENTRY_NOT_FOUND;
	}

private:
	team_id	fTeam;
};


// #pragma mark -


BDebugLooper::BDebugLooper()
	:
	fLock("debug looper"),
	fThread(-1),
	fOwnsThread(false),
	fTerminating(false),
	fNotified(false),
	fJobs(NULL),
	fEventSemaphore(-1)
{
}


BDebugLooper::~BDebugLooper()
{
}


status_t
BDebugLooper::Init()
{
	status_t error = fLock.InitCheck();
	if (error != B_OK)
		return error;

	AutoLocker<BLocker> locker(fLock);

	if (fThread >= 0)
		return B_BAD_VALUE;

	if (fJobs == NULL) {
		fJobs = new(std::nothrow) JobList;
		if (fJobs == NULL)
			return B_NO_MEMORY;
	}

	if (fEventSemaphore < 0) {
		fEventSemaphore = create_sem(0, "debug looper event");
		if (fEventSemaphore < 0)
			return fEventSemaphore;
	}

	return B_OK;
}


thread_id
BDebugLooper::Run(bool spawnThread)
{
	AutoLocker<BLocker> locker(fLock);

	if (fThread >= 0)
		return B_BAD_VALUE;

	fNotified = false;

	if (spawnThread) {
		fThread = spawn_thread(&_MessageLoopEntry, "debug looper",
			B_NORMAL_PRIORITY, this);
		if (fThread < 0)
			return fThread;

		fOwnsThread = true;

		resume_thread(fThread);
		return B_OK;
	}

	fThread = find_thread(NULL);
	fOwnsThread = false;

	_MessageLoop();
	return B_OK;
}


void
BDebugLooper::Quit()
{
	AutoLocker<BLocker> locker(fLock);

	fTerminating = true;
	_Notify();
}


status_t
BDebugLooper::AddTeamDebugger(BTeamDebugger* debugger,
	BDebugMessageHandler* handler)
{
	if (debugger == NULL || handler == NULL)
		return B_BAD_VALUE;

	AddDebuggerJob job(debugger, handler);
	return _DoJob(&job);
}


bool
BDebugLooper::RemoveTeamDebugger(BTeamDebugger* debugger)
{
	if (debugger == NULL)
		return false;

	RemoveDebuggerJob job(debugger->Team());
	return _DoJob(&job) == B_OK;
}


bool
BDebugLooper::RemoveTeamDebugger(team_id team)
{
	if (team < 0)
		return false;

	RemoveDebuggerJob job(team);
	return _DoJob(&job) == B_OK;
}


/*static*/ status_t
BDebugLooper::_MessageLoopEntry(void* data)
{
	return ((BDebugLooper*)data)->_MessageLoop();
}


status_t
BDebugLooper::_MessageLoop()
{
	while (true) {
		// prepare the wait info array
		int32 debuggerCount = fDebuggers.CountItems();
		object_wait_info waitInfos[debuggerCount + 1];

		for (int32 i = 0; i < debuggerCount; i++) {
			waitInfos[i].object
				= fDebuggers.ItemAt(i)->debugger->DebuggerPort();
			waitInfos[i].type = B_OBJECT_TYPE_PORT;
			waitInfos[i].events = B_EVENT_READ;
		}

		waitInfos[debuggerCount].object = fEventSemaphore;
		waitInfos[debuggerCount].type = B_OBJECT_TYPE_SEMAPHORE;
		waitInfos[debuggerCount].events = B_EVENT_ACQUIRE_SEMAPHORE;

		// wait for the next event
		wait_for_objects(waitInfos, debuggerCount + 1);

		AutoLocker<BLocker> locker(fLock);

		// handle all pending jobs
		bool handledJobs = fJobs->Head() != NULL;
		while (Job* job = fJobs->RemoveHead())
			job->Done(job->Do(this));

		// acquire notification semaphore and mark unnotified
		if ((waitInfos[debuggerCount].events & B_EVENT_ACQUIRE_SEMAPHORE) != 0)
			acquire_sem(fEventSemaphore);
		fNotified = false;

		if (fTerminating)
			return B_OK;

		// Always loop when jobs were executed, since that might add/remove
		// debuggers.
		if (handledJobs)
			continue;

		// read a pending port message
		for (int32 i = 0; i < debuggerCount; i++) {
			if ((waitInfos[i].events & B_EVENT_READ) != 0) {
				Debugger* debugger = fDebuggers.ItemAt(i);

				// read the message
				debug_debugger_message_data message;
				int32 code;
				ssize_t messageSize = read_port(
					debugger->debugger->DebuggerPort(), &code, &message,
					sizeof(message));
				if (messageSize < 0)
					continue;

				// handle the message
				bool continueThread = debugger->handler->HandleDebugMessage(
					code, message);

				// If requested, tell the thread to continue (only when there
				// is a thread and the message was synchronous).
				if (continueThread && message.origin.thread >= 0
						&& message.origin.nub_port >= 0) {
					debugger->debugger->ContinueThread(message.origin.thread);
				}

				// Handle only one message -- the hook might have added/removed
				// debuggers which makes further iteration problematic.
				break;
			}
		}
	}
}


status_t
BDebugLooper::_DoJob(Job* job)
{
	AutoLocker<BLocker> locker(fLock);

	// execute directly, if in looper thread or not running yet
	if (fThread < 0 || fThread == find_thread(NULL))
		return job->Do(this);

	// execute in the looper thread
	fJobs->Add(job);
	_Notify();

	return job->Wait(fLock);
}


void
BDebugLooper::_Notify()
{
	if (fNotified)
		return;

	fNotified = true;
	release_sem(fEventSemaphore);
}
