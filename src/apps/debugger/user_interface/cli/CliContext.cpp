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
	fInputLoopWaiting(false),
	fTerminating(false)
{
	sCurrentContext = this;
}

CliContext::~CliContext()
{
	Cleanup();
	sCurrentContext = NULL;
}


status_t
CliContext::Init(Team* team, UserInterfaceListener* listener)
{
	fTeam = team;
	fListener = listener;

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
}


void
CliContext::Terminating()
{
	AutoLocker<BLocker> locker(fLock);

	fTerminating = true;

	if (fBlockingSemaphore >= 0) {
		delete_sem(fBlockingSemaphore);
		fBlockingSemaphore = -1;
	}

	fInputLoopWaiting = false;

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
	AutoLocker<BLocker> locker(fLock);

	sem_id blockingSemaphore = fBlockingSemaphore;
	fInputLoopWaiting = true;

	locker.Unlock();

	fListener->UserInterfaceQuitRequested(
		killTeam
			? UserInterfaceListener::QUIT_OPTION_ASK_KILL_TEAM
			: UserInterfaceListener::QUIT_OPTION_ASK_RESUME_TEAM);

	while (acquire_sem(blockingSemaphore) == B_INTERRUPTED) {
	}
}


void
CliContext::WaitForThreadOrUser()
{
	// TODO:...
}


/*static*/ const char*
CliContext::_GetPrompt(EditLine* editLine)
{
	return sCurrentContext != NULL ? sCurrentContext->fPrompt : NULL;
}
