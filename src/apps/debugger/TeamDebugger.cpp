/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2010-2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "TeamDebugger.h"

#include <stdarg.h>
#include <stdio.h>

#include <new>

#include <Message.h>

#include <AutoLocker.h>

#include "debug_utils.h"

#include "BreakpointManager.h"
#include "BreakpointSetting.h"
#include "CpuState.h"
#include "DebuggerInterface.h"
#include "FileManager.h"
#include "Function.h"
#include "FunctionID.h"
#include "ImageDebugInfo.h"
#include "Jobs.h"
#include "LocatableFile.h"
#include "MessageCodes.h"
#include "SettingsManager.h"
#include "SourceCode.h"
#include "SpecificImageDebugInfo.h"
#include "StackFrame.h"
#include "StackFrameValues.h"
#include "Statement.h"
#include "SymbolInfo.h"
#include "TeamDebugInfo.h"
#include "TeamMemoryBlock.h"
#include "TeamMemoryBlockManager.h"
#include "TeamSettings.h"
#include "TeamUiSettings.h"
#include "Tracing.h"
#include "ValueNode.h"
#include "ValueNodeContainer.h"
#include "Variable.h"

// #pragma mark - ImageHandler


struct TeamDebugger::ImageHandler : public BReferenceable,
	private LocatableFile::Listener {
public:
	ImageHandler(TeamDebugger* teamDebugger, Image* image)
		:
		fTeamDebugger(teamDebugger),
		fImage(image)
	{
		fImage->AcquireReference();
		if (fImage->ImageFile() != NULL)
			fImage->ImageFile()->AddListener(this);
	}

	~ImageHandler()
	{
		if (fImage->ImageFile() != NULL)
			fImage->ImageFile()->RemoveListener(this);
		fImage->ReleaseReference();
	}

	Image* GetImage() const
	{
		return fImage;
	}

	image_id ImageID() const
	{
		return fImage->ID();
	}

private:
	// LocatableFile::Listener
	virtual void LocatableFileChanged(LocatableFile* file)
	{
		BMessage message(MSG_IMAGE_FILE_CHANGED);
		message.AddInt32("image", fImage->ID());
		fTeamDebugger->PostMessage(&message);
	}

private:
	TeamDebugger*	fTeamDebugger;
	Image*			fImage;

public:
	ImageHandler*	fNext;
};


// #pragma mark - ImageHandlerHashDefinition


struct TeamDebugger::ImageHandlerHashDefinition {
	typedef image_id		KeyType;
	typedef	ImageHandler	ValueType;

	size_t HashKey(image_id key) const
	{
		return (size_t)key;
	}

	size_t Hash(const ImageHandler* value) const
	{
		return HashKey(value->ImageID());
	}

	bool Compare(image_id key, const ImageHandler* value) const
	{
		return value->ImageID() == key;
	}

	ImageHandler*& GetLink(ImageHandler* value) const
	{
		return value->fNext;
	}
};


// #pragma mark - TeamDebugger


TeamDebugger::TeamDebugger(Listener* listener, UserInterface* userInterface,
	SettingsManager* settingsManager)
	:
	BLooper("team debugger"),
	fListener(listener),
	fSettingsManager(settingsManager),
	fTeam(NULL),
	fTeamID(-1),
	fImageHandlers(NULL),
	fDebuggerInterface(NULL),
	fFileManager(NULL),
	fWorker(NULL),
	fBreakpointManager(NULL),
	fMemoryBlockManager(NULL),
	fDebugEventListener(-1),
	fUserInterface(userInterface),
	fTerminating(false),
	fKillTeamOnQuit(false)
{
	fUserInterface->AcquireReference();
}


TeamDebugger::~TeamDebugger()
{
	if (fTeam != NULL)
		_SaveSettings();

	AutoLocker<BLooper> locker(this);

	fTerminating = true;

	if (fDebuggerInterface != NULL) {
		fDebuggerInterface->Close(fKillTeamOnQuit);
		fDebuggerInterface->ReleaseReference();
	}

	if (fWorker != NULL)
		fWorker->ShutDown();

	locker.Unlock();

	if (fDebugEventListener >= 0)
		wait_for_thread(fDebugEventListener, NULL);

	// terminate UI
	if (fUserInterface != NULL) {
		fUserInterface->Terminate();
		fUserInterface->ReleaseReference();
	}

	ThreadHandler* threadHandler = fThreadHandlers.Clear(true);
	while (threadHandler != NULL) {
		ThreadHandler* next = threadHandler->fNext;
		threadHandler->ReleaseReference();
		threadHandler = next;
	}

	if (fImageHandlers != NULL) {
		ImageHandler* imageHandler = fImageHandlers->Clear(true);
		while (imageHandler != NULL) {
			ImageHandler* next = imageHandler->fNext;
			imageHandler->ReleaseReference();
			imageHandler = next;
		}
	}

	delete fImageHandlers;

	delete fBreakpointManager;
	delete fMemoryBlockManager;
	delete fWorker;
	delete fTeam;
	delete fFileManager;

	fListener->TeamDebuggerQuit(this);
}


status_t
TeamDebugger::Init(team_id teamID, thread_id threadID, bool stopInMain)
{
	bool targetIsLocal = true;
		// TODO: Support non-local targets!

	// the first thing we want to do when running
	PostMessage(MSG_LOAD_SETTINGS);

	fTeamID = teamID;

	// create debugger interface
	fDebuggerInterface = new(std::nothrow) DebuggerInterface(fTeamID);
	if (fDebuggerInterface == NULL)
		return B_NO_MEMORY;

	status_t error = fDebuggerInterface->Init();
	if (error != B_OK)
		return error;

	// create file manager
	fFileManager = new(std::nothrow) FileManager;
	if (fFileManager == NULL)
		return B_NO_MEMORY;

	error = fFileManager->Init(targetIsLocal);
	if (error != B_OK)
		return error;

	// create team debug info
	TeamDebugInfo* teamDebugInfo = new(std::nothrow) TeamDebugInfo(
		fDebuggerInterface, fDebuggerInterface->GetArchitecture(),
		fFileManager);
	if (teamDebugInfo == NULL)
		return B_NO_MEMORY;
	BReference<TeamDebugInfo> teamDebugInfoReference(teamDebugInfo);

	error = teamDebugInfo->Init();
	if (error != B_OK)
		return error;

	// check whether the team exists at all
	// TODO: That should be done in the debugger interface!
	team_info teamInfo;
	error = get_team_info(fTeamID, &teamInfo);
	if (error != B_OK)
		return error;

	// create a team object
	fTeam = new(std::nothrow) ::Team(fTeamID, fDebuggerInterface,
		fDebuggerInterface->GetArchitecture(), teamDebugInfo,
		teamDebugInfo);
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

	// create image handler table
	fImageHandlers = new(std::nothrow) ImageHandlerTable;
	if (fImageHandlers == NULL)
		return B_NO_MEMORY;

	error = fImageHandlers->Init();
	if (error != B_OK)
		return error;

	// create our worker
	fWorker = new(std::nothrow) Worker;
	if (fWorker == NULL)
		return B_NO_MEMORY;

	error = fWorker->Init();
	if (error != B_OK)
		return error;

	// create the breakpoint manager
	fBreakpointManager = new(std::nothrow) BreakpointManager(fTeam,
		fDebuggerInterface);
	if (fBreakpointManager == NULL)
		return B_NO_MEMORY;

	error = fBreakpointManager->Init();
	if (error != B_OK)
		return error;

	// create the memory block manager
	fMemoryBlockManager = new(std::nothrow) TeamMemoryBlockManager();
	if (fMemoryBlockManager == NULL)
		return B_NO_MEMORY;

	error = fMemoryBlockManager->Init();
	if (error != B_OK)
		return error;

	// set team debugging flags
	fDebuggerInterface->SetTeamDebuggingFlags(
		B_TEAM_DEBUG_THREADS | B_TEAM_DEBUG_IMAGES);

	// get the initial state of the team
	AutoLocker< ::Team> teamLocker(fTeam);

	ThreadHandler* mainThreadHandler = NULL;
	{
		BObjectList<ThreadInfo> threadInfos(20, true);
		status_t error = fDebuggerInterface->GetThreadInfos(threadInfos);
		for (int32 i = 0; ThreadInfo* info = threadInfos.ItemAt(i); i++) {
			::Thread* thread;
			error = fTeam->AddThread(*info, &thread);
			if (error != B_OK)
				return error;

			ThreadHandler* handler = new(std::nothrow) ThreadHandler(thread,
				fWorker, fDebuggerInterface,
				fBreakpointManager);
			if (handler == NULL)
				return B_NO_MEMORY;

			fThreadHandlers.Insert(handler);

			if (thread->IsMainThread())
				mainThreadHandler = handler;

			handler->Init();
		}
	}

	Image* appImage = NULL;
	{
		BObjectList<ImageInfo> imageInfos(20, true);
		status_t error = fDebuggerInterface->GetImageInfos(imageInfos);
		for (int32 i = 0; ImageInfo* info = imageInfos.ItemAt(i); i++) {
			Image* image;
			error = _AddImage(*info, &image);
			if (error != B_OK)
				return error;
			if (image->Type() == B_APP_IMAGE)
				appImage = image;

			ImageDebugInfoRequested(image);
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

	// init the UI
	error = fUserInterface->Init(fTeam, this);
	if (error != B_OK) {
		ERROR("Error: Failed to init the UI: %s\n", strerror(error));
		return error;
	}

	// if requested, stop the given thread
	if (threadID >= 0) {
		if (stopInMain) {
			SymbolInfo symbolInfo;
			if (appImage != NULL && mainThreadHandler != NULL
				&& fDebuggerInterface->GetSymbolInfo(
					fTeam->ID(), appImage->ID(), "main", B_SYMBOL_TYPE_TEXT,
					symbolInfo) == B_OK) {
				mainThreadHandler->SetBreakpointAndRun(symbolInfo.Address());
			}
		} else {
			debug_thread(threadID);
				// TODO: Superfluous, if the thread is already stopped.
		}
	}

	fListener->TeamDebuggerStarted(this);

	return B_OK;
}


void
TeamDebugger::Activate()
{
	fUserInterface->Show();
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
				handler->ReleaseReference();
			}
			break;
		}

		case MSG_SET_BREAKPOINT:
		case MSG_CLEAR_BREAKPOINT:
		{
			UserBreakpoint* breakpoint = NULL;
			BReference<UserBreakpoint> breakpointReference;
			uint64 address = 0;

			if (message->FindPointer("breakpoint", (void**)&breakpoint) == B_OK)
				breakpointReference.SetTo(breakpoint, true);
			else if (message->FindUInt64("address", &address) != B_OK)
				break;

			if (message->what == MSG_SET_BREAKPOINT) {
				bool enabled;
				if (message->FindBool("enabled", &enabled) != B_OK)
					enabled = true;

				if (breakpoint != NULL)
					_HandleSetUserBreakpoint(breakpoint, enabled);
				else
					_HandleSetUserBreakpoint(address, enabled);
			} else {
				if (breakpoint != NULL)
					_HandleClearUserBreakpoint(breakpoint);
				else
					_HandleClearUserBreakpoint(address);
			}

			break;
		}

		case MSG_INSPECT_ADDRESS:
		{
			TeamMemoryBlock::Listener* listener;
			if (message->FindPointer("listener",
				reinterpret_cast<void **>(&listener)) != B_OK) {
				break;
			}

			target_addr_t address;
			if (message->FindUInt64("address",
				&address) == B_OK) {
				_HandleInspectAddress(address, listener);
			}
			break;
		}

		case MSG_THREAD_STATE_CHANGED:
		{
			int32 threadID;
			if (message->FindInt32("thread", &threadID) != B_OK)
				break;

			if (ThreadHandler* handler = _GetThreadHandler(threadID)) {
				handler->HandleThreadStateChanged();
				handler->ReleaseReference();
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
				handler->ReleaseReference();
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
				handler->ReleaseReference();
			}
			break;
		}

		case MSG_IMAGE_DEBUG_INFO_CHANGED:
		{
			int32 imageID;
			if (message->FindInt32("image", &imageID) != B_OK)
				break;

			_HandleImageDebugInfoChanged(imageID);
			break;
		}

		case MSG_IMAGE_FILE_CHANGED:
		{
			int32 imageID;
			if (message->FindInt32("image", &imageID) != B_OK)
				break;

			_HandleImageFileChanged(imageID);
			break;
		}

		case MSG_DEBUGGER_EVENT:
		{
			DebugEvent* event;
			if (message->FindPointer("event", (void**)&event) != B_OK)
				break;

			_HandleDebuggerMessage(event);
			delete event;
			break;
		}

		case MSG_LOAD_SETTINGS:
			_LoadSettings();
			Activate();
			break;

		default:
			BLooper::MessageReceived(message);
			break;
	}
}


void
TeamDebugger::SourceEntryLocateRequested(const char* sourcePath,
	const char* locatedPath)
{
	AutoLocker<FileManager> locker(fFileManager);
	fFileManager->SourceEntryLocated(sourcePath, locatedPath);
}


void
TeamDebugger::FunctionSourceCodeRequested(FunctionInstance* functionInstance)
{
	Function* function = functionInstance->GetFunction();

	// mark loading
	AutoLocker< ::Team> locker(fTeam);

	if (functionInstance->SourceCodeState() != FUNCTION_SOURCE_NOT_LOADED)
		return;
	if (function->SourceCodeState() == FUNCTION_SOURCE_LOADED)
		return;

	functionInstance->SetSourceCode(NULL, FUNCTION_SOURCE_LOADING);

	bool loadForFunction = false;
	if (function->SourceCodeState() == FUNCTION_SOURCE_NOT_LOADED) {
		loadForFunction = true;
		function->SetSourceCode(NULL, FUNCTION_SOURCE_LOADING);
	}

	locker.Unlock();

	// schedule the job
	if (fWorker->ScheduleJob(
			new(std::nothrow) LoadSourceCodeJob(fDebuggerInterface,
				fDebuggerInterface->GetArchitecture(), fTeam, functionInstance,
					loadForFunction),
			this) != B_OK) {
		// scheduling failed -- mark unavailable
		locker.Lock();
		function->SetSourceCode(NULL, FUNCTION_SOURCE_UNAVAILABLE);
		locker.Unlock();
	}
}


void
TeamDebugger::ImageDebugInfoRequested(Image* image)
{
	LoadImageDebugInfoJob::ScheduleIfNecessary(fWorker, image);
}


void
TeamDebugger::ValueNodeValueRequested(CpuState* cpuState,
	ValueNodeContainer* container, ValueNode* valueNode)
{
	AutoLocker<ValueNodeContainer> containerLocker(container);
	if (valueNode->Container() != container)
		return;

	// check whether a job is already in progress
	AutoLocker<Worker> workerLocker(fWorker);
	SimpleJobKey jobKey(valueNode, JOB_TYPE_RESOLVE_VALUE_NODE_VALUE);
	if (fWorker->GetJob(jobKey) != NULL)
		return;
	workerLocker.Unlock();

	// schedule the job
	status_t error = fWorker->ScheduleJob(
		new(std::nothrow) ResolveValueNodeValueJob(fDebuggerInterface,
			fDebuggerInterface->GetArchitecture(), cpuState,
			fTeam->GetTeamTypeInformation(), container,	valueNode),	this);
	if (error != B_OK) {
		// scheduling failed -- set the value to invalid
		valueNode->SetLocationAndValue(NULL, NULL, error);
	}
}


void
TeamDebugger::ThreadActionRequested(thread_id threadID,
	uint32 action)
{
	BMessage message(action);
	message.AddInt32("thread", threadID);
	PostMessage(&message);
}


void
TeamDebugger::SetBreakpointRequested(target_addr_t address, bool enabled)
{
	BMessage message(MSG_SET_BREAKPOINT);
	message.AddUInt64("address", (uint64)address);
	message.AddBool("enabled", enabled);
	PostMessage(&message);
}


void
TeamDebugger::SetBreakpointEnabledRequested(UserBreakpoint* breakpoint,
	bool enabled)
{
	BMessage message(MSG_SET_BREAKPOINT);
	BReference<UserBreakpoint> breakpointReference(breakpoint);
	if (message.AddPointer("breakpoint", breakpoint) == B_OK
		&& message.AddBool("enabled", enabled) == B_OK
		&& PostMessage(&message) == B_OK) {
		breakpointReference.Detach();
	}
}


void
TeamDebugger::ClearBreakpointRequested(target_addr_t address)
{
	BMessage message(MSG_CLEAR_BREAKPOINT);
	message.AddUInt64("address", (uint64)address);
	PostMessage(&message);
}


void
TeamDebugger::ClearBreakpointRequested(UserBreakpoint* breakpoint)
{
	BMessage message(MSG_CLEAR_BREAKPOINT);
	BReference<UserBreakpoint> breakpointReference(breakpoint);
	if (message.AddPointer("breakpoint", breakpoint) == B_OK
		&& PostMessage(&message) == B_OK) {
		breakpointReference.Detach();
	}
}


void
TeamDebugger::InspectRequested(target_addr_t address,
	TeamMemoryBlock::Listener *listener)
{
	BMessage message(MSG_INSPECT_ADDRESS);
	message.AddUInt64("address", address);
	message.AddPointer("listener", listener);
	PostMessage(&message);
}


bool
TeamDebugger::UserInterfaceQuitRequested()
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

	int32 choice = fUserInterface->SynchronouslyAskUser("Quit Debugger",
		message, killLabel, "Cancel", resumeLabel);

	switch (choice) {
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
	TRACE_JOBS("TeamDebugger::JobDone(%p)\n", job);
}


void
TeamDebugger::JobFailed(Job* job)
{
	TRACE_JOBS("TeamDebugger::JobFailed(%p)\n", job);
	// TODO: notify user
}


void
TeamDebugger::JobAborted(Job* job)
{
	TRACE_JOBS("TeamDebugger::JobAborted(%p)\n", job);
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


void
TeamDebugger::ImageDebugInfoChanged(const ::Team::ImageEvent& event)
{
	BMessage message(MSG_IMAGE_DEBUG_INFO_CHANGED);
	message.AddInt32("image", event.GetImage()->ID());
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
			TRACE_EVENTS("TeamDebugger for team %ld: received event from team "
				"%ld!\n", fTeamID, event->Team());
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
	TRACE_EVENTS("TeamDebugger::_HandleDebuggerMessage(): %ld\n",
		event->EventType());

	bool handled = false;

	ThreadHandler* handler = _GetThreadHandler(event->Thread());
	BReference<ThreadHandler> handlerReference(handler);

	switch (event->EventType()) {
		case B_DEBUGGER_MESSAGE_THREAD_DEBUGGED:
			TRACE_EVENTS("B_DEBUGGER_MESSAGE_THREAD_DEBUGGED: thread: %ld\n",
				event->Thread());

			if (handler != NULL) {
				handled = handler->HandleThreadDebugged(
					dynamic_cast<ThreadDebuggedEvent*>(event));
			}
			break;
		case B_DEBUGGER_MESSAGE_DEBUGGER_CALL:
			TRACE_EVENTS("B_DEBUGGER_MESSAGE_DEBUGGER_CALL: thread: %ld\n",
				event->Thread());

			if (handler != NULL) {
				handled = handler->HandleDebuggerCall(
					dynamic_cast<DebuggerCallEvent*>(event));
			}
			break;
		case B_DEBUGGER_MESSAGE_BREAKPOINT_HIT:
			TRACE_EVENTS("B_DEBUGGER_MESSAGE_BREAKPOINT_HIT: thread: %ld\n",
				event->Thread());

			if (handler != NULL) {
				handled = handler->HandleBreakpointHit(
					dynamic_cast<BreakpointHitEvent*>(event));
			}
			break;
		case B_DEBUGGER_MESSAGE_WATCHPOINT_HIT:
			TRACE_EVENTS("B_DEBUGGER_MESSAGE_WATCHPOINT_HIT: thread: %ld\n",
				event->Thread());

			if (handler != NULL) {
				handled = handler->HandleWatchpointHit(
					dynamic_cast<WatchpointHitEvent*>(event));
			}
			break;
		case B_DEBUGGER_MESSAGE_SINGLE_STEP:
			TRACE_EVENTS("B_DEBUGGER_MESSAGE_SINGLE_STEP: thread: %ld\n",
				event->Thread());

			if (handler != NULL) {
				handled = handler->HandleSingleStep(
					dynamic_cast<SingleStepEvent*>(event));
			}
			break;
		case B_DEBUGGER_MESSAGE_EXCEPTION_OCCURRED:
			TRACE_EVENTS("B_DEBUGGER_MESSAGE_EXCEPTION_OCCURRED: thread: %ld\n",
				event->Thread());

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
			TRACE_EVENTS("B_DEBUGGER_MESSAGE_TEAM_DELETED: team: %ld\n",
				event->Team());
			break;
		case B_DEBUGGER_MESSAGE_TEAM_EXEC:
			TRACE_EVENTS("B_DEBUGGER_MESSAGE_TEAM_EXEC: team: %ld\n",
				event->Team());
			// TODO: Handle!
			break;
		case B_DEBUGGER_MESSAGE_THREAD_CREATED:
		{
			ThreadCreatedEvent* threadEvent
				= dynamic_cast<ThreadCreatedEvent*>(event);
			TRACE_EVENTS("B_DEBUGGER_MESSAGE_THREAD_CREATED: thread: %ld\n",
				threadEvent->NewThread());
			handled = _HandleThreadCreated(threadEvent);
			break;
		}
		case DEBUGGER_MESSAGE_THREAD_RENAMED:
		{
			ThreadRenamedEvent* threadEvent
				= dynamic_cast<ThreadRenamedEvent*>(event);
			TRACE_EVENTS("DEBUGGER_MESSAGE_THREAD_RENAMED: thread: %ld "
				"(\"%s\")\n",
				threadEvent->RenamedThread(), threadEvent->NewName());
			handled = _HandleThreadRenamed(threadEvent);
			break;
		}
		case DEBUGGER_MESSAGE_THREAD_PRIORITY_CHANGED:
		{
			ThreadPriorityChangedEvent* threadEvent
				= dynamic_cast<ThreadPriorityChangedEvent*>(event);
			TRACE_EVENTS("B_DEBUGGER_MESSAGE_THREAD_PRIORITY_CHANGED: thread:"
				" %ld\n", threadEvent->ChangedThread());
			handled = _HandleThreadPriorityChanged(threadEvent);
			break;
		}
		case B_DEBUGGER_MESSAGE_THREAD_DELETED:
			TRACE_EVENTS("B_DEBUGGER_MESSAGE_THREAD_DELETED: thread: %ld\n",
				event->Thread());
			handled = _HandleThreadDeleted(
				dynamic_cast<ThreadDeletedEvent*>(event));
			break;
		case B_DEBUGGER_MESSAGE_IMAGE_CREATED:
		{
			ImageCreatedEvent* imageEvent
				= dynamic_cast<ImageCreatedEvent*>(event);
			TRACE_EVENTS("B_DEBUGGER_MESSAGE_IMAGE_CREATED: image: \"%s\" "
				"(%ld)\n", imageEvent->GetImageInfo().Name().String(),
				imageEvent->GetImageInfo().ImageID());
			handled = _HandleImageCreated(imageEvent);
			break;
		}
		case B_DEBUGGER_MESSAGE_IMAGE_DELETED:
		{
			ImageDeletedEvent* imageEvent
				= dynamic_cast<ImageDeletedEvent*>(event);
			TRACE_EVENTS("B_DEBUGGER_MESSAGE_IMAGE_DELETED: image: \"%s\" "
				"(%ld)\n", imageEvent->GetImageInfo().Name().String(),
				imageEvent->GetImageInfo().ImageID());
			handled = _HandleImageDeleted(imageEvent);
			break;
		}
		case B_DEBUGGER_MESSAGE_PRE_SYSCALL:
		case B_DEBUGGER_MESSAGE_POST_SYSCALL:
		case B_DEBUGGER_MESSAGE_SIGNAL_RECEIVED:
		case B_DEBUGGER_MESSAGE_PROFILER_UPDATE:
		case B_DEBUGGER_MESSAGE_HANDED_OVER:
			// not interested
			break;
		default:
			WARNING("TeamDebugger for team %ld: unknown event type: "
				"%ld\n", fTeamID, event->EventType());
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

		ThreadHandler* handler = new(std::nothrow) ThreadHandler(thread,
			fWorker, fDebuggerInterface,
			fBreakpointManager);
		if (handler != NULL) {
			fThreadHandlers.Insert(handler);
			handler->Init();
		}
	}

	return false;
}


bool
TeamDebugger::_HandleThreadRenamed(ThreadRenamedEvent* event)
{
	AutoLocker< ::Team> locker(fTeam);

	::Thread* thread = fTeam->ThreadByID(event->RenamedThread());

	if (thread != NULL)
		thread->SetName(event->NewName());

	return false;
}


bool
TeamDebugger::_HandleThreadPriorityChanged(ThreadPriorityChangedEvent*)
{
	// TODO: implement once we actually track thread priorities

	return false;
}


bool
TeamDebugger::_HandleThreadDeleted(ThreadDeletedEvent* event)
{
	AutoLocker< ::Team> locker(fTeam);
	if (ThreadHandler* handler = fThreadHandlers.Lookup(event->Thread())) {
		fThreadHandlers.Remove(handler);
		handler->ReleaseReference();
	}
	fTeam->RemoveThread(event->Thread());
	return false;
}


bool
TeamDebugger::_HandleImageCreated(ImageCreatedEvent* event)
{
	AutoLocker< ::Team> locker(fTeam);
	_AddImage(event->GetImageInfo());
	return false;
}


bool
TeamDebugger::_HandleImageDeleted(ImageDeletedEvent* event)
{
	AutoLocker< ::Team> locker(fTeam);
	fTeam->RemoveImage(event->GetImageInfo().ImageID());

	ImageHandler* imageHandler = fImageHandlers->Lookup(
		event->GetImageInfo().ImageID());
	if (imageHandler == NULL)
		return false;

	fImageHandlers->Remove(imageHandler);
	BReference<ImageHandler> imageHandlerReference(imageHandler, true);
	locker.Unlock();

	// remove breakpoints in the image
	fBreakpointManager->RemoveImageBreakpoints(imageHandler->GetImage());

	return false;
}


void
TeamDebugger::_HandleImageDebugInfoChanged(image_id imageID)
{
	// get the image (via the image handler)
	AutoLocker< ::Team> locker(fTeam);
	ImageHandler* imageHandler = fImageHandlers->Lookup(imageID);
	if (imageHandler == NULL)
		return;

	Image* image = imageHandler->GetImage();
	BReference<Image> imageReference(image);

	locker.Unlock();

	// update breakpoints in the image
	fBreakpointManager->UpdateImageBreakpoints(image);
}


void
TeamDebugger::_HandleImageFileChanged(image_id imageID)
{
	TRACE_IMAGES("TeamDebugger::_HandleImageFileChanged(%ld)\n", imageID);
// TODO: Reload the debug info!
}


void
TeamDebugger::_HandleSetUserBreakpoint(target_addr_t address, bool enabled)
{
	TRACE_CONTROL("TeamDebugger::_HandleSetUserBreakpoint(%#llx, %d)\n",
		address, enabled);

	// check whether there already is a breakpoint
	AutoLocker< ::Team> locker(fTeam);

	Breakpoint* breakpoint = fTeam->BreakpointAtAddress(address);
	UserBreakpoint* userBreakpoint = NULL;
	if (breakpoint != NULL && breakpoint->FirstUserBreakpoint() != NULL)
		userBreakpoint = breakpoint->FirstUserBreakpoint()->GetUserBreakpoint();
	BReference<UserBreakpoint> userBreakpointReference(userBreakpoint);

	if (userBreakpoint == NULL) {
		TRACE_CONTROL("  no breakpoint yet\n");

		// get the function at the address
		Image* image = fTeam->ImageByAddress(address);

		TRACE_CONTROL("  image: %p\n", image);

		if (image == NULL)
			return;
		ImageDebugInfo* imageDebugInfo = image->GetImageDebugInfo();

		TRACE_CONTROL("  image debug info: %p\n", imageDebugInfo);

		if (imageDebugInfo == NULL)
			return;
			// TODO: Handle this case by loading the debug info, if possible!
		FunctionInstance* functionInstance
			= imageDebugInfo->FunctionAtAddress(address);

		TRACE_CONTROL("  function instance: %p\n", functionInstance);

		if (functionInstance == NULL)
			return;
		Function* function = functionInstance->GetFunction();

		TRACE_CONTROL("  function: %p\n", function);

		// get the source location for the address
		FunctionDebugInfo* functionDebugInfo
			= functionInstance->GetFunctionDebugInfo();
		SourceLocation sourceLocation;
		Statement* breakpointStatement = NULL;
		if (functionDebugInfo->GetSpecificImageDebugInfo()->GetStatement(
				functionDebugInfo, address, breakpointStatement) != B_OK) {
			return;
		}

		sourceLocation = breakpointStatement->StartSourceLocation();
		breakpointStatement->ReleaseReference();

		target_addr_t relativeAddress = address - functionInstance->Address();

		TRACE_CONTROL("  relative address: %#llx, source location: "
			"(%ld, %ld)\n", relativeAddress, sourceLocation.Line(),
			sourceLocation.Column());

		// get function id
		FunctionID* functionID = functionInstance->GetFunctionID();
		if (functionID == NULL)
			return;
		BReference<FunctionID> functionIDReference(functionID, true);

		// create the user breakpoint
		userBreakpoint = new(std::nothrow) UserBreakpoint(
			UserBreakpointLocation(functionID, function->SourceFile(),
				sourceLocation, relativeAddress));
		if (userBreakpoint == NULL)
			return;
		userBreakpointReference.SetTo(userBreakpoint, true);

		TRACE_CONTROL("  created user breakpoint: %p\n", userBreakpoint);

		// iterate through all function instances and create
		// UserBreakpointInstances
		for (FunctionInstanceList::ConstIterator it
					= function->Instances().GetIterator();
				FunctionInstance* instance = it.Next();) {
			TRACE_CONTROL("  function instance %p: range: %#llx - %#llx\n",
				instance, instance->Address(),
				instance->Address() + instance->Size());

			// get the breakpoint address for the instance
			target_addr_t instanceAddress = 0;
			if (instance == functionInstance) {
				instanceAddress = address;
			} else if (functionInstance->SourceFile() != NULL) {
				// We have a source file, so get the address for the source
				// location.
				Statement* statement = NULL;
				functionDebugInfo = instance->GetFunctionDebugInfo();
				functionDebugInfo->GetSpecificImageDebugInfo()
					->GetStatementAtSourceLocation(functionDebugInfo,
						sourceLocation, statement);
				if (statement != NULL) {
					instanceAddress = statement->CoveringAddressRange().Start();
						// TODO: What about BreakpointAllowed()?
					statement->ReleaseReference();
				}
			}

			TRACE_CONTROL("    breakpoint address using source info: %llx\n",
				instanceAddress);

			if (instanceAddress == 0) {
				// No source file (or we failed getting the statement), so try
				// to use the same relative address.
				if (relativeAddress > instance->Size())
					continue;
				instanceAddress = instance->Address() + relativeAddress;
			}

			TRACE_CONTROL("    final breakpoint address: %llx\n",
				instanceAddress);

			UserBreakpointInstance* breakpointInstance = new(std::nothrow)
				UserBreakpointInstance(userBreakpoint, instanceAddress);
			if (breakpointInstance == NULL
				|| !userBreakpoint->AddInstance(breakpointInstance)) {
				delete breakpointInstance;
				return;
			}

			TRACE_CONTROL("  breakpoint instance: %p\n", breakpointInstance);
		}
	}

	locker.Unlock();

	_HandleSetUserBreakpoint(userBreakpoint, enabled);
}


void
TeamDebugger::_HandleSetUserBreakpoint(UserBreakpoint* breakpoint, bool enabled)
{
	status_t error = fBreakpointManager->InstallUserBreakpoint(breakpoint,
		enabled);
	if (error != B_OK) {
		_NotifyUser("Install Breakpoint", "Failed to install breakpoint: %s",
			strerror(error));
	}
}


void
TeamDebugger::_HandleClearUserBreakpoint(target_addr_t address)
{
	TRACE_CONTROL("TeamDebugger::_HandleClearUserBreakpoint(%#llx)\n", address);

	AutoLocker< ::Team> locker(fTeam);

	Breakpoint* breakpoint = fTeam->BreakpointAtAddress(address);
	if (breakpoint == NULL || breakpoint->FirstUserBreakpoint() == NULL)
		return;
	UserBreakpoint* userBreakpoint
		= breakpoint->FirstUserBreakpoint()->GetUserBreakpoint();
	BReference<UserBreakpoint> userBreakpointReference(userBreakpoint);

	locker.Unlock();

	_HandleClearUserBreakpoint(userBreakpoint);
}


void
TeamDebugger::_HandleClearUserBreakpoint(UserBreakpoint* breakpoint)
{
	fBreakpointManager->UninstallUserBreakpoint(breakpoint);
}


void
TeamDebugger::_HandleInspectAddress(target_addr_t address,
	TeamMemoryBlock::Listener* listener)
{
	TRACE_CONTROL("TeamDebugger::_HandleInspectAddress(%" B_PRIx64 ", %p)\n",
		address, listener);

	TeamMemoryBlock* memoryBlock = fMemoryBlockManager
		->GetMemoryBlock(address);

	if (memoryBlock == NULL) {
		_NotifyUser("Inspect Address", "Failed to allocate memory block");
		return;
	}

	if (!memoryBlock->HasListener(listener))
		memoryBlock->AddListener(listener);

	if (!memoryBlock->IsValid()) {
		AutoLocker< ::Team> teamLocker(fTeam);

		TeamMemory* memory = fTeam->GetTeamMemory();
		// schedule the job
		status_t result;
		if ((result = fWorker->ScheduleJob(
			new(std::nothrow) RetrieveMemoryBlockJob(fTeam, memory,
				memoryBlock),
			this)) != B_OK) {
			memoryBlock->ReleaseReference();
			_NotifyUser("Inspect Address", "Failed to retrieve memory data: %s",
				strerror(result));
		}
	} else
		memoryBlock->NotifyDataRetrieved();

}


ThreadHandler*
TeamDebugger::_GetThreadHandler(thread_id threadID)
{
	AutoLocker< ::Team> locker(fTeam);

	ThreadHandler* handler = fThreadHandlers.Lookup(threadID);
	if (handler != NULL)
		handler->AcquireReference();
	return handler;
}


status_t
TeamDebugger::_AddImage(const ImageInfo& imageInfo, Image** _image)
{
	LocatableFile* file = NULL;
	if (strchr(imageInfo.Name(), '/') != NULL)
		file = fFileManager->GetTargetFile(imageInfo.Name());
	BReference<LocatableFile> imageFileReference(file, true);

	Image* image;
	status_t error = fTeam->AddImage(imageInfo, file, &image);
	if (error != B_OK)
		return error;

	ImageDebugInfoRequested(image);

	ImageHandler* imageHandler = new(std::nothrow) ImageHandler(this, image);
	if (imageHandler != NULL)
		fImageHandlers->Insert(imageHandler);

	if (_image != NULL)
		*_image = image;

	return B_OK;
}


void
TeamDebugger::_LoadSettings()
{
	// get the team name
	AutoLocker< ::Team> locker(fTeam);
	BString teamName = fTeam->Name();
	locker.Unlock();

	// load the settings
	if (fSettingsManager->LoadTeamSettings(teamName, fTeamSettings) != B_OK)
		return;

	// create the saved breakpoints
	for (int32 i = 0; const BreakpointSetting* breakpointSetting
			= fTeamSettings.BreakpointAt(i); i++) {
		if (breakpointSetting->GetFunctionID() == NULL)
			continue;

		// get the source file, if any
		LocatableFile* sourceFile = NULL;
		if (breakpointSetting->SourceFile().Length() > 0) {
			sourceFile = fFileManager->GetSourceFile(
				breakpointSetting->SourceFile());
			if (sourceFile == NULL)
				continue;
		}
		BReference<LocatableFile> sourceFileReference(sourceFile, true);

		// create the breakpoint
		UserBreakpointLocation location(breakpointSetting->GetFunctionID(),
			sourceFile, breakpointSetting->GetSourceLocation(),
			breakpointSetting->RelativeAddress());

		UserBreakpoint* breakpoint = new(std::nothrow) UserBreakpoint(location);
		if (breakpoint == NULL)
			return;
		BReference<UserBreakpoint> breakpointReference(breakpoint, true);

		// install it
		fBreakpointManager->InstallUserBreakpoint(breakpoint,
			breakpointSetting->IsEnabled());
	}

	const TeamUiSettings* uiSettings = fTeamSettings.UiSettingFor(
		fUserInterface->ID());
	if (uiSettings != NULL)
			fUserInterface->LoadSettings(uiSettings);
}


void
TeamDebugger::_SaveSettings()
{
	// get the settings
	AutoLocker< ::Team> locker(fTeam);
	TeamSettings settings;
	if (settings.SetTo(fTeam) != B_OK)
		return;

	TeamUiSettings* uiSettings = NULL;
	if (fUserInterface->SaveSettings(uiSettings) != B_OK)
		return;
	if (uiSettings != NULL)
		settings.AddUiSettings(uiSettings);

	// preserve the UI settings from our cached copy.
	for (int32 i = 0; i < fTeamSettings.CountUiSettings(); i++) {
		const TeamUiSettings* oldUiSettings = fTeamSettings.UiSettingAt(i);
		if (strcmp(oldUiSettings->ID(), fUserInterface->ID()) != 0) {
			TeamUiSettings* clonedSettings = oldUiSettings->Clone();
			if (clonedSettings != NULL)
				settings.AddUiSettings(clonedSettings);
		}
	}
	locker.Unlock();

	// save the settings
	fSettingsManager->SaveTeamSettings(settings);
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

	// notify the user
	fUserInterface->NotifyUser(title, buffer, USER_NOTIFICATION_WARNING);
}


// #pragma mark - Listener


TeamDebugger::Listener::~Listener()
{
}
