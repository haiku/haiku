/*
 * Copyright 2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "CliContext.h"

#include <AutoDeleter.h>
#include <AutoLocker.h>

#include "UserInterface.h"


// NOTE: This is a simple work-around for EditLine not having any kind of user
// data field. Hence in _GetPrompt() we don't have access to the context object.
// ATM only one CLI is possible in Debugger, so a static variable works well
// enough. Should that ever change, we would need a thread-safe
// EditLine* -> CliContext* map.
static CliContext* sCurrentContext;


// #pragma mark - Event


struct CliContext::Event : DoublyLinkedListLinkImpl<CliContext::Event> {
	Event(int type, Thread* thread = NULL)
		:
		fType(type),
		fThreadReference(thread)
	{
	}

	int Type() const
	{
		return fType;
	}

	Thread* GetThread() const
	{
		return fThreadReference.Get();
	}

private:
	int					fType;
	BReference<Thread>	fThreadReference;
};


// #pragma mark - CliContext


CliContext::CliContext()
	:
	fLock("CliContext"),
	fTeam(NULL),
	fListener(NULL),
	fEditLine(NULL),
	fHistory(NULL),
	fPrompt(NULL),
	fBlockingSemaphore(-1),
	fInputLoopWaitingForEvents(0),
	fEventsOccurred(0),
	fInputLoopWaiting(false),
	fTerminating(false),
	fCurrentThread(NULL)
{
	sCurrentContext = this;
}


CliContext::~CliContext()
{
	Cleanup();
	sCurrentContext = NULL;

	if (fBlockingSemaphore >= 0)
		delete_sem(fBlockingSemaphore);
}


status_t
CliContext::Init(Team* team, UserInterfaceListener* listener)
{
	fTeam = team;
	fListener = listener;

	fTeam->AddListener(this);

	status_t error = fLock.InitCheck();
	if (error != B_OK)
		return error;

	fBlockingSemaphore = create_sem(0, "CliContext block");
	if (fBlockingSemaphore < 0)
		return fBlockingSemaphore;

	fEditLine = el_init("Debugger", stdin, stdout, stderr);
	if (fEditLine == NULL)
		return B_ERROR;

	fHistory = history_init();
	if (fHistory == NULL)
		return B_ERROR;

	HistEvent historyEvent;
	history(fHistory, &historyEvent, H_SETSIZE, 100);

	el_set(fEditLine, EL_HIST, &history, fHistory);
	el_set(fEditLine, EL_EDITOR, "emacs");
	el_set(fEditLine, EL_PROMPT, &_GetPrompt);

	return B_OK;
}


void
CliContext::Cleanup()
{
	Terminating();

	while (Event* event = fPendingEvents.RemoveHead())
		delete event;

	if (fEditLine != NULL) {
		el_end(fEditLine);
		fEditLine = NULL;
	}

	if (fHistory != NULL) {
		history_end(fHistory);
		fHistory = NULL;
	}

	if (fTeam != NULL) {
		fTeam->RemoveListener(this);
		fTeam = NULL;
	}
}


void
CliContext::Terminating()
{
	AutoLocker<BLocker> locker(fLock);

	fTerminating = true;
	_SignalInputLoop(EVENT_QUIT);

	// TODO: Signal the input loop, should it be in PromptUser()!
}


thread_id
CliContext::CurrentThreadID() const
{
	return fCurrentThread != NULL ? fCurrentThread->ID() : -1;
}


void
CliContext::SetCurrentThread(Thread* thread)
{
	AutoLocker<BLocker> locker(fLock);

	if (fCurrentThread != NULL)
		fCurrentThread->ReleaseReference();

	fCurrentThread = thread;

	if (fCurrentThread != NULL)
		fCurrentThread->AcquireReference();
}


void
CliContext::PrintCurrentThread()
{
	AutoLocker<Team> teamLocker(fTeam);

	if (fCurrentThread != NULL) {
		printf("current thread: %" B_PRId32 " \"%s\"\n", fCurrentThread->ID(),
			fCurrentThread->Name());
	} else
		printf("no current thread\n");
}


const char*
CliContext::PromptUser(const char* prompt)
{
	fPrompt = prompt;

	int count;
	const char* line = el_gets(fEditLine, &count);

	fPrompt = NULL;

	ProcessPendingEvents();

	return line;
}


void
CliContext::AddLineToInputHistory(const char* line)
{
	HistEvent historyEvent;
	history(fHistory, &historyEvent, H_ENTER, line);
}


void
CliContext::QuitSession(bool killTeam)
{
	_PrepareToWaitForEvents(EVENT_QUIT);

	fListener->UserInterfaceQuitRequested(
		killTeam
			? UserInterfaceListener::QUIT_OPTION_ASK_KILL_TEAM
			: UserInterfaceListener::QUIT_OPTION_ASK_RESUME_TEAM);

	_WaitForEvents();
}


void
CliContext::WaitForThreadOrUser()
{
	ProcessPendingEvents();

// TODO: Deal with SIGINT as well!
	for (;;) {
		_PrepareToWaitForEvents(
			EVENT_USER_INTERRUPT | EVENT_THREAD_STOPPED);

		// check whether there are any threads stopped already
		Thread* stoppedThread = NULL;
		BReference<Thread> stoppedThreadReference;

		AutoLocker<Team> teamLocker(fTeam);

		for (ThreadList::ConstIterator it = fTeam->Threads().GetIterator();
				Thread* thread = it.Next();) {
			if (thread->State() == THREAD_STATE_STOPPED) {
				stoppedThread = thread;
				stoppedThreadReference.SetTo(thread);
				break;
			}
		}

		teamLocker.Unlock();

		if (stoppedThread != NULL) {
			if (fCurrentThread == NULL)
				fCurrentThread = stoppedThread;

			_SignalInputLoop(EVENT_THREAD_STOPPED);
		}

		uint32 events = _WaitForEvents();
		if ((events & EVENT_QUIT) != 0 || stoppedThread != NULL) {
			ProcessPendingEvents();
			return;
		}
	}
}


void
CliContext::ProcessPendingEvents()
{
	AutoLocker<Team> teamLocker(fTeam);

	for (;;) {
		// get the next event
		AutoLocker<BLocker> locker(fLock);
		Event* event = fPendingEvents.RemoveHead();
		locker.Unlock();
		if (event == NULL)
			break;
		ObjectDeleter<Event> eventDeleter(event);

		// process the event
		Thread* thread = event->GetThread();

		switch (event->Type()) {
			case EVENT_QUIT:
			case EVENT_USER_INTERRUPT:
				break;
			case EVENT_THREAD_ADDED:
				printf("[new thread: %" B_PRId32 " \"%s\"]\n", thread->ID(),
					thread->Name());
				break;
			case EVENT_THREAD_REMOVED:
				printf("[thread terminated: %" B_PRId32 " \"%s\"]\n",
					thread->ID(), thread->Name());
				break;
			case EVENT_THREAD_STOPPED:
				printf("[thread stopped: %" B_PRId32 " \"%s\"]\n",
					thread->ID(), thread->Name());
				break;
		}
	}
}


void
CliContext::ThreadAdded(const Team::ThreadEvent& threadEvent)
{
	_QueueEvent(
		new(std::nothrow) Event(EVENT_THREAD_ADDED, threadEvent.GetThread()));
	_SignalInputLoop(EVENT_THREAD_ADDED);
}


void
CliContext::ThreadRemoved(const Team::ThreadEvent& threadEvent)
{
	_QueueEvent(
		new(std::nothrow) Event(EVENT_THREAD_REMOVED, threadEvent.GetThread()));
	_SignalInputLoop(EVENT_THREAD_REMOVED);
}


void
CliContext::ThreadStateChanged(const Team::ThreadEvent& threadEvent)
{
	if (threadEvent.GetThread()->State() != THREAD_STATE_STOPPED)
		return;

	_QueueEvent(
		new(std::nothrow) Event(EVENT_THREAD_STOPPED, threadEvent.GetThread()));
	_SignalInputLoop(EVENT_THREAD_STOPPED);
}


void
CliContext::_QueueEvent(Event* event)
{
	if (event == NULL) {
		// no memory -- can't do anything about it
		return;
	}

	AutoLocker<BLocker> locker(fLock);
	fPendingEvents.Add(event);
}


void
CliContext::_PrepareToWaitForEvents(uint32 eventMask)
{
	// Set the events we're going to wait for -- always wait for "quit".
	AutoLocker<BLocker> locker(fLock);
	fInputLoopWaitingForEvents = eventMask | EVENT_QUIT;
	fEventsOccurred = fTerminating ? EVENT_QUIT : 0;
}


uint32
CliContext::_WaitForEvents()
{
	AutoLocker<BLocker> locker(fLock);

	if (fEventsOccurred == 0) {
		sem_id blockingSemaphore = fBlockingSemaphore;
		fInputLoopWaiting = true;

		locker.Unlock();

		while (acquire_sem(blockingSemaphore) == B_INTERRUPTED) {
		}

		locker.Lock();
	}

	uint32 events = fEventsOccurred;
	fEventsOccurred = 0;
	return events;
}


void
CliContext::_SignalInputLoop(uint32 events)
{
	AutoLocker<BLocker> locker(fLock);

	if ((fInputLoopWaitingForEvents & events) == 0)
		return;

	fEventsOccurred = fInputLoopWaitingForEvents & events;
	fInputLoopWaitingForEvents = 0;

	if (fInputLoopWaiting) {
		fInputLoopWaiting = false;
		release_sem(fBlockingSemaphore);
	}
}


/*static*/ const char*
CliContext::_GetPrompt(EditLine* editLine)
{
	return sCurrentContext != NULL ? sCurrentContext->fPrompt : NULL;
}
