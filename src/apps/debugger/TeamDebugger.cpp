/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "TeamDebugger.h"

#include <stdarg.h>
#include <stdio.h>

#include <new>

#include <Alert.h>
#include <Message.h>

#include <AutoLocker.h>

#include "debug_utils.h"

#include "CpuState.h"
#include "DebuggerInterface.h"
#include "Jobs.h"
#include "MessageCodes.h"
#include "Statement.h"
#include "TeamDebugModel.h"


TeamDebugger::TeamDebugger()
	:
	BLooper("team debugger"),
	fTeam(NULL),
	fDebugModel(NULL),
	fTeamID(-1),
	fDebuggerInterface(NULL),
	fWorker(NULL),
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
	fWorker->ShutDown();

	locker.Unlock();

	if (fDebugEventListener >= 0)
		wait_for_thread(fDebugEventListener, NULL);

	delete fDebuggerInterface;
	delete fWorker;
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

	fTeam->AddListener(this);

	// create our worker
	fWorker = new(std::nothrow) Worker;
	if (fWorker == NULL)
		return B_NO_MEMORY;

	error = fWorker->Init();
	if (error != B_OK)
		return error;

	// create debugger interface
	fDebuggerInterface = new(std::nothrow) DebuggerInterface(fTeamID);
	if (fDebuggerInterface == NULL)
		return B_NO_MEMORY;

	error = fDebuggerInterface->Init();
	if (error != B_OK)
		return error;

	// create the team debug model
	fDebugModel = new(std::nothrow) TeamDebugModel(fTeam, fDebuggerInterface,
		fDebuggerInterface->GetArchitecture());
	if (fDebugModel == NULL)
		return B_NO_MEMORY;

	error = fDebugModel->Init();
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
		case MSG_THREAD_RUN:
		case MSG_THREAD_STOP:
		case MSG_THREAD_STEP_OVER:
		case MSG_THREAD_STEP_INTO:
		case MSG_THREAD_STEP_OUT:
		{
			int32 threadID;
			if (message->FindInt32("thread", &threadID) != B_OK)
				break;

			_HandleThreadAction(threadID, message->what);
			break;
		}

		case MSG_SET_BREAKPONT:
		case MSG_CLEAR_BREAKPONT:
		{
			uint64 address;
			if (message->FindUInt64("address", &address) != B_OK)
				break;

			if (message->what == MSG_SET_BREAKPONT) {
				bool enabled;
				if (message->FindBool("enabled", &enabled) != B_OK)
					enabled = true;

				_HandleSetUserBreakpoint(address, enabled);
			} else
				_HandleClearUserBreakpoint(address);
			break;
		}

		case MSG_THREAD_STATE_CHANGED:
		{
			int32 threadID;
			if (message->FindInt32("thread", &threadID) != B_OK)
				break;

			_HandleThreadStateChanged(threadID);
			break;
		}
		case MSG_THREAD_CPU_STATE_CHANGED:
		{
			int32 threadID;
			if (message->FindInt32("thread", &threadID) != B_OK)
				break;

			_HandleCpuStateChanged(threadID);
			break;
		}
		case MSG_THREAD_STACK_TRACE_CHANGED:
		{
			int32 threadID;
			if (message->FindInt32("thread", &threadID) != B_OK)
				break;

			_HandleStackTraceChanged(threadID);
			break;
		}

		default:
			BLooper::MessageReceived(message);
			break;
	}
}


void
TeamDebugger::StackFrameSourceCodeRequested(TeamWindow* window,
	StackFrame* frame)
{
	// mark loading
	AutoLocker< ::Team> locker(fTeam);
	if (frame->SourceCodeState() != STACK_SOURCE_NOT_LOADED)
		return;
	frame->SetSourceCode(NULL, STACK_SOURCE_LOADING);
	locker.Unlock();

	// schedule the job
	if (fWorker->ScheduleJob(
			new(std::nothrow) LoadSourceCodeJob(fDebuggerInterface,
				fDebuggerInterface->GetArchitecture(), fTeam, frame),
			this) != B_OK) {
		// scheduling failed -- mark unavailable
		locker.Lock();
		frame->SetSourceCode(NULL, STACK_SOURCE_UNAVAILABLE);
		locker.Unlock();
	}
}


void
TeamDebugger::ThreadActionRequested(TeamWindow* window, thread_id threadID,
	uint32 action)
{
	BMessage message(action);
	message.AddInt32("thread", threadID);
	PostMessage(&message);
}


void
TeamDebugger::SetBreakpointRequested(target_addr_t address, bool enabled)
{
	BMessage message(MSG_SET_BREAKPONT);
	message.AddUInt64("address", (uint64)address);
	message.AddBool("enabled", enabled);
	PostMessage(&message);
}


void
TeamDebugger::ClearBreakpointRequested(target_addr_t address)
{
	BMessage message(MSG_CLEAR_BREAKPONT);
	message.AddUInt64("address", (uint64)address);
	PostMessage(&message);
}


bool
TeamDebugger::TeamWindowQuitRequested(TeamWindow* window)
{
	// TODO:...
	return true;
}


void
TeamDebugger::JobDone(Job* job)
{
printf("TeamDebugger::JobDone(%p)\n", job);
}


void
TeamDebugger::JobFailed(Job* job)
{
printf("TeamDebugger::JobFailed(%p)\n", job);
}


void
TeamDebugger::JobAborted(Job* job)
{
printf("TeamDebugger::JobAborted(%p)\n", job);
	// TODO: For a stack frame source loader thread we should reset the
	// loading state! Asynchronously due to locking order.
}


void
TeamDebugger::ThreadStateChanged(const ::Team::ThreadEvent& event)
{
	BMessage message(MSG_THREAD_STATE_CHANGED);
	message.AddInt32("thread", event.GetThread()->ID());
	PostMessage(&message);
}


void
TeamDebugger::ThreadCpuStateChanged(const ::Team::ThreadEvent& event)
{
	BMessage message(MSG_THREAD_CPU_STATE_CHANGED);
	message.AddInt32("thread", event.GetThread()->ID());
	PostMessage(&message);
}


void
TeamDebugger::ThreadStackTraceChanged(const ::Team::ThreadEvent& event)
{
	BMessage message(MSG_THREAD_STACK_TRACE_CHANGED);
	message.AddInt32("thread", event.GetThread()->ID());
	PostMessage(&message);
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
			handled = _HandleThreadDebugged(
				dynamic_cast<ThreadDebuggedEvent*>(event));
			break;
		case B_DEBUGGER_MESSAGE_DEBUGGER_CALL:
printf("B_DEBUGGER_MESSAGE_DEBUGGER_CALL: thread: %ld\n", event->Thread());
			handled = _HandleDebuggerCall(
				dynamic_cast<DebuggerCallEvent*>(event));
			break;
		case B_DEBUGGER_MESSAGE_BREAKPOINT_HIT:
printf("B_DEBUGGER_MESSAGE_BREAKPOINT_HIT: thread: %ld\n", event->Thread());
			handled = _HandleBreakpointHit(
				dynamic_cast<BreakpointHitEvent*>(event));
			break;
		case B_DEBUGGER_MESSAGE_WATCHPOINT_HIT:
printf("B_DEBUGGER_MESSAGE_WATCHPOINT_HIT: thread: %ld\n", event->Thread());
			handled = _HandleWatchpointHit(
				dynamic_cast<WatchpointHitEvent*>(event));
			break;
		case B_DEBUGGER_MESSAGE_SINGLE_STEP:
printf("B_DEBUGGER_MESSAGE_SINGLE_STEP: thread: %ld\n", event->Thread());
			handled = _HandleSingleStep(dynamic_cast<SingleStepEvent*>(event));
			break;
		case B_DEBUGGER_MESSAGE_EXCEPTION_OCCURRED:
printf("B_DEBUGGER_MESSAGE_EXCEPTION_OCCURRED: thread: %ld\n", event->Thread());
			handled = _HandleExceptionOccurred(
				dynamic_cast<ExceptionOccurredEvent*>(event));
			break;
//		case B_DEBUGGER_MESSAGE_TEAM_CREATED:
//printf("B_DEBUGGER_MESSAGE_TEAM_CREATED: team: %ld\n", message.team_created.new_team);
//			break;
		case B_DEBUGGER_MESSAGE_TEAM_DELETED:
			// TODO: Handle!
printf("B_DEBUGGER_MESSAGE_TEAM_DELETED: team: %ld\n", event->Team());
			break;
		case B_DEBUGGER_MESSAGE_TEAM_EXEC:
printf("B_DEBUGGER_MESSAGE_TEAM_EXEC: team: %ld\n", event->Team());
			// TODO: Handle!
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
TeamDebugger::_HandleThreadStopped(thread_id threadID, CpuState* cpuState)
{
	// get the thread
	AutoLocker< ::Team> locker(fTeam);
	::Thread* thread = fTeam->ThreadByID(threadID);
	if (thread == NULL)
		return false;

	_SetThreadState(thread, THREAD_STATE_STOPPED, cpuState);

	return true;
}


bool
TeamDebugger::_HandleThreadDebugged(ThreadDebuggedEvent* event)
{
	return _HandleThreadStopped(event->Thread(), NULL);
}


bool
TeamDebugger::_HandleDebuggerCall(DebuggerCallEvent* event)
{
	return _HandleThreadStopped(event->Thread(), NULL);
}


bool
TeamDebugger::_HandleBreakpointHit(BreakpointHitEvent* event)
{
	return _HandleThreadStopped(event->Thread(), event->GetCpuState());
}


bool
TeamDebugger::_HandleWatchpointHit(WatchpointHitEvent* event)
{
	return _HandleThreadStopped(event->Thread(), event->GetCpuState());
}


bool
TeamDebugger::_HandleSingleStep(SingleStepEvent* event)
{
	return _HandleThreadStopped(event->Thread(), event->GetCpuState());
}


bool
TeamDebugger::_HandleExceptionOccurred(ExceptionOccurredEvent* event)
{
	return _HandleThreadStopped(event->Thread(), NULL);
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
	CpuState* cpuState = NULL;
	status_t error = fDebuggerInterface->GetCpuState(thread->ID(), cpuState);

	uint32 newState = THREAD_STATE_UNKNOWN;
	if (error == B_OK) {
		newState = THREAD_STATE_STOPPED;
		cpuState->RemoveReference();
	} else if (error == B_BAD_THREAD_STATE)
		newState = THREAD_STATE_RUNNING;

	_SetThreadState(thread, newState, cpuState);
}


void
TeamDebugger::_SetThreadState(::Thread* thread, uint32 state,
	CpuState* cpuState)
{
	thread->SetState(state);
	thread->SetCpuState(cpuState);
}


status_t
TeamDebugger::_SetUserBreakpoint(target_addr_t address, bool enabled)
{
	user_breakpoint_state state = enabled
		? USER_BREAKPOINT_ENABLED : USER_BREAKPOINT_DISABLED;

	AutoLocker<TeamDebugModel> locker(fDebugModel);

	// If there already is a breakpoint, it might already have the requested
	// state.
	Breakpoint* breakpoint = fDebugModel->BreakpointAtAddress(address);
	if (breakpoint != NULL && breakpoint->UserState() == state)
		return B_OK;

	// create a breakpoint, if it doesn't exist yet
	if (breakpoint == NULL) {
		Image* image = fTeam->ImageByAddress(address);
		if (image == NULL)
			return B_OK;

		breakpoint = new(std::nothrow) Breakpoint(image, address);
		if (breakpoint == NULL)
			return B_NO_MEMORY;

		if (!fDebugModel->AddBreakpoint(breakpoint))
			return B_NO_MEMORY;
	}

	user_breakpoint_state oldState = breakpoint->UserState();

	// set the breakpoint state
	breakpoint->SetUserState(state);
	fDebugModel->NotifyUserBreakpointChanged(breakpoint);

	bool install = breakpoint->ShouldBeInstalled();
	if (breakpoint->IsInstalled() == install)
		return B_OK;

	// The breakpoint needs to be installed/uninstalled.
	locker.Unlock();

	status_t error = install
		? fDebuggerInterface->InstallBreakpoint(address)
		: fDebuggerInterface->UninstallBreakpoint(address);

	locker.Lock();

	breakpoint = fDebugModel->BreakpointAtAddress(address);

	// Mark the breakpoint installed/uninstalled, if everything went fine.
	if (error == B_OK) {
		breakpoint->SetInstalled(install);
printf("-> breakpoint %sinstalled successfully!\n", install ? "" : "un");
		return B_OK;
	}

	// revert on error
	breakpoint->SetUserState(oldState);
	fDebugModel->NotifyUserBreakpointChanged(breakpoint);

	if (breakpoint->IsUnused())
		fDebugModel->RemoveBreakpoint(breakpoint);

	return error;
}


void
TeamDebugger::_HandleThreadAction(thread_id threadID, uint32 action)
{
	AutoLocker< ::Team> locker(fTeam);

	::Thread* thread = fTeam->ThreadByID(threadID);
	if (thread == NULL || thread->State() == THREAD_STATE_UNKNOWN)
		return;

	// When stop is requested, thread must be running, otherwise stopped.
	if (action == MSG_THREAD_STOP
			? thread->State() != THREAD_STATE_RUNNING
			: thread->State() != THREAD_STATE_STOPPED) {
		return;
	}

	// When continuing the thread update thread state before actually issuing
	// the command, since we need to unlock.
	if (action != MSG_THREAD_STOP)
		_SetThreadState(thread, THREAD_STATE_RUNNING, NULL);

	locker.Unlock();

	switch (action) {
		case MSG_THREAD_RUN:
printf("MSG_THREAD_RUN\n");
			fDebuggerInterface->ContinueThread(threadID);
			break;
		case MSG_THREAD_STOP:
printf("MSG_THREAD_STOP\n");
			fDebuggerInterface->StopThread(threadID);
			break;
		case MSG_THREAD_STEP_OVER:
printf("MSG_THREAD_STEP_OVER\n");
			fDebuggerInterface->SingleStepThread(threadID);
			break;
		case MSG_THREAD_STEP_INTO:
printf("MSG_THREAD_STEP_INTO\n");
			fDebuggerInterface->SingleStepThread(threadID);
			break;
		case MSG_THREAD_STEP_OUT:
printf("MSG_THREAD_STEP_OUT\n");
			fDebuggerInterface->SingleStepThread(threadID);
			break;

// TODO: Handle stepping correctly!
	}
}


void
TeamDebugger::_HandleSetUserBreakpoint(target_addr_t address, bool enabled)
{
printf("TeamDebugger::_HandleSetUserBreakpoint(%#llx, %d)\n", address, enabled);
	status_t error = _SetUserBreakpoint(address, enabled);
	if (error != B_OK) {
		_NotifyUser("Install Breakpoint", "Failed to install breakpoint: %s",
			strerror(error));
	}
}


void
TeamDebugger::_HandleClearUserBreakpoint(target_addr_t address)
{
printf("TeamDebugger::_HandleClearUserBreakpoint(%#llx)\n", address);
	AutoLocker<TeamDebugModel> locker(fDebugModel);

	Breakpoint* breakpoint = fDebugModel->BreakpointAtAddress(address);
	if (breakpoint == NULL || breakpoint->UserState() == USER_BREAKPOINT_NONE)
		return;

	// set the breakpoint state
	breakpoint->SetUserState(USER_BREAKPOINT_NONE);
	fDebugModel->NotifyUserBreakpointChanged(breakpoint);

	// check whether the breakpoint needs to be uninstalled
	bool uninstall = !breakpoint->ShouldBeInstalled()
		&& breakpoint->IsInstalled();

	// if unused remove it
	if (breakpoint->IsUnused())
		fDebugModel->RemoveBreakpoint(breakpoint);

	locker.Unlock();

	if (uninstall)
		fDebuggerInterface->UninstallBreakpoint(address);
}


void
TeamDebugger::_HandleThreadStateChanged(thread_id threadID)
{
	AutoLocker< ::Team> teamLocker(fTeam);

	::Thread* thread = fTeam->ThreadByID(threadID);
	if (thread == NULL)
		return;

	// cancel jobs for this thread
	fWorker->AbortJob(JobKey(thread, JOB_TYPE_GET_CPU_STATE));
	fWorker->AbortJob(JobKey(thread, JOB_TYPE_GET_STACK_TRACE));

	// If the thread is stopped and has no CPU state yet, schedule a job.
	if (thread->State() == THREAD_STATE_STOPPED
			&& thread->GetCpuState() == NULL) {
		fWorker->ScheduleJob(
			new(std::nothrow) GetCpuStateJob(fDebuggerInterface, thread),
			this);
	}
}


void
TeamDebugger::_HandleCpuStateChanged(thread_id threadID)
{
	AutoLocker< ::Team> teamLocker(fTeam);

	::Thread* thread = fTeam->ThreadByID(threadID);
	if (thread == NULL)
		return;

	// cancel stack trace job for this thread
	fWorker->AbortJob(JobKey(thread, JOB_TYPE_GET_STACK_TRACE));

	// If the thread has a CPU state, but no stack trace yet, schedule a job.
	if (thread->GetCpuState() != NULL && thread->GetStackTrace() == NULL) {
		fWorker->ScheduleJob(
			new(std::nothrow) GetStackTraceJob(fDebuggerInterface,
				fDebuggerInterface->GetArchitecture(), thread),
			this);
	}
}


void
TeamDebugger::_HandleStackTraceChanged(thread_id threadID)
{
printf("TeamDebugger::_HandleStackTraceChanged()\n");
}


void
TeamDebugger::_NotifyUser(const char* title, const char* text,...)
{
	// print the message
	char buffer[1024];
	va_list args;
	va_start(args, text);
	vsnprintf(buffer, sizeof(buffer), text, args);
	va_end(args);

	// show the alert
	BAlert* alert = new(std::nothrow) BAlert(title, buffer, "OK",
		NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	if (alert != NULL)
		alert->Go(NULL);

	// TODO: We need to let the alert run asynchronously, but we shouldn't just
	// create it and don't care anymore. Maybe an error window, which can
	// display a list of errors would be the better choice.
}
