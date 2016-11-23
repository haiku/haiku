/*
 * Copyright 2009-2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2010-2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "controllers/TeamDebugger.h"

#include <stdarg.h>
#include <stdio.h>

#include <new>

#include <Entry.h>
#include <InterfaceDefs.h>
#include <Message.h>
#include <StringList.h>

#include <AutoDeleter.h>
#include <AutoLocker.h>

#include "debug_utils.h"
#include "syscall_numbers.h"

#include "Architecture.h"
#include "BreakpointManager.h"
#include "BreakpointSetting.h"
#include "CpuState.h"
#include "DebugEvent.h"
#include "DebuggerInterface.h"
#include "DebugReportGenerator.h"
#include "ExpressionInfo.h"
#include "FileManager.h"
#include "Function.h"
#include "FunctionID.h"
#include "ImageDebugInfo.h"
#include "ImageDebugInfoLoadingState.h"
#include "ImageDebugLoadingStateHandler.h"
#include "ImageDebugLoadingStateHandlerRoster.h"
#include "Jobs.h"
#include "LocatableFile.h"
#include "MessageCodes.h"
#include "NoOpSettingsManager.h"
#include "SettingsManager.h"
#include "SourceCode.h"
#include "SourceLanguage.h"
#include "SpecificImageDebugInfo.h"
#include "SpecificImageDebugInfoLoadingState.h"
#include "StackFrame.h"
#include "StackFrameValues.h"
#include "Statement.h"
#include "SymbolInfo.h"
#include "TeamDebugInfo.h"
#include "TeamInfo.h"
#include "TeamMemoryBlock.h"
#include "TeamMemoryBlockManager.h"
#include "TeamSettings.h"
#include "TeamSignalSettings.h"
#include "TeamUiSettings.h"
#include "Tracing.h"
#include "ValueNode.h"
#include "ValueNodeContainer.h"
#include "Variable.h"
#include "WatchpointManager.h"


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


// #pragma mark - ImageInfoPendingThread


struct TeamDebugger::ImageInfoPendingThread {
public:
	ImageInfoPendingThread(image_id image, thread_id thread)
		:
		fImage(image),
		fThread(thread)
	{
	}

	~ImageInfoPendingThread()
	{
	}

	image_id ImageID() const
	{
		return fImage;
	}

	thread_id ThreadID() const
	{
		return fThread;
	}

private:
	image_id				fImage;
	thread_id				fThread;

public:
	ImageInfoPendingThread*	fNext;
};


// #pragma mark - ImageHandlerHashDefinition


struct TeamDebugger::ImageInfoPendingThreadHashDefinition {
	typedef image_id				KeyType;
	typedef	ImageInfoPendingThread	ValueType;

	size_t HashKey(image_id key) const
	{
		return (size_t)key;
	}

	size_t Hash(const ImageInfoPendingThread* value) const
	{
		return HashKey(value->ImageID());
	}

	bool Compare(image_id key, const ImageInfoPendingThread* value) const
	{
		return value->ImageID() == key;
	}

	ImageInfoPendingThread*& GetLink(ImageInfoPendingThread* value) const
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
	fIsPostMortem(false),
	fImageHandlers(NULL),
	fImageInfoPendingThreads(NULL),
	fDebuggerInterface(NULL),
	fFileManager(NULL),
	fWorker(NULL),
	fBreakpointManager(NULL),
	fWatchpointManager(NULL),
	fMemoryBlockManager(NULL),
	fReportGenerator(NULL),
	fDebugEventListener(-1),
	fUserInterface(userInterface),
	fTerminating(false),
	fKillTeamOnQuit(false),
	fCommandLineArgc(0),
	fCommandLineArgv(NULL),
	fExecPending(false)
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

	if (fImageInfoPendingThreads != NULL) {
		ImageInfoPendingThread* thread = fImageInfoPendingThreads->Clear(true);
		while (thread != NULL) {
			ImageInfoPendingThread* next = thread->fNext;
			delete thread;
			thread = next;
		}
	}

	if (fReportGenerator != NULL) {
		fReportGenerator->Lock();
		fReportGenerator->Quit();
	}

	delete fWorker;

	delete fImageInfoPendingThreads;

	delete fBreakpointManager;
	delete fWatchpointManager;
	delete fMemoryBlockManager;
	delete fTeam;
	delete fFileManager;

	for (int i = 0; i < fCommandLineArgc; i++) {
		if (fCommandLineArgv[i] != NULL)
			free(const_cast<char*>(fCommandLineArgv[i]));
	}

	delete [] fCommandLineArgv;

	fListener->TeamDebuggerQuit(this);
}


status_t
TeamDebugger::Init(DebuggerInterface* interface, thread_id threadID, int argc,
	const char* const* argv, bool stopInMain)
{
	bool targetIsLocal = true;
		// TODO: Support non-local targets!

	// the first thing we want to do when running
	PostMessage(MSG_LOAD_SETTINGS);

	status_t error = _HandleSetArguments(argc, argv);
	if (error != B_OK)
		return error;

	if (fSettingsManager == NULL) {
		// if we have not been provided with a settings manager,
		// simply use the no-op manager by default.
		fSettingsManager = new(std::nothrow) NoOpSettingsManager;
		if (fSettingsManager == NULL)
			return B_NO_MEMORY;
	}

	fDebuggerInterface = interface;
	fDebuggerInterface->AcquireReference();
	fTeamID = interface->TeamID();
	fIsPostMortem = interface->IsPostMortem();


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
	TeamInfo teamInfo;
	error = fDebuggerInterface->GetTeamInfo(teamInfo);
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
	fTeam->SetName(teamInfo.Arguments());
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

	fImageInfoPendingThreads = new(std::nothrow) ImageInfoPendingThreadTable;
	if (fImageInfoPendingThreads == NULL)
		return B_NO_MEMORY;

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

	// create the watchpoint manager
	fWatchpointManager = new(std::nothrow) WatchpointManager(fTeam,
		fDebuggerInterface);
	if (fWatchpointManager == NULL)
		return B_NO_MEMORY;

	error = fWatchpointManager->Init();
	if (error != B_OK)
		return error;

	// create the memory block manager
	fMemoryBlockManager = new(std::nothrow) TeamMemoryBlockManager();
	if (fMemoryBlockManager == NULL)
		return B_NO_MEMORY;

	error = fMemoryBlockManager->Init();
	if (error != B_OK)
		return error;

	// create the debug report generator
	fReportGenerator = new(std::nothrow) DebugReportGenerator(fTeam, this,
		fDebuggerInterface);
	if (fReportGenerator == NULL)
		return B_NO_MEMORY;

	error = fReportGenerator->Init();
	if (error != B_OK)
		return error;

	// set team debugging flags
	fDebuggerInterface->SetTeamDebuggingFlags(
		B_TEAM_DEBUG_THREADS | B_TEAM_DEBUG_IMAGES
			| B_TEAM_DEBUG_POST_SYSCALL | B_TEAM_DEBUG_SIGNALS
			| B_TEAM_DEBUG_TEAM_CREATION);

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
				fWorker, fDebuggerInterface, this, fBreakpointManager);
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

	// create the debug event listener (for live debugging only)
	if (!fDebuggerInterface->IsPostMortem()) {
		char buffer[128];
		snprintf(buffer, sizeof(buffer), "team %" B_PRId32 " debug listener",
			fTeamID);
		fDebugEventListener = spawn_thread(_DebugEventListenerEntry, buffer,
			B_NORMAL_PRIORITY, this);
		if (fDebugEventListener < 0)
			return fDebugEventListener;

		resume_thread(fDebugEventListener);
	}

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
	if (threadID >= 0 && !fDebuggerInterface->IsPostMortem()) {
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
		case MSG_THREAD_SET_ADDRESS:
		case MSG_THREAD_STOP:
		case MSG_THREAD_STEP_OVER:
		case MSG_THREAD_STEP_INTO:
		case MSG_THREAD_STEP_OUT:
		{
			int32 threadID;
			target_addr_t address;
			if (message->FindInt32("thread", &threadID) != B_OK)
				break;

			if (message->FindUInt64("address", &address) != B_OK)
				address = 0;

			if (ThreadHandler* handler = _GetThreadHandler(threadID)) {
				handler->HandleThreadAction(message->what, address);
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

			if (message->FindPointer("breakpoint", (void**)&breakpoint)
				== B_OK) {
				breakpointReference.SetTo(breakpoint, true);
			} else if (message->FindUInt64("address", &address) != B_OK)
				break;

			if (message->what == MSG_SET_BREAKPOINT) {
				bool enabled;
				if (message->FindBool("enabled", &enabled) != B_OK)
					enabled = true;

				bool hidden;
				if (message->FindBool("hidden", &hidden) != B_OK)
					hidden = false;

				if (breakpoint != NULL)
					_HandleSetUserBreakpoint(breakpoint, enabled);
				else
					_HandleSetUserBreakpoint(address, enabled, hidden);
			} else {
				if (breakpoint != NULL)
					_HandleClearUserBreakpoint(breakpoint);
				else
					_HandleClearUserBreakpoint(address);
			}

			break;
		}

		case MSG_SET_BREAKPOINT_CONDITION:
		{
			UserBreakpoint* breakpoint = NULL;
			BReference<UserBreakpoint> breakpointReference;
			if (message->FindPointer("breakpoint", (void**)&breakpoint)
				!= B_OK) {
				break;
			}

			breakpointReference.SetTo(breakpoint, true);

			const char* condition;
			if (message->FindString("condition", &condition) != B_OK)
				break;

			AutoLocker< ::Team> teamLocker(fTeam);
			breakpoint->SetCondition(condition);
			fTeam->NotifyUserBreakpointChanged(breakpoint);

			break;
		}

		case MSG_CLEAR_BREAKPOINT_CONDITION:
		{
			UserBreakpoint* breakpoint = NULL;
			BReference<UserBreakpoint> breakpointReference;
			if (message->FindPointer("breakpoint", (void**)&breakpoint)
				!= B_OK)
				break;

			breakpointReference.SetTo(breakpoint, true);

			AutoLocker< ::Team> teamLocker(fTeam);
			breakpoint->SetCondition(NULL);
			fTeam->NotifyUserBreakpointChanged(breakpoint);

			break;
		}

		case MSG_STOP_ON_IMAGE_LOAD:
		{
			bool enabled;
			bool useNames;
			if (message->FindBool("enabled", &enabled) != B_OK)
				break;

			if (message->FindBool("useNames", &useNames) != B_OK)
				break;

			AutoLocker< ::Team> teamLocker(fTeam);
			fTeam->SetStopOnImageLoad(enabled, useNames);
			break;
		}

		case MSG_ADD_STOP_IMAGE_NAME:
		{
			BString imageName;
			if (message->FindString("name", &imageName) != B_OK)
				break;

			AutoLocker< ::Team> teamLocker(fTeam);
			fTeam->AddStopImageName(imageName);
			break;
		}

		case MSG_REMOVE_STOP_IMAGE_NAME:
		{
			BString imageName;
			if (message->FindString("name", &imageName) != B_OK)
				break;

			AutoLocker< ::Team> teamLocker(fTeam);
			fTeam->RemoveStopImageName(imageName);
			break;
		}

		case MSG_SET_DEFAULT_SIGNAL_DISPOSITION:
		{
			int32 disposition;
			if (message->FindInt32("disposition", &disposition) != B_OK)
				break;

			AutoLocker< ::Team> teamLocker(fTeam);
			fTeam->SetDefaultSignalDisposition(disposition);
			break;
		}

		case MSG_SET_CUSTOM_SIGNAL_DISPOSITION:
		{
			int32 signal;
			int32 disposition;
			if (message->FindInt32("signal", &signal) != B_OK
				|| message->FindInt32("disposition", &disposition) != B_OK) {
				break;
			}

			AutoLocker< ::Team> teamLocker(fTeam);
			fTeam->SetCustomSignalDisposition(signal, disposition);
			break;
		}

		case MSG_REMOVE_CUSTOM_SIGNAL_DISPOSITION:
		{
			int32 signal;
			if (message->FindInt32("signal", &signal) != B_OK)
				break;

			AutoLocker< ::Team> teamLocker(fTeam);
			fTeam->RemoveCustomSignalDisposition(signal);
			break;
		}

		case MSG_SET_WATCHPOINT:
		case MSG_CLEAR_WATCHPOINT:
		{
			Watchpoint* watchpoint = NULL;
			BReference<Watchpoint> watchpointReference;
			uint64 address = 0;
			uint32 type = 0;
			int32 length = 0;

			if (message->FindPointer("watchpoint", (void**)&watchpoint)
					== B_OK) {
				watchpointReference.SetTo(watchpoint, true);
			} else if (message->FindUInt64("address", &address) != B_OK)
				break;

			if (message->what == MSG_SET_WATCHPOINT) {
				if (watchpoint == NULL && (message->FindUInt32("type", &type)
							!= B_OK
						|| message->FindInt32("length", &length) != B_OK)) {
					break;
				}

				bool enabled;
				if (message->FindBool("enabled", &enabled) != B_OK)
					enabled = true;

				if (watchpoint != NULL)
					_HandleSetWatchpoint(watchpoint, enabled);
				else
					_HandleSetWatchpoint(address, type, length, enabled);
			} else {
				if (watchpoint != NULL)
					_HandleClearWatchpoint(watchpoint);
				else
					_HandleClearWatchpoint(address);
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

		case MSG_WRITE_TARGET_MEMORY:
		{
			target_addr_t address;
			if (message->FindUInt64("address", &address) != B_OK)
				break;

			void* data;
			if (message->FindPointer("data", &data) != B_OK)
				break;

			target_size_t size;
			if (message->FindUInt64("size", &size) != B_OK)
				break;

			_HandleWriteMemory(address, data, size);
			break;
		}

		case MSG_EVALUATE_EXPRESSION:
		{
			SourceLanguage* language;
			if (message->FindPointer("language",
				reinterpret_cast<void**>(&language)) != B_OK) {
				break;
			}

			// ExpressionEvaluationRequested() acquires a reference
			// to both the language and the expression info on our behalf.
			BReference<SourceLanguage> reference(language, true);

			ExpressionInfo* info;
			if (message->FindPointer("info",
				reinterpret_cast<void**>(&info)) != B_OK) {
				break;
			}

			BReference<ExpressionInfo> infoReference(info, true);

			StackFrame* frame;
			if (message->FindPointer("frame",
				reinterpret_cast<void**>(&frame)) != B_OK) {
				// the stack frame isn't needed, unless variable
				// evaluation is desired.
				frame = NULL;
			}

			::Thread* thread;
			if (message->FindPointer("thread",
				reinterpret_cast<void**>(&thread)) != B_OK) {
				// the thread isn't needed, unless variable
				// evaluation is desired.
				thread = NULL;
			}

			_HandleEvaluateExpression(language, info, frame, thread);
			break;
		}

		case MSG_GENERATE_DEBUG_REPORT:
		{
			fReportGenerator->PostMessage(message);
			break;
		}

		case MSG_WRITE_CORE_FILE:
		{
			entry_ref ref;
			if (message->FindRef("target", &ref) != B_OK)
				break;

			_HandleWriteCoreFile(ref);
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

		case MSG_TEAM_RESTART_REQUESTED:
		{
			if (fCommandLineArgc == 0)
				break;

			_SaveSettings();
			fListener->TeamDebuggerRestartRequested(this);
			break;
		}

		case MSG_DEBUG_INFO_NEEDS_USER_INPUT:
		{
			Job* job;
			ImageDebugInfoLoadingState* state;
			if (message->FindPointer("job", (void**)&job) != B_OK)
				break;
			if (message->FindPointer("state", (void**)&state) != B_OK)
				break;

			_HandleDebugInfoJobUserInput(state);
			fWorker->ResumeJob(job);
			break;
		}

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
TeamDebugger::SourceEntryInvalidateRequested(LocatableFile* sourceFile)
{
	AutoLocker< ::Team> locker(fTeam);

	fTeam->DebugInfo()->ClearSourceCode(sourceFile);
}


void
TeamDebugger::FunctionSourceCodeRequested(FunctionInstance* functionInstance,
	bool forceDisassembly)
{
	Function* function = functionInstance->GetFunction();

	// mark loading
	AutoLocker< ::Team> locker(fTeam);

	if (forceDisassembly && functionInstance->SourceCodeState()
			!= FUNCTION_SOURCE_NOT_LOADED) {
		return;
	} else if (!forceDisassembly && function->SourceCodeState()
			== FUNCTION_SOURCE_LOADED) {
		return;
	}

	functionInstance->SetSourceCode(NULL, FUNCTION_SOURCE_LOADING);

	bool loadForFunction = false;
	if (!forceDisassembly && (function->SourceCodeState()
				== FUNCTION_SOURCE_NOT_LOADED
			|| function->SourceCodeState() == FUNCTION_SOURCE_SUPPRESSED)) {
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
	LoadImageDebugInfoJob::ScheduleIfNecessary(fWorker, image, this);
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
			fTeam->GetTeamTypeInformation(), container,	valueNode), this);
	if (error != B_OK) {
		// scheduling failed -- set the value to invalid
		valueNode->SetLocationAndValue(NULL, NULL, error);
	}
}

void
TeamDebugger::ValueNodeWriteRequested(ValueNode* node, CpuState* state,
	Value* newValue)
{
	// schedule the job
	status_t error = fWorker->ScheduleJob(
		new(std::nothrow) WriteValueNodeValueJob(fDebuggerInterface,
			fDebuggerInterface->GetArchitecture(), state,
			fTeam->GetTeamTypeInformation(), node, newValue), this);
	if (error != B_OK) {
		BString message;
		message.SetToFormat("Request to write new value for variable %s "
			"failed: %s.\n", node->Name().String(), strerror(error));
		fUserInterface->NotifyUser("Error", message.String(),
			USER_NOTIFICATION_ERROR);
	}
}


void
TeamDebugger::ThreadActionRequested(thread_id threadID,
	uint32 action, target_addr_t address)
{
	BMessage message(action);
	message.AddInt32("thread", threadID);
	message.AddUInt64("address", address);
	PostMessage(&message);
}


void
TeamDebugger::SetBreakpointRequested(target_addr_t address, bool enabled,
	bool hidden)
{
	BMessage message(MSG_SET_BREAKPOINT);
	message.AddUInt64("address", (uint64)address);
	message.AddBool("enabled", enabled);
	message.AddBool("hidden", hidden);
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
TeamDebugger::SetBreakpointConditionRequested(UserBreakpoint* breakpoint,
	const char* condition)
{
	BMessage message(MSG_SET_BREAKPOINT_CONDITION);
	BReference<UserBreakpoint> breakpointReference(breakpoint);
	if (message.AddPointer("breakpoint", breakpoint) == B_OK
		&& message.AddString("condition", condition) == B_OK
		&& PostMessage(&message) == B_OK) {
		breakpointReference.Detach();
	}
}


void
TeamDebugger::ClearBreakpointConditionRequested(UserBreakpoint* breakpoint)
{
	BMessage message(MSG_CLEAR_BREAKPOINT_CONDITION);
	BReference<UserBreakpoint> breakpointReference(breakpoint);
	if (message.AddPointer("breakpoint", breakpoint) == B_OK
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
TeamDebugger::SetStopOnImageLoadRequested(bool enabled, bool useImageNames)
{
	BMessage message(MSG_STOP_ON_IMAGE_LOAD);
	message.AddBool("enabled", enabled);
	message.AddBool("useNames", useImageNames);
	PostMessage(&message);
}


void
TeamDebugger::AddStopImageNameRequested(const char* name)
{
	BMessage message(MSG_ADD_STOP_IMAGE_NAME);
	message.AddString("name", name);
	PostMessage(&message);
}


void
TeamDebugger::RemoveStopImageNameRequested(const char* name)
{
	BMessage message(MSG_REMOVE_STOP_IMAGE_NAME);
	message.AddString("name", name);
	PostMessage(&message);
}


void
TeamDebugger::SetDefaultSignalDispositionRequested(int32 disposition)
{
	BMessage message(MSG_SET_DEFAULT_SIGNAL_DISPOSITION);
	message.AddInt32("disposition", disposition);
	PostMessage(&message);
}


void
TeamDebugger::SetCustomSignalDispositionRequested(int32 signal,
	int32 disposition)
{
	BMessage message(MSG_SET_CUSTOM_SIGNAL_DISPOSITION);
	message.AddInt32("signal", signal);
	message.AddInt32("disposition", disposition);
	PostMessage(&message);
}


void
TeamDebugger::RemoveCustomSignalDispositionRequested(int32 signal)
{
	BMessage message(MSG_REMOVE_CUSTOM_SIGNAL_DISPOSITION);
	message.AddInt32("signal", signal);
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
TeamDebugger::SetWatchpointRequested(target_addr_t address, uint32 type,
	int32 length, bool enabled)
{
	BMessage message(MSG_SET_WATCHPOINT);
	message.AddUInt64("address", (uint64)address);
	message.AddUInt32("type", type);
	message.AddInt32("length", length);
	message.AddBool("enabled", enabled);
	PostMessage(&message);
}


void
TeamDebugger::SetWatchpointEnabledRequested(Watchpoint* watchpoint,
	bool enabled)
{
	BMessage message(MSG_SET_WATCHPOINT);
	BReference<Watchpoint> watchpointReference(watchpoint);
	if (message.AddPointer("watchpoint", watchpoint) == B_OK
		&& message.AddBool("enabled", enabled) == B_OK
		&& PostMessage(&message) == B_OK) {
		watchpointReference.Detach();
	}
}


void
TeamDebugger::ClearWatchpointRequested(target_addr_t address)
{
	BMessage message(MSG_CLEAR_WATCHPOINT);
	message.AddUInt64("address", (uint64)address);
	PostMessage(&message);
}


void
TeamDebugger::ClearWatchpointRequested(Watchpoint* watchpoint)
{
	BMessage message(MSG_CLEAR_WATCHPOINT);
	BReference<Watchpoint> watchpointReference(watchpoint);
	if (message.AddPointer("watchpoint", watchpoint) == B_OK
		&& PostMessage(&message) == B_OK) {
		watchpointReference.Detach();
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


void
TeamDebugger::MemoryWriteRequested(target_addr_t address, const void* data,
	target_size_t size)
{
	BMessage message(MSG_WRITE_TARGET_MEMORY);
	message.AddUInt64("address", address);
	message.AddPointer("data", data);
	message.AddUInt64("size", size);
	PostMessage(&message);
}


void
TeamDebugger::ExpressionEvaluationRequested(SourceLanguage* language,
	ExpressionInfo* info, StackFrame* frame, ::Thread* thread)
{
	BMessage message(MSG_EVALUATE_EXPRESSION);
	message.AddPointer("language", language);
	message.AddPointer("info", info);
	if (frame != NULL)
		message.AddPointer("frame", frame);
	if (thread != NULL)
		message.AddPointer("thread", thread);

	BReference<SourceLanguage> languageReference(language);
	BReference<ExpressionInfo> infoReference(info);
	if (PostMessage(&message) == B_OK) {
		languageReference.Detach();
		infoReference.Detach();
	}
}


void
TeamDebugger::DebugReportRequested(entry_ref* targetPath)
{
	BMessage message(MSG_GENERATE_DEBUG_REPORT);
	message.AddRef("target", targetPath);
	PostMessage(&message);
}


void
TeamDebugger::WriteCoreFileRequested(entry_ref* targetPath)
{
	BMessage message(MSG_WRITE_CORE_FILE);
	message.AddRef("target", targetPath);
	PostMessage(&message);
}


void
TeamDebugger::TeamRestartRequested()
{
	PostMessage(MSG_TEAM_RESTART_REQUESTED);
}


bool
TeamDebugger::UserInterfaceQuitRequested(QuitOption quitOption)
{
	bool askUser = false;
	switch (quitOption) {
		case QUIT_OPTION_ASK_USER:
			askUser = true;
			break;

		case QUIT_OPTION_ASK_KILL_TEAM:
			fKillTeamOnQuit = true;
			break;

		case QUIT_OPTION_ASK_RESUME_TEAM:
			break;
	}

	if (askUser) {
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
			case -1:
				return false;
			case 2:
				// Detach from the team and resume and stopped threads.
				break;
		}
	}

	PostMessage(B_QUIT_REQUESTED);

	return true;
}


void
TeamDebugger::JobStarted(Job* job)
{
	BString description(job->GetDescription());
	if (!description.IsEmpty()) {
		description.Append(B_UTF8_ELLIPSIS);
		fUserInterface->NotifyBackgroundWorkStatus(description.String());
	}
}


void
TeamDebugger::JobDone(Job* job)
{
	TRACE_JOBS("TeamDebugger::JobDone(%p)\n", job);
	_ResetUserBackgroundStatusIfNeeded();
}


void
TeamDebugger::JobWaitingForInput(Job* job)
{
	LoadImageDebugInfoJob* infoJob = dynamic_cast<LoadImageDebugInfoJob*>(job);

	if (infoJob == NULL)
		return;

	BMessage message(MSG_DEBUG_INFO_NEEDS_USER_INPUT);
	message.AddPointer("job", infoJob);
	message.AddPointer("state", infoJob->GetLoadingState());
	PostMessage(&message);
}


void
TeamDebugger::JobFailed(Job* job)
{
	TRACE_JOBS("TeamDebugger::JobFailed(%p)\n", job);
	// TODO: notify user
	_ResetUserBackgroundStatusIfNeeded();
}


void
TeamDebugger::JobAborted(Job* job)
{
	TRACE_JOBS("TeamDebugger::JobAborted(%p)\n", job);
	// TODO: For a stack frame source loader thread we should reset the
	// loading state! Asynchronously due to locking order.
	_ResetUserBackgroundStatusIfNeeded();
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
			TRACE_EVENTS("TeamDebugger for team %" B_PRId32 ": received event "
				"from team %" B_PRId32 "!\n", fTeamID, event->Team());
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
	TRACE_EVENTS("TeamDebugger::_HandleDebuggerMessage(): %" B_PRId32 "\n",
		event->EventType());

	bool handled = false;

	ThreadHandler* handler = _GetThreadHandler(event->Thread());
	BReference<ThreadHandler> handlerReference(handler);

	switch (event->EventType()) {
		case B_DEBUGGER_MESSAGE_THREAD_DEBUGGED:
			TRACE_EVENTS("B_DEBUGGER_MESSAGE_THREAD_DEBUGGED: thread: %"
				B_PRId32 "\n", event->Thread());

			if (handler != NULL) {
				handled = handler->HandleThreadDebugged(
					dynamic_cast<ThreadDebuggedEvent*>(event));
			}
			break;
		case B_DEBUGGER_MESSAGE_DEBUGGER_CALL:
			TRACE_EVENTS("B_DEBUGGER_MESSAGE_DEBUGGER_CALL: thread: %" B_PRId32
				"\n", event->Thread());

			if (handler != NULL) {
				handled = handler->HandleDebuggerCall(
					dynamic_cast<DebuggerCallEvent*>(event));
			}
			break;
		case B_DEBUGGER_MESSAGE_BREAKPOINT_HIT:
			TRACE_EVENTS("B_DEBUGGER_MESSAGE_BREAKPOINT_HIT: thread: %" B_PRId32
				"\n", event->Thread());

			if (handler != NULL) {
				handled = handler->HandleBreakpointHit(
					dynamic_cast<BreakpointHitEvent*>(event));
			}
			break;
		case B_DEBUGGER_MESSAGE_WATCHPOINT_HIT:
			TRACE_EVENTS("B_DEBUGGER_MESSAGE_WATCHPOINT_HIT: thread: %" B_PRId32
				"\n", event->Thread());

			if (handler != NULL) {
				handled = handler->HandleWatchpointHit(
					dynamic_cast<WatchpointHitEvent*>(event));
			}
			break;
		case B_DEBUGGER_MESSAGE_SINGLE_STEP:
			TRACE_EVENTS("B_DEBUGGER_MESSAGE_SINGLE_STEP: thread: %" B_PRId32
				"\n", event->Thread());

			if (handler != NULL) {
				handled = handler->HandleSingleStep(
					dynamic_cast<SingleStepEvent*>(event));
			}
			break;
		case B_DEBUGGER_MESSAGE_EXCEPTION_OCCURRED:
			TRACE_EVENTS("B_DEBUGGER_MESSAGE_EXCEPTION_OCCURRED: thread: %"
				B_PRId32 "\n", event->Thread());

			if (handler != NULL) {
				handled = handler->HandleExceptionOccurred(
					dynamic_cast<ExceptionOccurredEvent*>(event));
			}
			break;
//		case B_DEBUGGER_MESSAGE_TEAM_CREATED:
//printf("B_DEBUGGER_MESSAGE_TEAM_CREATED: team: %ld\n", message.team_created.new_team);
//			break;
		case B_DEBUGGER_MESSAGE_TEAM_DELETED:
		{
			TRACE_EVENTS("B_DEBUGGER_MESSAGE_TEAM_DELETED: team: %" B_PRId32
				"\n", event->Team());
			TeamDeletedEvent* teamEvent
				= dynamic_cast<TeamDeletedEvent*>(event);
			handled = _HandleTeamDeleted(teamEvent);
			break;
		}
		case B_DEBUGGER_MESSAGE_TEAM_EXEC:
		{
			TRACE_EVENTS("B_DEBUGGER_MESSAGE_TEAM_EXEC: team: %" B_PRId32 "\n",
				event->Team());

			TeamExecEvent* teamEvent
				= dynamic_cast<TeamExecEvent*>(event);
			_PrepareForTeamExec(teamEvent);
			break;
		}
		case B_DEBUGGER_MESSAGE_THREAD_CREATED:
		{
			ThreadCreatedEvent* threadEvent
				= dynamic_cast<ThreadCreatedEvent*>(event);
			TRACE_EVENTS("B_DEBUGGER_MESSAGE_THREAD_CREATED: thread: %" B_PRId32
				"\n", threadEvent->NewThread());
			handled = _HandleThreadCreated(threadEvent);
			break;
		}
		case DEBUGGER_MESSAGE_THREAD_RENAMED:
		{
			ThreadRenamedEvent* threadEvent
				= dynamic_cast<ThreadRenamedEvent*>(event);
			TRACE_EVENTS("DEBUGGER_MESSAGE_THREAD_RENAMED: thread: %" B_PRId32
				" (\"%s\")\n",
				threadEvent->RenamedThread(), threadEvent->NewName());
			handled = _HandleThreadRenamed(threadEvent);
			break;
		}
		case DEBUGGER_MESSAGE_THREAD_PRIORITY_CHANGED:
		{
			ThreadPriorityChangedEvent* threadEvent
				= dynamic_cast<ThreadPriorityChangedEvent*>(event);
			TRACE_EVENTS("B_DEBUGGER_MESSAGE_THREAD_PRIORITY_CHANGED: thread:"
				" %" B_PRId32 "\n", threadEvent->ChangedThread());
			handled = _HandleThreadPriorityChanged(threadEvent);
			break;
		}
		case B_DEBUGGER_MESSAGE_THREAD_DELETED:
			TRACE_EVENTS("B_DEBUGGER_MESSAGE_THREAD_DELETED: thread: %" B_PRId32
				"\n", event->Thread());
			handled = _HandleThreadDeleted(
				dynamic_cast<ThreadDeletedEvent*>(event));
			break;
		case B_DEBUGGER_MESSAGE_IMAGE_CREATED:
		{
			ImageCreatedEvent* imageEvent
				= dynamic_cast<ImageCreatedEvent*>(event);
			TRACE_EVENTS("B_DEBUGGER_MESSAGE_IMAGE_CREATED: image: \"%s\" "
				"(%" B_PRId32 ")\n", imageEvent->GetImageInfo().Name().String(),
				imageEvent->GetImageInfo().ImageID());
			handled = _HandleImageCreated(imageEvent);
			break;
		}
		case B_DEBUGGER_MESSAGE_IMAGE_DELETED:
		{
			ImageDeletedEvent* imageEvent
				= dynamic_cast<ImageDeletedEvent*>(event);
			TRACE_EVENTS("B_DEBUGGER_MESSAGE_IMAGE_DELETED: image: \"%s\" "
				"(%" B_PRId32 ")\n", imageEvent->GetImageInfo().Name().String(),
				imageEvent->GetImageInfo().ImageID());
			handled = _HandleImageDeleted(imageEvent);
			break;
		}
		case B_DEBUGGER_MESSAGE_POST_SYSCALL:
		{
			PostSyscallEvent* postSyscallEvent
				= dynamic_cast<PostSyscallEvent*>(event);
			TRACE_EVENTS("B_DEBUGGER_MESSAGE_POST_SYSCALL: syscall: %"
				B_PRIu32 "\n", postSyscallEvent->GetSyscallInfo().Syscall());
			handled = _HandlePostSyscall(postSyscallEvent);

			// if a thread was blocked in a syscall when we requested to
			// stop it for debugging, then that request will interrupt
			// said call, and the post syscall event will be all we get
			// in response. Consequently, we need to treat this case as
			// equivalent to having received a thread debugged event.
			AutoLocker< ::Team> teamLocker(fTeam);
			::Thread* thread = fTeam->ThreadByID(event->Thread());
			if (handler != NULL && thread != NULL
				&& thread->StopRequestPending()) {
				handled = handler->HandleThreadDebugged(NULL);
			}
			break;
		}
		case B_DEBUGGER_MESSAGE_SIGNAL_RECEIVED:
		{
			TRACE_EVENTS("B_DEBUGGER_MESSAGE_SIGNAL_RECEIVED: thread: %"
				B_PRId32 "\n", event->Thread());

			if (handler != NULL) {
				handled = handler->HandleSignalReceived(
					dynamic_cast<SignalReceivedEvent*>(event));
			}
			break;
		}
		case B_DEBUGGER_MESSAGE_PRE_SYSCALL:
		case B_DEBUGGER_MESSAGE_PROFILER_UPDATE:
		case B_DEBUGGER_MESSAGE_HANDED_OVER:
			// not interested
			break;
		default:
			WARNING("TeamDebugger for team %" B_PRId32 ": unknown event type: "
				"%" B_PRId32 "\n", fTeamID, event->EventType());
			break;
	}

	if (!handled && event->ThreadStopped())
		fDebuggerInterface->ContinueThread(event->Thread());
}


bool
TeamDebugger::_HandleTeamDeleted(TeamDeletedEvent* event)
{
	char message[64];
	fDebuggerInterface->Close(false);

	snprintf(message, sizeof(message), "Team %" B_PRId32 " has terminated. ",
		event->Team());

	int32 result = fUserInterface->SynchronouslyAskUser("Team terminated",
		message, "Do nothing", "Quit", fCommandLineArgc != 0
			? "Restart team" : NULL);

	switch (result) {
		case 1:
		case -1:
		{
			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		case 2:
		{
			_SaveSettings();
			fListener->TeamDebuggerRestartRequested(this);
			break;
		}
		default:
			break;
	}

	return true;
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
			fWorker, fDebuggerInterface, this, fBreakpointManager);
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

	ImageInfoPendingThread* info = new(std::nothrow) ImageInfoPendingThread(
		event->GetImageInfo().ImageID(), event->Thread());
	if (info == NULL)
		return false;

	fImageInfoPendingThreads->Insert(info);
	return true;
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


bool
TeamDebugger::_HandlePostSyscall(PostSyscallEvent* event)
{
	const SyscallInfo& info = event->GetSyscallInfo();

	switch (info.Syscall()) {
		case SYSCALL_WRITE:
		{
			if ((ssize_t)info.ReturnValue() <= 0)
				break;

			int32 fd;
			target_addr_t address;
			size_t size;
			// TODO: decoding the syscall arguments should probably be
			// factored out into an Architecture method of its own, since
			// there's no guarantee the target architecture has the same
			// endianness as the host. This could re-use the syscall
			// argument parser that strace uses, though that would need to
			// be adapted to handle the aforementioned endian differences.
			// This works for x86{-64} for now though.
			if (fTeam->GetArchitecture()->AddressSize() == 4) {
				const uint32* args = (const uint32*)info.Arguments();
				fd = args[0];
				address = args[3];
				size = args[4];
			} else {
				const uint64* args = (const uint64*)info.Arguments();
				fd = args[0];
				address = args[2];
				size = args[3];
			}

			if (fd == 1 || fd == 2) {
				BString data;

				ssize_t result = fDebuggerInterface->ReadMemoryString(
					address, size, data);
				if (result >= 0)
					fTeam->NotifyConsoleOutputReceived(fd, data);
			}
			break;
		}
		case SYSCALL_WRITEV:
		{
			// TODO: handle
		}
		default:
			break;
	}

	return false;
}


void
TeamDebugger::_PrepareForTeamExec(TeamExecEvent* event)
{
	// NB: must be called with team lock held.

	_SaveSettings();

	// when notified of exec, we need to clear out data related
	// to the old team.
	const ImageList& images = fTeam->Images();

	for (ImageList::ConstIterator it = images.GetIterator();
			Image* image = it.Next();) {
		fBreakpointManager->RemoveImageBreakpoints(image);
	}

	BObjectList<UserBreakpoint> breakpointsToRemove(20, false);
	const UserBreakpointList& breakpoints = fTeam->UserBreakpoints();
	for (UserBreakpointList::ConstIterator it = breakpoints.GetIterator();
			UserBreakpoint* breakpoint = it.Next();) {
		breakpointsToRemove.AddItem(breakpoint);
		breakpoint->AcquireReference();
	}

	for (int32 i = 0; i < breakpointsToRemove.CountItems(); i++) {
		UserBreakpoint* breakpoint = breakpointsToRemove.ItemAt(i);
		fTeam->RemoveUserBreakpoint(breakpoint);
		fTeam->NotifyUserBreakpointChanged(breakpoint);
		breakpoint->ReleaseReference();
	}

	fTeam->ClearImages();
	fTeam->ClearSignalDispositionMappings();
	fExecPending = true;
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
	image_debug_info_state state = image->ImageDebugInfoState();

	bool handlePostExecSetup = fExecPending && image->Type() == B_APP_IMAGE
		&& state != IMAGE_DEBUG_INFO_LOADING;
	// this needs to be done first so that breakpoints are loaded.
	// otherwise, UpdateImageBreakpoints() won't find the appropriate
	// UserBreakpoints to create/install instances for.
	if (handlePostExecSetup) {
		fTeam->SetName(image->Name());
		_LoadSettings();
		fExecPending = false;
	}

	locker.Unlock();

	if (state == IMAGE_DEBUG_INFO_LOADED
		|| state == IMAGE_DEBUG_INFO_UNAVAILABLE) {

		// update breakpoints in the image
		fBreakpointManager->UpdateImageBreakpoints(image);

		ImageInfoPendingThread* thread =  fImageInfoPendingThreads
			->Lookup(imageID);
		if (thread != NULL) {
			fImageInfoPendingThreads->Remove(thread);
			ObjectDeleter<ImageInfoPendingThread> threadDeleter(thread);
			locker.Lock();
			ThreadHandler* handler = _GetThreadHandler(thread->ThreadID());
			BReference<ThreadHandler> handlerReference(handler);
			if (fTeam->StopOnImageLoad()) {

				bool stop = true;
				const BString& imageName = image->Name();
				// only match on the image filename itself
				const char* rawImageName = imageName.String()
					+ imageName.FindLast('/') + 1;
				if (fTeam->StopImageNameListEnabled()) {
					const BStringList& nameList = fTeam->StopImageNames();
					stop = nameList.HasString(rawImageName);
				}

				if (stop && handler != NULL) {
					BString stopReason;
					stopReason.SetToFormat("Image '%s' loaded.",
						rawImageName);
					locker.Unlock();

					if (handler->HandleThreadDebugged(NULL, stopReason))
						return;
				} else
					locker.Unlock();
			} else if (handlePostExecSetup) {
				// in the case of an exec(), we can't stop in main() until
				// the new app image has been loaded, so we know where to
				// set the main breakpoint at.
				SymbolInfo symbolInfo;
				if (fDebuggerInterface->GetSymbolInfo(fTeam->ID(), image->ID(),
						"main", B_SYMBOL_TYPE_TEXT, symbolInfo) == B_OK) {
					handler->SetBreakpointAndRun(symbolInfo.Address());
				}
			} else {
				locker.Unlock();
				fDebuggerInterface->ContinueThread(thread->ThreadID());
			}
		}
	}
}


void
TeamDebugger::_HandleImageFileChanged(image_id imageID)
{
	TRACE_IMAGES("TeamDebugger::_HandleImageFileChanged(%" B_PRId32 ")\n",
		imageID);
// TODO: Reload the debug info!
}


void
TeamDebugger::_HandleSetUserBreakpoint(target_addr_t address, bool enabled,
	bool hidden)
{
	TRACE_CONTROL("TeamDebugger::_HandleSetUserBreakpoint(%#" B_PRIx64
		", %d, %d)\n", address, enabled, hidden);

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

		TRACE_CONTROL("  relative address: %#" B_PRIx64 ", source location: "
			"(%" B_PRId32 ", %" B_PRId32 ")\n", relativeAddress,
			sourceLocation.Line(), sourceLocation.Column());

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

		userBreakpoint->SetHidden(hidden);

		TRACE_CONTROL("  created user breakpoint: %p\n", userBreakpoint);

		// iterate through all function instances and create
		// UserBreakpointInstances
		for (FunctionInstanceList::ConstIterator it
					= function->Instances().GetIterator();
				FunctionInstance* instance = it.Next();) {
			TRACE_CONTROL("  function instance %p: range: %#" B_PRIx64 " - %#"
				B_PRIx64 "\n", instance, instance->Address(),
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

			TRACE_CONTROL("    breakpoint address using source info: %" B_PRIx64
				"\n", instanceAddress);

			if (instanceAddress == 0) {
				// No source file (or we failed getting the statement), so try
				// to use the same relative address.
				if (relativeAddress > instance->Size())
					continue;
				instanceAddress = instance->Address() + relativeAddress;
			}

			TRACE_CONTROL("    final breakpoint address: %" B_PRIx64 "\n",
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
	TRACE_CONTROL("TeamDebugger::_HandleClearUserBreakpoint(%#" B_PRIx64 ")\n",
		address);

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
TeamDebugger::_HandleSetWatchpoint(target_addr_t address, uint32 type,
	int32 length, bool enabled)
{
	Watchpoint* watchpoint = new(std::nothrow) Watchpoint(address, type,
		length);

	if (watchpoint == NULL)
		return;
	BReference<Watchpoint> watchpointRef(watchpoint, true);

	_HandleSetWatchpoint(watchpoint, enabled);
}


void
TeamDebugger::_HandleSetWatchpoint(Watchpoint* watchpoint, bool enabled)
{
	status_t error = fWatchpointManager->InstallWatchpoint(watchpoint,
		enabled);
	if (error != B_OK) {
		_NotifyUser("Install Watchpoint", "Failed to install watchpoint: %s",
			strerror(error));
	}
}


void
TeamDebugger::_HandleClearWatchpoint(target_addr_t address)
{
	TRACE_CONTROL("TeamDebugger::_HandleClearWatchpoint(%#" B_PRIx64 ")\n",
		address);

	AutoLocker< ::Team> locker(fTeam);

	Watchpoint* watchpoint = fTeam->WatchpointAtAddress(address);
	if (watchpoint == NULL)
		return;
	BReference<Watchpoint> watchpointReference(watchpoint);

	locker.Unlock();

	_HandleClearWatchpoint(watchpoint);
}


void
TeamDebugger::_HandleClearWatchpoint(Watchpoint* watchpoint)
{
	fWatchpointManager->UninstallWatchpoint(watchpoint);
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

	if (!memoryBlock->IsValid()) {
		AutoLocker< ::Team> teamLocker(fTeam);

		if (!memoryBlock->HasListener(listener))
			memoryBlock->AddListener(listener);

		TeamMemory* memory = fTeam->GetTeamMemory();
		// schedule the job
		status_t result;
		if ((result = fWorker->ScheduleJob(
			new(std::nothrow) RetrieveMemoryBlockJob(fTeam, memory,
				memoryBlock),
			this)) != B_OK) {

			memoryBlock->NotifyDataRetrieved(result);
			memoryBlock->ReleaseReference();

			_NotifyUser("Inspect Address", "Failed to retrieve memory data: %s",
				strerror(result));
		}
	} else
		memoryBlock->NotifyDataRetrieved();

}


void
TeamDebugger::_HandleWriteMemory(target_addr_t address, void* data,
	target_size_t size)
{
	TRACE_CONTROL("TeamDebugger::_HandleWriteTargetMemory(%" B_PRIx64 ", %p, "
		"%" B_PRIu64 ")\n", address, data, size);

	AutoLocker< ::Team> teamLocker(fTeam);
	TeamMemory* memory = fTeam->GetTeamMemory();
	// schedule the job
	status_t result;
	if ((result = fWorker->ScheduleJob(
		new(std::nothrow) WriteMemoryJob(fTeam, memory, address, data, size),
		this)) != B_OK) {
		_NotifyUser("Write Memory", "Failed to write memory data: %s",
			strerror(result));
	}
}


void
TeamDebugger::_HandleEvaluateExpression(SourceLanguage* language,
	ExpressionInfo* info, StackFrame* frame, ::Thread* thread)
{
	status_t result = fWorker->ScheduleJob(
		new(std::nothrow) ExpressionEvaluationJob(fTeam, fDebuggerInterface,
			language, info, frame, thread));
	if (result != B_OK) {
		_NotifyUser("Evaluate Expression", "Failed to evaluate expression: %s",
			strerror(result));
	}
}


void
TeamDebugger::_HandleWriteCoreFile(const entry_ref& targetPath)
{
	status_t result = fWorker->ScheduleJob(
		new(std::nothrow) WriteCoreFileJob(fTeam, fDebuggerInterface,
			targetPath));
	if (result != B_OK) {
		_NotifyUser("Write Core File", "Failed to write core file: %s",
			strerror(result));
	}
}


status_t
TeamDebugger::_HandleSetArguments(int argc, const char* const* argv)
{
	fCommandLineArgc = argc;
	fCommandLineArgv = new(std::nothrow) const char*[argc];
	if (fCommandLineArgv == NULL)
		return B_NO_MEMORY;

	memset(const_cast<char **>(fCommandLineArgv), 0, sizeof(char*) * argc);

	for (int i = 0; i < argc; i++) {
		fCommandLineArgv[i] = strdup(argv[i]);
		if (fCommandLineArgv[i] == NULL)
			return B_NO_MEMORY;
	}

	return B_OK;
}


void
TeamDebugger::_HandleDebugInfoJobUserInput(ImageDebugInfoLoadingState* state)
{
	SpecificImageDebugInfoLoadingState* specificState
		= state->GetSpecificDebugInfoLoadingState();

	ImageDebugLoadingStateHandler* handler;
	if (ImageDebugLoadingStateHandlerRoster::Default()
			->FindStateHandler(specificState, handler) != B_OK) {
		TRACE_JOBS("TeamDebugger::_HandleDebugInfoJobUserInput(): "
			"Failed to find appropriate information handler, aborting.");
		return;
	}

	handler->HandleState(specificState, fUserInterface);
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

		breakpoint->SetHidden(breakpointSetting->IsHidden());
		breakpoint->SetCondition(breakpointSetting->Condition());

		// install it
		fBreakpointManager->InstallUserBreakpoint(breakpoint,
			breakpointSetting->IsEnabled());
	}

	fFileManager->LoadLocationMappings(fTeamSettings.FileManagerSettings());

	const TeamUiSettings* uiSettings = fTeamSettings.UiSettingFor(
		fUserInterface->ID());
	if (uiSettings != NULL)
			fUserInterface->LoadSettings(uiSettings);

	const TeamSignalSettings* signalSettings = fTeamSettings.SignalSettings();
	if (signalSettings != NULL) {
		fTeam->SetDefaultSignalDisposition(
			signalSettings->DefaultSignalDisposition());

		int32 signal;
		int32 disposition;
		for (int32 i = 0; i < signalSettings->CountCustomSignalDispositions();
			i++) {
			if (signalSettings->GetCustomSignalDispositionAt(i, signal,
				disposition) == B_OK) {
				fTeam->SetCustomSignalDisposition(signal, disposition);
			}
		}
	}
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

	fFileManager->SaveLocationMappings(settings.FileManagerSettings());
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


void
TeamDebugger::_ResetUserBackgroundStatusIfNeeded()
{
	if (!fWorker->HasPendingJobs())
		fUserInterface->NotifyBackgroundWorkStatus("Ready.");
}


// #pragma mark - Listener


TeamDebugger::Listener::~Listener()
{
}
