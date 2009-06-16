/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "TeamDebugger.h"

#include <stdio.h>

#include <new>

#include <Message.h>

#include <AutoLocker.h>

#include "Team.h"


TeamDebugger::TeamDebugger()
	:
	BLooper("team debugger"),
	fTeam(NULL),
	fTeamID(-1),
	fDebuggerPort(-1),
	fNubPort(-1),
	fDebugEventListener(-1),
	fTeamWindow(NULL),
	fTerminating(false)
{
}


TeamDebugger::~TeamDebugger()
{
	AutoLocker<BLooper> locker(this);

	fTerminating = true;

	if (fDebuggerPort >= 0)
		delete_port(fDebuggerPort);

	locker.Unlock();

	if (fDebugEventListener >= 0)
		wait_for_thread(fDebugEventListener, NULL);

	delete fTeam;
}


status_t
TeamDebugger::Init(team_id teamID, thread_id threadID, bool stopInMain)
{
	fTeamID = teamID;

	// check whether the team exists at all
	team_info teamInfo;
	status_t error = get_team_info(fTeamID, &teamInfo);
	if (error != B_OK)
		return error;

	// create a team object
	fTeam = new(std::nothrow) ::Team(fTeamID);
	if (fTeam == NULL)
		return B_NO_MEMORY;

	error = fTeam->Init();
	if (error != B_OK)
		return error;
	fTeam->SetName(teamInfo.args);
		// TODO: Set a better name!

	// create debugger port
	char buffer[128];
	snprintf(buffer, sizeof(buffer), "team %ld debugger", fTeamID);
	fDebuggerPort = create_port(100, buffer);
	if (fDebuggerPort < 0)
		return fDebuggerPort;

	// install as team debugger
	fNubPort = install_team_debugger(fTeamID, fDebuggerPort);
	if (fNubPort < 0)
		return fNubPort;

// TODO: Set the debug event flags!

	// get the initial state of the team
	AutoLocker< ::Team> teamLocker(fTeam);

	thread_info threadInfo;
	int32 cookie = 0;
	while (get_next_thread_info(fTeamID, &cookie, &threadInfo) == B_OK) {
		error = fTeam->AddThread(threadInfo);
		if (error != B_OK)
			return error;
	}

	image_info imageInfo;
	cookie = 0;
	while (get_next_image_info(fTeamID, &cookie, &imageInfo) == B_OK) {
		error = fTeam->AddImage(imageInfo);
		if (error != B_OK)
			return error;
	}

	// create the debug event listener
	snprintf(buffer, sizeof(buffer), "team %ld debug listener", fTeamID);
	fDebugEventListener = spawn_thread(_DebugEventListenerEntry, buffer,
		B_NORMAL_PRIORITY, this);
	if (fDebugEventListener < 0)
		return fDebugEventListener;

	resume_thread(fDebugEventListener);

	// run looper
	thread_id looperThread = Run();
	if (looperThread < 0)
		return looperThread;

	// create the team window
	try {
		fTeamWindow = TeamWindow::Create(fTeam, this);
	} catch (...) {
		// TODO: Notify the user!
		fprintf(stderr, "Error: Failed to create team window!\n");
		return B_NO_MEMORY;
	}

	fTeamWindow->Show();

	// if requested, stop the given thread
	if (threadID >= 0) {
		if (stopInMain) {
			// TODO: Set a temporary breakpoint in main and run the thread.
		} else {
			debug_thread(threadID);
				// TODO: Superfluous, if the thread is already stopped.
		}
	}

	return B_OK;
}


void
TeamDebugger::MessageReceived(BMessage* message)
{
	switch (message->what) {
		default:
			BLooper::MessageReceived(message);
			break;
	}
}


/*static*/ status_t
TeamDebugger::_DebugEventListenerEntry(void* data)
{
	return ((TeamDebugger*)data)->_DebugEventListener();
}


status_t
TeamDebugger::_DebugEventListener()
{
	while (!fTerminating) {
		// read the next message
		debug_debugger_message_data message;
		int32 messageCode;
		ssize_t size = read_port(fDebuggerPort, &messageCode, &message,
			sizeof(message));
		if (size < 0) {
			if (size == B_INTERRUPTED)
				continue;
// TODO: Error handling!
			break;
		}

		if (message.origin.team != fTeamID) {
printf("TeamDebugger for team %ld: received message from team %ld!\n", fTeamID,
message.origin.team);
			continue;
		}

		_HandleDebuggerMessage(messageCode, message);

		if (messageCode == B_DEBUGGER_MESSAGE_TEAM_DELETED
			|| messageCode == B_DEBUGGER_MESSAGE_TEAM_EXEC) {
			// TODO:...
			break;
		}
	}

	return B_OK;
}


void
TeamDebugger::_HandleDebuggerMessage(int32 messageCode,
	const debug_debugger_message_data& message)
{
	switch (messageCode) {
		case B_DEBUGGER_MESSAGE_THREAD_DEBUGGED:
			break;
		case B_DEBUGGER_MESSAGE_DEBUGGER_CALL:
			break;
		case B_DEBUGGER_MESSAGE_BREAKPOINT_HIT:
			break;
		case B_DEBUGGER_MESSAGE_WATCHPOINT_HIT:
			break;
		case B_DEBUGGER_MESSAGE_SINGLE_STEP:
			break;
		case B_DEBUGGER_MESSAGE_EXCEPTION_OCCURRED:
			break;
		case B_DEBUGGER_MESSAGE_TEAM_CREATED:
			break;
		case B_DEBUGGER_MESSAGE_TEAM_DELETED:
			break;
		case B_DEBUGGER_MESSAGE_TEAM_EXEC:
			break;
		case B_DEBUGGER_MESSAGE_THREAD_CREATED:
			break;
		case B_DEBUGGER_MESSAGE_THREAD_DELETED:
			break;
		case B_DEBUGGER_MESSAGE_IMAGE_CREATED:
			break;
		case B_DEBUGGER_MESSAGE_IMAGE_DELETED:
			break;
		case B_DEBUGGER_MESSAGE_PRE_SYSCALL:
		case B_DEBUGGER_MESSAGE_POST_SYSCALL:
		case B_DEBUGGER_MESSAGE_SIGNAL_RECEIVED:
		case B_DEBUGGER_MESSAGE_PROFILER_UPDATE:
		case B_DEBUGGER_MESSAGE_HANDED_OVER:
			// not interested
			break;
		default:
			printf("TeamDebugger for team %ld: unknown message from kernel: "
				"%ld\n", fTeamID, messageCode);
			break;
	}
}
