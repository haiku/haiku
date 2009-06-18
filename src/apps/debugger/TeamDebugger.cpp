/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "TeamDebugger.h"

#include <stdio.h>

#include <new>

#include <Message.h>

#include <AutoLocker.h>

#include "debug_utils.h"

#include "Team.h"
#include "TeamDebugModel.h"


TeamDebugger::TeamDebugger()
	:
	BLooper("team debugger"),
	fTeam(NULL),
	fDebugModel(NULL),
	fTeamID(-1),
	fDebuggerPort(-1),
	fNubPort(-1),
	fDebugEventListener(-1),
	fTeamWindow(NULL),
	fTerminating(false)
{
	fDebugContext.reply_port = -1;
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

	destroy_debug_context(&fDebugContext);

	delete fDebugModel;
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

	// create the team debug model
	fDebugModel = new(std::nothrow) TeamDebugModel(fTeam);
	if (fDebugModel == NULL)
		return B_NO_MEMORY;

	error = fDebugModel->Init();
	if (error != B_OK)
		return error;

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

	// init debug context
	error = init_debug_context(&fDebugContext, fTeamID, fNubPort);
	if (error != B_OK)
		return error;

	// set team debugging flags
	set_team_debugging_flags(fNubPort,
		B_TEAM_DEBUG_THREADS | B_TEAM_DEBUG_IMAGES);

	// get the initial state of the team
	AutoLocker< ::Team> teamLocker(fTeam);

	thread_info threadInfo;
	int32 cookie = 0;
	while (get_next_thread_info(fTeamID, &cookie, &threadInfo) == B_OK) {
		::Thread* thread;
		error = fTeam->AddThread(threadInfo, &thread);
		if (error != B_OK)
			return error;

		_UpdateThreadState(thread);
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
		fTeamWindow = TeamWindow::Create(fDebugModel, this);
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
printf("TeamDebugger::_HandleDebuggerMessage(): %ld\n", messageCode);
	bool handled = false;

	switch (messageCode) {
		case B_DEBUGGER_MESSAGE_THREAD_DEBUGGED:
printf("B_DEBUGGER_MESSAGE_THREAD_DEBUGGED: thread: %ld\n", message.origin.thread);
			break;
		case B_DEBUGGER_MESSAGE_DEBUGGER_CALL:
printf("B_DEBUGGER_MESSAGE_DEBUGGER_CALL: thread: %ld\n", message.origin.thread);
			break;
		case B_DEBUGGER_MESSAGE_BREAKPOINT_HIT:
printf("B_DEBUGGER_MESSAGE_BREAKPOINT_HIT: thread: %ld\n", message.origin.thread);
			break;
		case B_DEBUGGER_MESSAGE_WATCHPOINT_HIT:
printf("B_DEBUGGER_MESSAGE_WATCHPOINT_HIT: thread: %ld\n", message.origin.thread);
			break;
		case B_DEBUGGER_MESSAGE_SINGLE_STEP:
printf("B_DEBUGGER_MESSAGE_SINGLE_STEP: thread: %ld\n", message.origin.thread);
			break;
		case B_DEBUGGER_MESSAGE_EXCEPTION_OCCURRED:
printf("B_DEBUGGER_MESSAGE_EXCEPTION_OCCURRED: thread: %ld\n", message.origin.thread);
			break;
		case B_DEBUGGER_MESSAGE_TEAM_CREATED:
printf("B_DEBUGGER_MESSAGE_TEAM_CREATED: team: %ld\n", message.team_created.new_team);
			break;
		case B_DEBUGGER_MESSAGE_TEAM_DELETED:
printf("B_DEBUGGER_MESSAGE_TEAM_DELETED: team: %ld\n", message.origin.team);
			break;
		case B_DEBUGGER_MESSAGE_TEAM_EXEC:
printf("B_DEBUGGER_MESSAGE_TEAM_EXEC: team: %ld\n", message.origin.team);
			break;
		case B_DEBUGGER_MESSAGE_THREAD_CREATED:
			handled = _HandleThreadCreated(message.thread_created);
			break;
		case B_DEBUGGER_MESSAGE_THREAD_DELETED:
			handled = _HandleThreadDeleted(message.thread_deleted);
			break;
		case B_DEBUGGER_MESSAGE_IMAGE_CREATED:
			handled = _HandleImageCreated(message.image_created);
			break;
		case B_DEBUGGER_MESSAGE_IMAGE_DELETED:
			handled = _HandleImageDeleted(message.image_deleted);
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

	if (!handled && message.origin.thread >= 0 && message.origin.nub_port >= 0)
		continue_thread(message.origin.nub_port, message.origin.thread);
}


bool
TeamDebugger::_HandleThreadCreated(const debug_thread_created& message)
{
	AutoLocker< ::Team> locker(fTeam);
	fTeam->AddThread(message.new_thread);
	return false;
}


bool
TeamDebugger::_HandleThreadDeleted(const debug_thread_deleted& message)
{
	AutoLocker< ::Team> locker(fTeam);
	fTeam->RemoveThread(message.origin.thread);
	return false;
}


bool
TeamDebugger::_HandleImageCreated(const debug_image_created& message)
{
	AutoLocker< ::Team> locker(fTeam);
	fTeam->AddImage(message.info);
	return false;
}


bool
TeamDebugger::_HandleImageDeleted(const debug_image_deleted& message)
{
	AutoLocker< ::Team> locker(fTeam);
	fTeam->RemoveImage(message.info.id);
	return false;
}


void
TeamDebugger::_UpdateThreadState(::Thread* thread)
{
	debug_nub_get_cpu_state message;
	message.reply_port = fDebugContext.reply_port;
	message.thread = thread->ID();

	debug_nub_get_cpu_state_reply reply;

	status_t error = send_debug_message(&fDebugContext,
		B_DEBUG_MESSAGE_GET_CPU_STATE, &message, sizeof(message), &reply,
		sizeof(reply));

	uint32 newState = THREAD_STATE_UNKNOWN;
	if (error == B_OK) {
		if (reply.error == B_OK)
			newState = THREAD_STATE_STOPPED;
		else if (reply.error == B_BAD_THREAD_STATE)
			newState = THREAD_STATE_RUNNING;
	}

	thread->SetState(newState);
}
