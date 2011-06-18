/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2010, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "TeamWindow.h"

#include <stdio.h>

#include <Button.h>
#include <FilePanel.h>
#include <LayoutBuilder.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Message.h>
#include <MessageFilter.h>
#include <Path.h>
#include <StringView.h>
#include <TabView.h>
#include <ScrollView.h>
#include <SplitView.h>
#include <TextView.h>

#include <AutoLocker.h>

#include "Breakpoint.h"
#include "CpuState.h"
#include "DisassembledCode.h"
#include "FileSourceCode.h"
#include "Image.h"
#include "ImageDebugInfo.h"
#include "InspectorWindow.h"
#include "LocatableFile.h"
#include "MessageCodes.h"
#include "RegistersView.h"
#include "StackTrace.h"
#include "StackTraceView.h"
#include "Tracing.h"
#include "TypeComponentPath.h"
#include "UserInterface.h"
#include "Variable.h"


enum {
	MAIN_TAB_INDEX_THREADS	= 0,
	MAIN_TAB_INDEX_IMAGES	= 1
};


enum {
	MSG_LOCATE_SOURCE_IF_NEEDED = 'lsin'
};


class PathViewMessageFilter : public BMessageFilter {
public:
		PathViewMessageFilter(BMessenger teamWindow)
			:
			BMessageFilter(B_MOUSE_UP),
			fTeamWindowMessenger(teamWindow)
		{
		}

		virtual filter_result Filter(BMessage*, BHandler**)
		{
			fTeamWindowMessenger.SendMessage(MSG_LOCATE_SOURCE_IF_NEEDED);

			return B_DISPATCH_MESSAGE;
		}

private:
		BMessenger fTeamWindowMessenger;
};


// #pragma mark - TeamWindow


TeamWindow::TeamWindow(::Team* team, UserInterfaceListener* listener)
	:
	BWindow(BRect(100, 100, 899, 699), "Team", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
	fTeam(team),
	fActiveThread(NULL),
	fActiveImage(NULL),
	fActiveStackTrace(NULL),
	fActiveStackFrame(NULL),
	fActiveBreakpoint(NULL),
	fActiveFunction(NULL),
	fActiveSourceCode(NULL),
	fActiveSourceObject(ACTIVE_SOURCE_NONE),
	fListener(listener),
	fTabView(NULL),
	fLocalsTabView(NULL),
	fThreadListView(NULL),
	fImageListView(NULL),
	fImageFunctionsView(NULL),
	fBreakpointsView(NULL),
	fVariablesView(NULL),
	fRegistersView(NULL),
	fStackTraceView(NULL),
	fSourceView(NULL),
	fRunButton(NULL),
	fStepOverButton(NULL),
	fStepIntoButton(NULL),
	fStepOutButton(NULL)
{
	fTeam->Lock();
	BString name = fTeam->Name();
	fTeam->Unlock();
	if (fTeam->ID() >= 0)
		name << " (" << fTeam->ID() << ")";
	SetTitle(name.String());

	fTeam->AddListener(this);
}


TeamWindow::~TeamWindow()
{
	if (fThreadListView != NULL)
		fThreadListView->UnsetListener();
	if (fStackTraceView != NULL)
		fStackTraceView->UnsetListener();
	if (fSourceView != NULL)
		fSourceView->UnsetListener();

	fTeam->RemoveListener(this);

	_SetActiveSourceCode(NULL);
	_SetActiveFunction(NULL);
	_SetActiveBreakpoint(NULL);
	_SetActiveStackFrame(NULL);
	_SetActiveStackTrace(NULL);
	_SetActiveImage(NULL);
	_SetActiveThread(NULL);
}


/*static*/ TeamWindow*
TeamWindow::Create(::Team* team, UserInterfaceListener* listener)
{
	TeamWindow* self = new TeamWindow(team, listener);

	try {
		self->_Init();
	} catch (...) {
		delete self;
		throw;
	}

	return self;
}


void
TeamWindow::DispatchMessage(BMessage* message, BHandler* handler)
{
	// Handle function key shortcuts for stepping
	switch (message->what) {
		case B_KEY_DOWN:
			if (fActiveThread != NULL) {
				int32 key;
				uint32 modifiers;
				if (message->FindInt32("key", &key) == B_OK
					&& message->FindInt32("modifiers", (int32*)&modifiers)
					== B_OK) {
					switch (key) {
						case B_F5_KEY:
							fListener->ThreadActionRequested(
								fActiveThread->ID(), MSG_THREAD_RUN);
							break;
						case B_F10_KEY:
							fListener->ThreadActionRequested(
								fActiveThread->ID(), MSG_THREAD_STEP_OVER);
							break;
						case B_F11_KEY:
							if ((modifiers & B_SHIFT_KEY) != 0) {
								fListener->ThreadActionRequested(
									fActiveThread->ID(), MSG_THREAD_STEP_OUT);
							} else {
								fListener->ThreadActionRequested(
									fActiveThread->ID(), MSG_THREAD_STEP_INTO);
							}
							break;
						default:
							break;
					}
				}
			}
			break;

		case B_COPY:
		case B_SELECT_ALL:
			BView* focusView = CurrentFocus();
			if (focusView != NULL)
				focusView->MessageReceived(message);
			break;
	}
	BWindow::DispatchMessage(message, handler);
}


void
TeamWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_SHOW_INSPECTOR_WINDOW:
		{
			if (fInspectorWindow) {
				fInspectorWindow->Activate(true);
				break;
			}

			try {
				fInspectorWindow = InspectorWindow::Create(fTeam, fListener,
					this);
				if (fInspectorWindow != NULL)
					fInspectorWindow->Show();
           	} catch (...) {
           		// TODO: notify user
           	}
           	break;
		}
		case MSG_INSPECTOR_WINDOW_CLOSED:
		{
			fInspectorWindow = NULL;
		}
		case B_REFS_RECEIVED:
		{
			entry_ref locatedPath;
			message->FindRef("refs", &locatedPath);
			_HandleResolveMissingSourceFile(locatedPath);
			break;
		}
		case MSG_LOCATE_SOURCE_IF_NEEDED:
		{
			if (fActiveFunction != NULL
				&& fActiveFunction->GetFunctionDebugInfo()
					->SourceFile() != NULL && fActiveSourceCode != NULL
				&& fActiveSourceCode->GetSourceFile() == NULL) {
				BFilePanel* panel = NULL;
				try {
					panel = new BFilePanel(B_OPEN_PANEL,
						new BMessenger(this));
					panel->Show();
				} catch (...) {
					delete panel;
				}
			}
			break;
		}
		case MSG_THREAD_RUN:
		case MSG_THREAD_STOP:
		case MSG_THREAD_STEP_OVER:
		case MSG_THREAD_STEP_INTO:
		case MSG_THREAD_STEP_OUT:
			if (fActiveThread != NULL) {
				fListener->ThreadActionRequested(fActiveThread->ID(),
					message->what);
			}
			break;

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

		case MSG_IMAGE_DEBUG_INFO_CHANGED:
		{
			int32 imageID;
			if (message->FindInt32("image", &imageID) != B_OK)
				break;

			_HandleImageDebugInfoChanged(imageID);
			break;
		}

		case MSG_USER_BREAKPOINT_CHANGED:
		{
			UserBreakpoint* breakpoint;
			if (message->FindPointer("breakpoint", (void**)&breakpoint) != B_OK)
				break;
			BReference<UserBreakpoint> breakpointReference(breakpoint, true);

			_HandleUserBreakpointChanged(breakpoint);
			break;
		}

		case MSG_FUNCTION_SOURCE_CODE_CHANGED:
		{
			_HandleSourceCodeChanged();
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
TeamWindow::QuitRequested()
{
	return fListener->UserInterfaceQuitRequested();
}


void
TeamWindow::ThreadSelectionChanged(::Thread* thread)
{
	_SetActiveThread(thread);
}


void
TeamWindow::ImageSelectionChanged(Image* image)
{
	_SetActiveImage(image);
}


void
TeamWindow::StackFrameSelectionChanged(StackFrame* frame)
{
	_SetActiveStackFrame(frame);
}


void
TeamWindow::FunctionSelectionChanged(FunctionInstance* function)
{
	// If the function wasn't already active, it was just selected by the user.
	if (function != NULL && function != fActiveFunction)
		fActiveSourceObject = ACTIVE_SOURCE_FUNCTION;

	_SetActiveFunction(function);
}


void
TeamWindow::BreakpointSelectionChanged(UserBreakpoint* breakpoint)
{
	_SetActiveBreakpoint(breakpoint);
}


void
TeamWindow::SetBreakpointEnabledRequested(UserBreakpoint* breakpoint,
	bool enabled)
{
	fListener->SetBreakpointEnabledRequested(breakpoint, enabled);
}


void
TeamWindow::ClearBreakpointRequested(UserBreakpoint* breakpoint)
{
	fListener->ClearBreakpointRequested(breakpoint);
}


void
TeamWindow::SetBreakpointRequested(target_addr_t address, bool enabled)
{
	fListener->SetBreakpointRequested(address, enabled);
}


void
TeamWindow::ClearBreakpointRequested(target_addr_t address)
{
	fListener->ClearBreakpointRequested(address);
}


void
TeamWindow::ValueNodeValueRequested(CpuState* cpuState,
	ValueNodeContainer* container, ValueNode* valueNode)
{
	fListener->ValueNodeValueRequested(cpuState, container, valueNode);
}


void
TeamWindow::ThreadStateChanged(const Team::ThreadEvent& event)
{
	BMessage message(MSG_THREAD_STATE_CHANGED);
	message.AddInt32("thread", event.GetThread()->ID());
	PostMessage(&message);
}


void
TeamWindow::ThreadCpuStateChanged(const Team::ThreadEvent& event)
{
	BMessage message(MSG_THREAD_CPU_STATE_CHANGED);
	message.AddInt32("thread", event.GetThread()->ID());
	PostMessage(&message);
}


void
TeamWindow::ThreadStackTraceChanged(const Team::ThreadEvent& event)
{
	BMessage message(MSG_THREAD_STACK_TRACE_CHANGED);
	message.AddInt32("thread", event.GetThread()->ID());
	PostMessage(&message);
}


void
TeamWindow::ImageDebugInfoChanged(const Team::ImageEvent& event)
{
	BMessage message(MSG_IMAGE_DEBUG_INFO_CHANGED);
	message.AddInt32("image", event.GetImage()->ID());
	PostMessage(&message);
}


void
TeamWindow::UserBreakpointChanged(const Team::UserBreakpointEvent& event)
{
	BMessage message(MSG_USER_BREAKPOINT_CHANGED);
	BReference<UserBreakpoint> breakpointReference(event.GetBreakpoint());
	if (message.AddPointer("breakpoint", event.GetBreakpoint()) == B_OK
		&& PostMessage(&message) == B_OK) {
		breakpointReference.Detach();
	}
}


void
TeamWindow::FunctionSourceCodeChanged(Function* function)
{
	TRACE_GUI("TeamWindow::FunctionSourceCodeChanged(%p): source: %p, "
		"state: %d\n", function, function->GetSourceCode(),
		function->SourceCodeState());

	PostMessage(MSG_FUNCTION_SOURCE_CODE_CHANGED);
}


void
TeamWindow::_Init()
{
	BScrollView* sourceScrollView;

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(fMenuBar = new BMenuBar("Menu"))
		.AddSplit(B_VERTICAL, 3.0f)
			.SetInsets(4.0f, 4.0f, 4.0f, 4.0f)
			.Add(fTabView = new BTabView("tab view"), 0.4f)
			.AddGroup(B_VERTICAL, 4.0f)
				.AddGroup(B_HORIZONTAL, 4.0f)
					.Add(fRunButton = new BButton("Run"))
					.Add(fStepOverButton = new BButton("Step Over"))
					.Add(fStepIntoButton = new BButton("Step Into"))
					.Add(fStepOutButton = new BButton("Step Out"))
					.AddGlue()
				.End()
				.Add(fSourcePathView = new BStringView(
					"source path",
					"Source path unavailable."), 4.0f)
				.AddSplit(B_HORIZONTAL, 3.0f)
					.Add(sourceScrollView = new BScrollView("source scroll",
						NULL, 0, true, true), 3.0f)
					.Add(fLocalsTabView = new BTabView("locals view"))
				.End()
			.End()
		.End();

	// add source view
	sourceScrollView->SetTarget(fSourceView = SourceView::Create(fTeam, this));

	// add threads tab
	BSplitView* threadGroup = new BSplitView(B_HORIZONTAL);
	threadGroup->SetName("Threads");
	fTabView->AddTab(threadGroup);
	BLayoutBuilder::Split<>(threadGroup)
		.Add(fThreadListView = ThreadListView::Create(fTeam, this))
		.Add(fStackTraceView = StackTraceView::Create(this));

	// add images tab
	BSplitView* imagesGroup = new BSplitView(B_HORIZONTAL);
	imagesGroup->SetName("Images");
	fTabView->AddTab(imagesGroup);
	BLayoutBuilder::Split<>(imagesGroup)
		.Add(fImageListView = ImageListView::Create(fTeam, this))
		.Add(fImageFunctionsView = ImageFunctionsView::Create(this));

	// add breakpoints tab
	BGroupView* breakpointsGroup = new BGroupView(B_HORIZONTAL, 4.0f);
	breakpointsGroup->SetName("Breakpoints");
	fTabView->AddTab(breakpointsGroup);
	BLayoutBuilder::Group<>(breakpointsGroup)
		.SetInsets(4.0f, 4.0f, 4.0f, 4.0f)
		.Add(fBreakpointsView = BreakpointsView::Create(fTeam, this));

	// add local variables tab
	BView* tab = fVariablesView = VariablesView::Create(this);
	fLocalsTabView->AddTab(tab);

	// add registers tab
	tab = fRegistersView = RegistersView::Create(fTeam->GetArchitecture());
	fLocalsTabView->AddTab(tab);

	fRunButton->SetMessage(new BMessage(MSG_THREAD_RUN));
	fStepOverButton->SetMessage(new BMessage(MSG_THREAD_STEP_OVER));
	fStepIntoButton->SetMessage(new BMessage(MSG_THREAD_STEP_INTO));
	fStepOutButton->SetMessage(new BMessage(MSG_THREAD_STEP_OUT));
	fRunButton->SetTarget(this);
	fStepOverButton->SetTarget(this);
	fStepIntoButton->SetTarget(this);
	fStepOutButton->SetTarget(this);

	fSourcePathView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	BMessageFilter* filter = new(std::nothrow) PathViewMessageFilter(
		BMessenger(this));
	if (filter != NULL)
		fSourcePathView->AddFilter(filter);

	// add menus and menu items
	BMenu* menu = new BMenu("File");
	fMenuBar->AddItem(menu);
	BMenuItem* item = new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED),
		'Q');
	menu->AddItem(item);
	item->SetTarget(this);
	menu = new BMenu("Edit");
	fMenuBar->AddItem(menu);
	item = new BMenuItem("Copy", new BMessage(B_COPY), 'C');
	menu->AddItem(item);
	item->SetTarget(this);
	item = new BMenuItem("Select All", new BMessage(B_SELECT_ALL), 'A');
	menu->AddItem(item);
	item->SetTarget(this);
	menu = new BMenu("Tools");
	fMenuBar->AddItem(menu);
	item = new BMenuItem("Inspect Memory",
		new BMessage(MSG_SHOW_INSPECTOR_WINDOW), 'I');
	menu->AddItem(item);
	item->SetTarget(this);

	AutoLocker< ::Team> locker(fTeam);
	_UpdateRunButtons();
}


void
TeamWindow::_SetActiveThread(::Thread* thread)
{
	if (thread == fActiveThread)
		return;

	if (fActiveThread != NULL)
		fActiveThread->ReleaseReference();

	fActiveThread = thread;

	if (fActiveThread != NULL)
		fActiveThread->AcquireReference();

	AutoLocker< ::Team> locker(fTeam);
	_UpdateRunButtons();

	StackTrace* stackTrace = fActiveThread != NULL
		? fActiveThread->GetStackTrace() : NULL;
	BReference<StackTrace> stackTraceReference(stackTrace);
		// hold a reference until we've set it

	locker.Unlock();

	fThreadListView->SetThread(fActiveThread);

	_SetActiveStackTrace(stackTrace);
	_UpdateCpuState();
}


void
TeamWindow::_SetActiveImage(Image* image)
{
	if (image == fActiveImage)
		return;

	if (fActiveImage != NULL)
		fActiveImage->ReleaseReference();

	fActiveImage = image;

	AutoLocker< ::Team> locker(fTeam);

	ImageDebugInfo* imageDebugInfo = NULL;
	BReference<ImageDebugInfo> imageDebugInfoReference;

	if (fActiveImage != NULL) {
		fActiveImage->AcquireReference();

		imageDebugInfo = fActiveImage->GetImageDebugInfo();
		imageDebugInfoReference.SetTo(imageDebugInfo);

		// If the debug info is not loaded yet, request it.
		if (fActiveImage->ImageDebugInfoState() == IMAGE_DEBUG_INFO_NOT_LOADED)
			fListener->ImageDebugInfoRequested(fActiveImage);
	}

	locker.Unlock();

	fImageListView->SetImage(fActiveImage);
	fImageFunctionsView->SetImageDebugInfo(imageDebugInfo);
}


void
TeamWindow::_SetActiveStackTrace(StackTrace* stackTrace)
{
	if (stackTrace == fActiveStackTrace)
		return;

	if (fActiveStackTrace != NULL)
		fActiveStackTrace->ReleaseReference();

	fActiveStackTrace = stackTrace;

	if (fActiveStackTrace != NULL)
		fActiveStackTrace->AcquireReference();

	fStackTraceView->SetStackTrace(fActiveStackTrace);
	fSourceView->SetStackTrace(fActiveStackTrace);

	if (fActiveStackTrace != NULL)
		_SetActiveStackFrame(fActiveStackTrace->FrameAt(0));
}


void
TeamWindow::_SetActiveStackFrame(StackFrame* frame)
{
	if (frame == fActiveStackFrame)
		return;

	if (fActiveStackFrame != NULL) {
		AutoLocker< ::Team> locker(fTeam);
		fActiveStackFrame->RemoveListener(this);
		locker.Unlock();

		fActiveStackFrame->ReleaseReference();
	}

	fActiveStackFrame = frame;

	if (fActiveStackFrame != NULL) {
		fActiveStackFrame->AcquireReference();

		AutoLocker< ::Team> locker(fTeam);
		fActiveStackFrame->AddListener(this);
		locker.Unlock();

		fActiveSourceObject = ACTIVE_SOURCE_STACK_FRAME;

		_SetActiveFunction(fActiveStackFrame->Function());
	}

	_UpdateCpuState();

	fStackTraceView->SetStackFrame(fActiveStackFrame);
	if (fActiveStackFrame != NULL)
		fVariablesView->SetStackFrame(fActiveThread, fActiveStackFrame);
	else
		fVariablesView->SetStackFrame(NULL, NULL);
	fSourceView->SetStackFrame(fActiveStackFrame);
}


void
TeamWindow::_SetActiveBreakpoint(UserBreakpoint* breakpoint)
{
	if (breakpoint == fActiveBreakpoint)
		return;

	if (fActiveBreakpoint != NULL)
		fActiveBreakpoint->ReleaseReference();

	fActiveBreakpoint = breakpoint;

	if (fActiveBreakpoint != NULL) {
		fActiveBreakpoint->AcquireReference();

		// get the breakpoint's function (more exactly: some function instance)
		AutoLocker< ::Team> locker(fTeam);

		Function* function = fTeam->FunctionByID(
			breakpoint->Location().GetFunctionID());
		FunctionInstance* functionInstance = function != NULL
			? function->FirstInstance() : NULL;
		BReference<FunctionInstance> functionInstanceReference(
			functionInstance);

		locker.Unlock();

		fActiveSourceObject = ACTIVE_SOURCE_BREAKPOINT;

		_SetActiveFunction(functionInstance);

		// scroll to the breakpoint's source code line number (it is not done
		// automatically, if the active function remains the same)
		_ScrollToActiveFunction();
	}

	fBreakpointsView->SetBreakpoint(fActiveBreakpoint);
}


void
TeamWindow::_SetActiveFunction(FunctionInstance* functionInstance)
{
// TODO: If a function is selected by other means than via selecting a stack
// frame, we should still select a matching stack frame, if it features the
// same function.
	if (functionInstance == fActiveFunction)
		return;

	AutoLocker< ::Team> locker(fTeam);

	if (fActiveFunction != NULL) {
		fActiveFunction->GetFunction()->RemoveListener(this);
		fActiveFunction->ReleaseReference();
	}

	// to avoid listener feedback problems, first unset the active function and
	// set the new image, if any
	locker.Unlock();

	fActiveFunction = NULL;

	if (functionInstance != NULL)
		_SetActiveImage(fTeam->ImageByAddress(functionInstance->Address()));

	fActiveFunction = functionInstance;

	locker.Lock();

	SourceCode* sourceCode = NULL;
	BReference<SourceCode> sourceCodeReference;

	if (fActiveFunction != NULL) {
		fActiveFunction->AcquireReference();
		fActiveFunction->GetFunction()->AddListener(this);

		Function* function = fActiveFunction->GetFunction();
		sourceCode = function->GetSourceCode();
		if (sourceCode == NULL)
			sourceCode = fActiveFunction->GetSourceCode();
		sourceCodeReference.SetTo(sourceCode);

		// If the source code is not loaded yet, request it.
		if (function->SourceCodeState() == FUNCTION_SOURCE_NOT_LOADED)
			fListener->FunctionSourceCodeRequested(fActiveFunction);
	}

	locker.Unlock();

	_SetActiveSourceCode(sourceCode);

	fImageFunctionsView->SetFunction(fActiveFunction);
}


void
TeamWindow::_SetActiveSourceCode(SourceCode* sourceCode)
{
	if (sourceCode == fActiveSourceCode) {
		_ScrollToActiveFunction();
		return;
	}

	if (fActiveSourceCode != NULL)
		fActiveSourceCode->ReleaseReference();

	fActiveSourceCode = sourceCode;

	if (fActiveSourceCode != NULL)
		fActiveSourceCode->AcquireReference();

	fSourceView->SetSourceCode(fActiveSourceCode);

	_ScrollToActiveFunction();
}

void
TeamWindow::_UpdateCpuState()
{
	// get the CPU state
	CpuState* cpuState = NULL;
	BReference<CpuState> cpuStateReference;
		// hold a reference until the register view has one

	if (fActiveThread != NULL) {
		// Get the CPU state from the active stack frame or the thread directly.
		if (fActiveStackFrame == NULL) {
			AutoLocker< ::Team> locker(fTeam);
			cpuState = fActiveThread->GetCpuState();
			cpuStateReference.SetTo(cpuState);
			locker.Unlock();
		} else
			cpuState = fActiveStackFrame->GetCpuState();
	}

	fRegistersView->SetCpuState(cpuState);
}


void
TeamWindow::_UpdateRunButtons()
{
	uint32 threadState = fActiveThread != NULL
		? fActiveThread->State() : THREAD_STATE_UNKNOWN;

	switch (threadState) {
		case THREAD_STATE_UNKNOWN:
			fRunButton->SetEnabled(false);
			fStepOverButton->SetEnabled(false);
			fStepIntoButton->SetEnabled(false);
			fStepOutButton->SetEnabled(false);
			break;
		case THREAD_STATE_RUNNING:
			fRunButton->SetLabel("Debug");
			fRunButton->SetMessage(new BMessage(MSG_THREAD_STOP));
			fRunButton->SetEnabled(true);
			fStepOverButton->SetEnabled(false);
			fStepIntoButton->SetEnabled(false);
			fStepOutButton->SetEnabled(false);
			break;
		case THREAD_STATE_STOPPED:
			fRunButton->SetLabel("Run");
			fRunButton->SetMessage(new BMessage(MSG_THREAD_RUN));
			fRunButton->SetEnabled(true);
			fStepOverButton->SetEnabled(true);
			fStepIntoButton->SetEnabled(true);
			fStepOutButton->SetEnabled(true);
			break;
	}
}


void
TeamWindow::_ScrollToActiveFunction()
{
	// Scroll to the active function, if it has been selected manually.
	if (fActiveFunction == NULL || fActiveSourceCode == NULL)
		return;

	switch (fActiveSourceObject) {
		case ACTIVE_SOURCE_FUNCTION:
			fSourceView->ScrollToAddress(fActiveFunction->Address());
			break;
		case ACTIVE_SOURCE_BREAKPOINT:
		{
			if (fActiveBreakpoint == NULL)
				break;

			const UserBreakpointLocation& location
				= fActiveBreakpoint->Location();
			int32 line = location.GetSourceLocation().Line();

			if (location.SourceFile() != NULL && line >= 0
				&& fActiveSourceCode->GetSourceFile()
					== location.SourceFile()) {
				fSourceView->ScrollToLine(line);
			} else {
				fSourceView->ScrollToAddress(
					fActiveFunction->Address()
						+ location.RelativeAddress());
			}
			break;
		}
		case ACTIVE_SOURCE_NONE:
		case ACTIVE_SOURCE_STACK_FRAME:
			break;
	}
}


void
TeamWindow::_HandleThreadStateChanged(thread_id threadID)
{
	AutoLocker< ::Team> locker(fTeam);

	::Thread* thread = fTeam->ThreadByID(threadID);
	if (thread == NULL)
		return;

	// If the thread has been stopped and we don't have an active thread yet
	// (or it isn't stopped), switch to this thread. Otherwise ignore the event.
	if (thread->State() == THREAD_STATE_STOPPED
		&& (fActiveThread == NULL
			|| (thread != fActiveThread
				&& fActiveThread->State() != THREAD_STATE_STOPPED))) {
		_SetActiveThread(thread);
	} else if (thread != fActiveThread) {
		// otherwise ignore the event, if the thread is not the active one
		return;
	}

	// Switch to the threads tab view when the thread has stopped.
	if (thread->State() == THREAD_STATE_STOPPED)
		fTabView->Select(MAIN_TAB_INDEX_THREADS);

	_UpdateRunButtons();
}


void
TeamWindow::_HandleCpuStateChanged(thread_id threadID)
{
	// We're only interested in the currently selected thread
	if (fActiveThread == NULL || threadID != fActiveThread->ID())
		return;

	_UpdateCpuState();
}


void
TeamWindow::_HandleStackTraceChanged(thread_id threadID)
{
	// We're only interested in the currently selected thread
	if (fActiveThread == NULL || threadID != fActiveThread->ID())
		return;

	AutoLocker< ::Team> locker(fTeam);

	StackTrace* stackTrace = fActiveThread != NULL
		? fActiveThread->GetStackTrace() : NULL;
	BReference<StackTrace> stackTraceReference(stackTrace);
		// hold a reference until the register view has one

	locker.Unlock();

	_SetActiveStackTrace(stackTrace);
}


void
TeamWindow::_HandleImageDebugInfoChanged(image_id imageID)
{
	TRACE_GUI("TeamWindow::_HandleImageDebugInfoChanged(%ld)\n", imageID);

	// We're only interested in the currently selected thread
	if (fActiveImage == NULL || imageID != fActiveImage->ID())
		return;

	AutoLocker< ::Team> locker(fTeam);

	ImageDebugInfo* imageDebugInfo = fActiveImage != NULL
		? fActiveImage->GetImageDebugInfo() : NULL;

	TRACE_GUI("  image debug info: %p\n", imageDebugInfo);

	BReference<ImageDebugInfo> imageDebugInfoReference(imageDebugInfo);
		// hold a reference until we've set it

	locker.Unlock();

	fImageFunctionsView->SetImageDebugInfo(imageDebugInfo);
}


void
TeamWindow::_HandleSourceCodeChanged()
{
	// If we don't have an active function anymore, the message is obsolete.
	if (fActiveFunction == NULL)
		return;

	// get a reference to the source code
	AutoLocker< ::Team> locker(fTeam);

	SourceCode* sourceCode = fActiveFunction->GetFunction()->GetSourceCode();
	if (sourceCode == NULL) {
		sourceCode = fActiveFunction->GetSourceCode();

		BString sourceText;
		LocatableFile* sourceFile = fActiveFunction->GetFunctionDebugInfo()
			->SourceFile();
		if (sourceFile != NULL && !sourceFile->GetLocatedPath(sourceText))
			sourceFile->GetPath(sourceText);

		if (sourceCode != NULL && sourceCode->GetSourceFile() == NULL
			&& sourceFile != NULL) {
			sourceText.Prepend("Click to locate source file '");
			sourceText += "'";
			fSourcePathView->SetText(sourceText.String());
		} else if (sourceFile != NULL) {
			sourceText.Prepend("File: ");
			fSourcePathView->SetText(sourceText.String());
		} else
			fSourcePathView->SetText("Source file unavailable.");
	}

	BReference<SourceCode> sourceCodeReference(sourceCode);

	locker.Unlock();

	_SetActiveSourceCode(sourceCode);
}


void
TeamWindow::_HandleUserBreakpointChanged(UserBreakpoint* breakpoint)
{
	fSourceView->UserBreakpointChanged(breakpoint);
	fBreakpointsView->UserBreakpointChanged(breakpoint);
}


void
TeamWindow::_HandleResolveMissingSourceFile(entry_ref& locatedPath)
{
	if (fActiveFunction != NULL) {
		LocatableFile* sourceFile = fActiveFunction->GetFunctionDebugInfo()
			->SourceFile();
		if (sourceFile != NULL) {
			BString sourcePath;
			BString targetPath;
			sourceFile->GetPath(sourcePath);
			BPath path(&locatedPath);
			targetPath = path.Path();
			fListener->SourceEntryLocateRequested(sourcePath, targetPath);
			fListener->FunctionSourceCodeRequested(fActiveFunction);
		}
	}
}
