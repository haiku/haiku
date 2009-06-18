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

#include "CpuState.h"
#include "DebuggerInterface.h"
#include "Team.h"
#include "TeamDebugModel.h"


TeamDebugger::TeamDebugger()
	:
	BLooper("team debugger"),
	fTeam(NULL),
	fDebugModel(NULL),
	fTeamID(-1),
	fDebuggerInterface(NULL),
	fDebugEventListener(-1),
	fTeamWindow(NULL),
	fTerminating(false)
{
}


TeamDebugger::~TeamDebugger()
{
	AutoLocker<BLooper> locker(this);

	fTerminating = true;

	fDebuggerInterface->Close();

	locker.Unlock();

	if (fDebugEventListener >= 0)
		wait_for_thread(fDebugEventListener, NULL);

	delete fDebugModel;
	delete fTeam;
	delete fDebuggerInterface;
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

	// create debugger interface
	fDebuggerInterface = new(std::nothrow) DebuggerInterface(fTeamID);
	if (fDebuggerInterface == NULL)
		return B_NO_MEMORY;

	error = fDebuggerInterface->Init();
	if (error != B_OK)
		return error;

	// set team debugging flags
	fDebuggerInterface->SetTeamDebuggingFlags(
		B_TEAM_DEBUG_THREADS | B_TEAM_DEBUG_IMAGES);

	// get the initial state of the team
	AutoLocker< ::Team> teamLocker(fTeam);

	{
		BObjectList<ThreadInfo> threadInfos(20, true);
		status_t error = fDebuggerInterface->GetThreadInfos(threadInfos);
		for (int32 i = 0; ThreadInfo* info = threadInfos.ItemAt(i); i++) {
			::Thread* thread;
			error = fTeam->AddThread(*info, &thread);
			if (error != B_OK)
				return error;

			_UpdateThreadState(thread);
		}
	}

	{
		BObjectList<ImageInfo> imageInfos(20, true);
		status_t error = fDebuggerInterface->GetImageInfos(imageInfos);
		for (int32 i = 0; ImageInfo* info = imageInfos.ItemAt(i); i++) {
			error = fTeam->AddImage(*info);
			if (error != B_OK)
				return error;
		}
	}

	// create the debug event listener
	char buffer[128];
	snprintf(buffer, sizeof(buffer), "team %ld debug listener", fTeamID);
	fDebugEventListener = spawn_thread(_DebugEventListenerEntry, buffer,
		B_NORMAL_PRIORITY, this);
//	if (fDebugEventListener < 0)
	if (fDebugEventListener < NULL)
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
		// get the next event
		DebugEvent* event;
		status_t error = fDebuggerInterface->GetNextDebugEvent(event);
		if (error != B_OK)
			break;
				// TODO: Error handling!


		if (event->Team() != fTeamID) {
printf("TeamDebugger for team %ld: received event from team %ld!\n", fTeamID,
event->Team());
			continue;
		}

		_HandleDebuggerMessage(event);

//		if (event->EventType() == B_DEBUGGER_MESSAGE_TEAM_DELETED
//			|| event->EventType() == B_DEBUGGER_MESSAGE_TEAM_EXEC) {
//			// TODO:...
//			break;
//		}
	}

	return B_OK;
}


void
TeamDebugger::_HandleDebuggerMessage(DebugEvent* event)
{
printf("TeamDebugger::_HandleDebuggerMessage(): %d\n", event->EventType());
	bool handled = false;

	switch (event->EventType()) {
		case B_DEBUGGER_MESSAGE_THREAD_DEBUGGED:
printf("B_DEBUGGER_MESSAGE_THREAD_DEBUGGED: thread: %ld\n", event->Thread());
			break;
		case B_DEBUGGER_MESSAGE_DEBUGGER_CALL:
printf("B_DEBUGGER_MESSAGE_DEBUGGER_CALL: thread: %ld\n", event->Thread());
			break;
		case B_DEBUGGER_MESSAGE_BREAKPOINT_HIT:
printf("B_DEBUGGER_MESSAGE_BREAKPOINT_HIT: thread: %ld\n", event->Thread());
			break;
		case B_DEBUGGER_MESSAGE_WATCHPOINT_HIT:
printf("B_DEBUGGER_MESSAGE_WATCHPOINT_HIT: thread: %ld\n", event->Thread());
			break;
		case B_DEBUGGER_MESSAGE_SINGLE_STEP:
printf("B_DEBUGGER_MESSAGE_SINGLE_STEP: thread: %ld\n", event->Thread());
			break;
		case B_DEBUGGER_MESSAGE_EXCEPTION_OCCURRED:
printf("B_DEBUGGER_MESSAGE_EXCEPTION_OCCURRED: thread: %ld\n", event->Thread());
			break;
//		case B_DEBUGGER_MESSAGE_TEAM_CREATED:
//printf("B_DEBUGGER_MESSAGE_TEAM_CREATED: team: %ld\n", message.team_created.new_team);
//			break;
		case B_DEBUGGER_MESSAGE_TEAM_DELETED:
printf("B_DEBUGGER_MESSAGE_TEAM_DELETED: team: %ld\n", event->Team());
			break;
		case B_DEBUGGER_MESSAGE_TEAM_EXEC:
printf("B_DEBUGGER_MESSAGE_TEAM_EXEC: team: %ld\n", event->Team());
			break;
		case B_DEBUGGER_MESSAGE_THREAD_CREATED:
			handled = _HandleThreadCreated(
				dynamic_cast<ThreadCreatedEvent*>(event));
			break;
		case B_DEBUGGER_MESSAGE_THREAD_DELETED:
			handled = _HandleThreadDeleted(
				dynamic_cast<ThreadDeletedEvent*>(event));
			break;
		case B_DEBUGGER_MESSAGE_IMAGE_CREATED:
			handled = _HandleImageCreated(
				dynamic_cast<ImageCreatedEvent*>(event));
			break;
		case B_DEBUGGER_MESSAGE_IMAGE_DELETED:
			handled = _HandleImageDeleted(
				dynamic_cast<ImageDeletedEvent*>(event));
			break;
		case B_DEBUGGER_MESSAGE_PRE_SYSCALL:
		case B_DEBUGGER_MESSAGE_POST_SYSCALL:
		case B_DEBUGGER_MESSAGE_SIGNAL_RECEIVED:
		case B_DEBUGGER_MESSAGE_PROFILER_UPDATE:
		case B_DEBUGGER_MESSAGE_HANDED_OVER:
			// not interested
			break;
		default:
			printf("TeamDebugger for team %ld: unknown event type: "
				"%d\n", fTeamID, event->EventType());
			break;
	}

	if (!handled && event->ThreadStopped())
		fDebuggerInterface->ContinueThread(event->Thread());
}


bool
TeamDebugger::_HandleThreadCreated(ThreadCreatedEvent* event)
{
	AutoLocker< ::Team> locker(fTeam);

	ThreadInfo info;
	status_t error = fDebuggerInterface->GetThreadInfo(event->NewThread(),
		info);
	if (error == B_OK)
		fTeam->AddThread(info);

	return false;
}


bool
TeamDebugger::_HandleThreadDeleted(ThreadDeletedEvent* event)
{
	AutoLocker< ::Team> locker(fTeam);
	fTeam->RemoveThread(event->Thread());
	return false;
}


bool
TeamDebugger::_HandleImageCreated(ImageCreatedEvent* event)
{
	AutoLocker< ::Team> locker(fTeam);
	fTeam->AddImage(event->GetImageInfo());
	return false;
}


bool
TeamDebugger::_HandleImageDeleted(ImageDeletedEvent* event)
{
	AutoLocker< ::Team> locker(fTeam);
	fTeam->RemoveImage(event->GetImageInfo().ImageID());
	return false;
}


void
TeamDebugger::_UpdateThreadState(::Thread* thread)
{
	CpuState* state = NULL;
	status_t error = fDebuggerInterface->GetCpuState(thread->ID(), state);

	uint32 newState = THREAD_STATE_UNKNOWN;
	if (error == B_OK) {
		newState = THREAD_STATE_STOPPED;
		state->RemoveReference();
	} else if (error == B_BAD_THREAD_STATE)
		newState = THREAD_STATE_RUNNING;

	thread->SetState(newState);
}
