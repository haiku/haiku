/*
 * Copyright 2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "CliContext.h"

#include <AutoLocker.h>

#include "UserInterface.h"


// NOTE: This is a simple work-around for EditLine not having any kind of user
// data field. Hence in _GetPrompt() we don't have access to the context object.
// ATM only one CLI is possible in Debugger, so a static variable works well
// enough. Should that ever change, we would need a thread-safe
// EditLine* -> CliContext* map.
static CliContext* sCurrentContext;


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
	fTerminating(false)
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


const char*
CliContext::PromptUser(const char* prompt)
{
	fPrompt = prompt;

	int count;
	const char* line = el_gets(fEditLine, &count);

	fPrompt = NULL;

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
// TODO: Deal with SIGINT as well!
	for (;;) {
		_PrepareToWaitForEvents(
			EVENT_USER_INTERRUPT | EVENT_THREAD_STATE_CHANGED);

		// check whether there are any threads stopped already
		thread_id stoppedThread = -1;
		AutoLocker<Team> teamLocker(fTeam);

		for (ThreadList::ConstIterator it = fTeam->Threads().GetIterator();
				Thread* thread = it.Next();) {
			if (thread->State() == THREAD_STATE_STOPPED) {
				stoppedThread = thread->ID();
				break;
			}
		}

		teamLocker.Unlock();

		if (stoppedThread >= 0)
			_SignalInputLoop(EVENT_THREAD_STATE_CHANGED);

		uint32 events = _WaitForEvents();
		if ((events & EVENT_QUIT) != 0 || stoppedThread >= 0)
			return;
	}
}


void
CliContext::ThreadStateChanged(const Team::ThreadEvent& event)
{
	_SignalInputLoop(EVENT_THREAD_STATE_CHANGED);
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
