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

#include "BreakpointManager.h"
#include "CpuState.h"
#include "DebuggerInterface.h"
#include "Jobs.h"
#include "MessageCodes.h"
#include "Statement.h"
#include "TeamDebugModel.h"


TeamDebugger::TeamDebugger(Listener* listener)
	:
	BLooper("team debugger"),
	fListener(listener),
	fTeam(NULL),
	fDebugModel(NULL),
	fTeamID(-1),
	fDebuggerInterface(NULL),
	fWorker(NULL),
	fBreakpointManager(NULL),
	fDebugEventListener(-1),
	fTeamWindow(NULL),
	fTerminating(false),
	fKillTeamOnQuit(false)
{
}


TeamDebugger::~TeamDebugger()
{
	AutoLocker<BLooper> locker(this);

	fTerminating = true;

	if (fDebuggerInterface != NULL)
		fDebuggerInterface->Close(fKillTeamOnQuit);

	if (fWorker != NULL)
		fWorker->ShutDown();

	locker.Unlock();

	if (fDebugEventListener >= 0)
		wait_for_thread(fDebugEventListener, NULL);

	// quit window
	if (fTeamWindow != NULL) {
		// TODO: This is not clean. If the window has been deleted we shouldn't
		// try to access it!
		if (fTeamWindow->Lock())
			fTeamWindow->Quit();
	}

	ThreadHandler* threadHandler = fThreadHandlers.Clear(true);
	while (threadHandler != NULL) {
		ThreadHandler* next = threadHandler->fNext;
		threadHandler->RemoveReference();
		threadHandler = next;
	}

	delete fBreakpointManager;
	delete fDebuggerInterface;
	delete fWorker;
	delete fDebugModel;
	delete fTeam;

	fListener->TeamDebuggerQuit(this);
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

	// init thread handler table
	error = fThreadHandlers.Init();
	if (error != B_OK)
		return error;

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

	// create the breakpoint manager
	fBreakpointManager = new(std::nothrow) BreakpointManager(fDebugModel,
		fDebuggerInterface);
	if (fBreakpointManager == NULL)
		return B_NO_MEMORY;

	error = fBreakpointManager->Init();
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

			ThreadHandler* handler = new(std::nothrow) ThreadHandler(
				fDebugModel, thread, fWorker, fDebuggerInterface,
				fBreakpointManager);
			if (handler == NULL)
				return B_NO_MEMORY;

			fThreadHandlers.Insert(handler);

			handler->Init();
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

			if (ThreadHandler* handler = _GetThreadHandler(threadID)) {
				handler->HandleThreadAction(message->what);
				handler->RemoveReference();
			}
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

			if (ThreadHandler* handler = _GetThreadHandler(threadID)) {
				handler->HandleThreadStateChanged();
				handler->RemoveReference();
			}
			break;
		}
		case MSG_THREAD_CPU_STATE_CHANGED:
		{
			int32 threadID;
			if (message->FindInt32("thread", &threadID) != B_OK)
				break;

			if (ThreadHandler* handler = _GetThreadHandler(threadID)) {
				handler->HandleCpuStateChanged();
				handler->RemoveReference();
			}
			break;
		}
		case MSG_THREAD_STACK_TRACE_CHANGED:
		{
			int32 threadID;
			if (message->FindInt32("thread", &threadID) != B_OK)
				break;

			if (ThreadHandler* handler = _GetThreadHandler(threadID)) {
				handler->HandleStackTraceChanged();
				handler->RemoveReference();
			}
			break;
		}

		case MSG_DEBUGGER_EVENT:
		{
			DebugEvent* event;
			if (message->FindPointer("event", (void**)&event) != B_OK)
				break;

			_HandleDebuggerMessage(event);
			delete event;
		}

		default:
			BLooper::MessageReceived(message);
			break;
	}
}


void
TeamDebugger::FunctionSourceCodeRequested(TeamWindow* window,
	FunctionDebugInfo* function)
{
	// mark loading
	AutoLocker< ::Team> locker(fTeam);
	if (function->SourceCodeState() != FUNCTION_SOURCE_NOT_LOADED)
		return;
	function->SetSourceCode(NULL, FUNCTION_SOURCE_LOADING);
	locker.Unlock();

	// schedule the job
	if (fWorker->ScheduleJob(
			new(std::nothrow) LoadSourceCodeJob(fDebuggerInterface,
				fDebuggerInterface->GetArchitecture(), fTeam, function),
			this) != B_OK) {
		// scheduling failed -- mark unavailable
		locker.Lock();
		function->SetSourceCode(NULL, FUNCTION_SOURCE_UNAVAILABLE);
		locker.Unlock();
	}
}


void
TeamDebugger::ImageDebugInfoRequested(TeamWindow* window, Image* image)
{
	LoadImageDebugInfoJob::ScheduleIfNecessary(fDebuggerInterface,
		fDebuggerInterface->GetArchitecture(), fWorker, image);
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
	AutoLocker< ::Team> locker(fTeam);
	BString name(fTeam->Name());
	locker.Unlock();

	BString message;
	message << "What shall be done about the debugged team '";
	message << name;
	message << "'?";

	name.Remove(0, name.FindLast('/') + 1);

	BString killLabel("Kill ");
	killLabel << name;

	BString resumeLabel("Resume ");
	resumeLabel << name;

	BAlert* alert = new BAlert("Quit Debugger", message.String(),
		killLabel.String(), "Cancel", resumeLabel.String());

	switch (alert->Go()) {
		case 0:
			fKillTeamOnQuit = true;
			break;
		case 1:
			return false;
		case 2:
			// Detach from the team and resume and stopped threads.
			break;
	}

	PostMessage(B_QUIT_REQUESTED);

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

		BMessage message(MSG_DEBUGGER_EVENT);
		if (message.AddPointer("event", event) != B_OK
			|| PostMessage(&message) != B_OK) {
			// TODO: Continue thread if necessary!
			delete event;
		}
	}

	return B_OK;
}


void
TeamDebugger::_HandleDebuggerMessage(DebugEvent* event)
{
printf("TeamDebugger::_HandleDebuggerMessage(): %d\n", event->EventType());
	bool handled = false;

	ThreadHandler* handler = _GetThreadHandler(event->Thread());
	Reference<ThreadHandler> handlerReference(handler);

	switch (event->EventType()) {
		case B_DEBUGGER_MESSAGE_THREAD_DEBUGGED:
printf("B_DEBUGGER_MESSAGE_THREAD_DEBUGGED: thread: %ld\n", event->Thread());
			if (handler != NULL) {
				handled = handler->HandleThreadDebugged(
					dynamic_cast<ThreadDebuggedEvent*>(event));
			}
			break;
		case B_DEBUGGER_MESSAGE_DEBUGGER_CALL:
printf("B_DEBUGGER_MESSAGE_DEBUGGER_CALL: thread: %ld\n", event->Thread());
			if (handler != NULL) {
				handled = handler->HandleDebuggerCall(
					dynamic_cast<DebuggerCallEvent*>(event));
			}
			break;
		case B_DEBUGGER_MESSAGE_BREAKPOINT_HIT:
printf("B_DEBUGGER_MESSAGE_BREAKPOINT_HIT: thread: %ld\n", event->Thread());
			if (handler != NULL) {
				handled = handler->HandleBreakpointHit(
					dynamic_cast<BreakpointHitEvent*>(event));
			}
			break;
		case B_DEBUGGER_MESSAGE_WATCHPOINT_HIT:
printf("B_DEBUGGER_MESSAGE_WATCHPOINT_HIT: thread: %ld\n", event->Thread());
			if (handler != NULL) {
				handled = handler->HandleWatchpointHit(
					dynamic_cast<WatchpointHitEvent*>(event));
			}
			break;
		case B_DEBUGGER_MESSAGE_SINGLE_STEP:
printf("B_DEBUGGER_MESSAGE_SINGLE_STEP: thread: %ld\n", event->Thread());
			if (handler != NULL) {
				handled = handler->HandleSingleStep(
					dynamic_cast<SingleStepEvent*>(event));
			}
			break;
		case B_DEBUGGER_MESSAGE_EXCEPTION_OCCURRED:
printf("B_DEBUGGER_MESSAGE_EXCEPTION_OCCURRED: thread: %ld\n", event->Thread());
			if (handler != NULL) {
				handled = handler->HandleExceptionOccurred(
					dynamic_cast<ExceptionOccurredEvent*>(event));
			}
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
TeamDebugger::_HandleThreadCreated(ThreadCreatedEvent* event)
{
	AutoLocker< ::Team> locker(fTeam);

	ThreadInfo info;
	status_t error = fDebuggerInterface->GetThreadInfo(event->NewThread(),
		info);
	if (error == B_OK) {
		::Thread* thread;
		fTeam->AddThread(info, &thread);

		ThreadHandler* handler = new(std::nothrow) ThreadHandler(
			fDebugModel, thread, fWorker, fDebuggerInterface,
			fBreakpointManager);
		if (handler != NULL) {
			fThreadHandlers.Insert(handler);
			handler->Init();
		}
	}

	return false;
}


bool
TeamDebugger::_HandleThreadDeleted(ThreadDeletedEvent* event)
{
	AutoLocker< ::Team> locker(fTeam);
	if (ThreadHandler* handler = fThreadHandlers.Lookup(event->Thread())) {
		fThreadHandlers.Remove(handler);
		handler->RemoveReference();
	}
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
TeamDebugger::_HandleSetUserBreakpoint(target_addr_t address, bool enabled)
{
printf("TeamDebugger::_HandleSetUserBreakpoint(%#llx, %d)\n", address, enabled);
	status_t error = fBreakpointManager->InstallUserBreakpoint(address,
		enabled);
	if (error != B_OK) {
		_NotifyUser("Install Breakpoint", "Failed to install breakpoint: %s",
			strerror(error));
	}
}


void
TeamDebugger::_HandleClearUserBreakpoint(target_addr_t address)
{
printf("TeamDebugger::_HandleClearUserBreakpoint(%#llx)\n", address);
	fBreakpointManager->UninstallUserBreakpoint(address);
}


ThreadHandler*
TeamDebugger::_GetThreadHandler(thread_id threadID)
{
	AutoLocker<TeamDebugModel> locker(fDebugModel);

	ThreadHandler* handler = fThreadHandlers.Lookup(threadID);
	if (handler != NULL)
		handler->AddReference();
	return handler;
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


// #pragma mark - Listener


TeamDebugger::Listener::~Listener()
{
}
