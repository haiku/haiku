/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "TeamWindow.h"

#include <stdio.h>

#include <Button.h>
#include <LayoutBuilder.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Message.h>
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
#include "MessageCodes.h"
#include "RegistersView.h"
#include "StackTrace.h"
#include "StackTraceView.h"
#include "TypeComponentPath.h"
#include "Variable.h"


enum {
	MAIN_TAB_INDEX_THREADS	= 0,
	MAIN_TAB_INDEX_IMAGES	= 1
};


// #pragma mark - TeamWindow


TeamWindow::TeamWindow(::Team* team, Listener* listener)
	:
	BWindow(BRect(100, 100, 899, 699), "Team", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
	fTeam(team),
	fActiveThread(NULL),
	fActiveImage(NULL),
	fActiveStackTrace(NULL),
	fActiveStackFrame(NULL),
	fActiveFunction(NULL),
	fActiveSourceCode(NULL),
	fListener(listener),
	fTabView(NULL),
	fLocalsTabView(NULL),
	fThreadListView(NULL),
	fImageListView(NULL),
	fImageFunctionsView(NULL),
	fVariablesView(NULL),
	fRegistersView(NULL),
	fStackTraceView(NULL),
	fSourceView(NULL),
	fRunButton(NULL),
	fStepOverButton(NULL),
	fStepIntoButton(NULL),
	fStepOutButton(NULL)
{
	BString name = fTeam->Name();
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
	_SetActiveStackFrame(NULL);
	_SetActiveStackTrace(NULL);
	_SetActiveImage(NULL);
	_SetActiveThread(NULL);
}


/*static*/ TeamWindow*
TeamWindow::Create(::Team* team, Listener* listener)
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

		case MSG_STACK_FRAME_VALUE_RETRIEVED:
		{
			void* _stackFrame;
			void* _variable;
			void* _path;
			if (message->FindPointer("stackFrame", &_stackFrame) == B_OK
				&& message->FindPointer("variable", &_variable) == B_OK
				&& message->FindPointer("path", &_path) == B_OK) {
				StackFrame* stackFrame = (StackFrame*)_stackFrame;
				Variable* variable = (Variable*)_variable;
				TypeComponentPath* path = (TypeComponentPath*)_path;
				_HandleStackFrameValueRetrieved(stackFrame, variable, path);
				path->ReleaseReference();
				variable->ReleaseReference();
				stackFrame->ReleaseReference();
			}
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
			uint64 address;
			if (message->FindUInt64("address", &address) != B_OK)
				break;

			_HandleUserBreakpointChanged(address);
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
	return fListener->TeamWindowQuitRequested();
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
	_SetActiveFunction(function);
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
TeamWindow::StackFrameValueRequested(::Thread* thread, StackFrame* stackFrame,
	Variable* variable, TypeComponentPath* path)
{
	fListener->StackFrameValueRequested(thread, stackFrame, variable, path);
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
TeamWindow::UserBreakpointChanged(const Team::BreakpointEvent& event)
{
	BMessage message(MSG_USER_BREAKPOINT_CHANGED);
	message.AddUInt64("address", event.GetBreakpoint()->Address());
	PostMessage(&message);
}


void
TeamWindow::FunctionSourceCodeChanged(Function* function)
{
printf("TeamWindow::FunctionSourceCodeChanged(%p): source: %p, state: %d\n",
function, function->GetSourceCode(), function->SourceCodeState());
	PostMessage(MSG_FUNCTION_SOURCE_CODE_CHANGED);
}


void
TeamWindow::StackFrameValueRetrieved(StackFrame* stackFrame, Variable* variable,
	TypeComponentPath* path)
{
	BMessage message(MSG_STACK_FRAME_VALUE_RETRIEVED);
	if (message.AddPointer("stackFrame", stackFrame) == B_OK
		&& message.AddPointer("variable", variable) == B_OK
		&& message.AddPointer("path", path) == B_OK
		&& PostMessage(&message) == B_OK) {
		stackFrame->AcquireReference();
		variable->AcquireReference();
		path->AcquireReference();
	}
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
	fRunButton->SetTarget(this);
	fStepOutButton->SetTarget(this);

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

	AutoLocker< ::Team> locker(fTeam);
	_UpdateRunButtons();
}


void
TeamWindow::_SetActiveThread(::Thread* thread)
{
	if (thread == fActiveThread)
		return;

	if (fActiveThread != NULL)
		fActiveThread->RemoveReference();

	fActiveThread = thread;

	if (fActiveThread != NULL)
		fActiveThread->AddReference();

	AutoLocker< ::Team> locker(fTeam);
	_UpdateRunButtons();

	StackTrace* stackTrace = fActiveThread != NULL
		? fActiveThread->GetStackTrace() : NULL;
	Reference<StackTrace> stackTraceReference(stackTrace);
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
		fActiveImage->RemoveReference();

	fActiveImage = image;

	AutoLocker< ::Team> locker(fTeam);

	ImageDebugInfo* imageDebugInfo = NULL;
	Reference<ImageDebugInfo> imageDebugInfoReference;

	if (fActiveImage != NULL) {
		fActiveImage->AddReference();

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
		fActiveStackTrace->RemoveReference();

	fActiveStackTrace = stackTrace;

	if (fActiveStackTrace != NULL)
		fActiveStackTrace->AddReference();

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

		fActiveStackFrame->RemoveReference();
	}

	fActiveStackFrame = frame;

	if (fActiveStackFrame != NULL) {
		fActiveStackFrame->AddReference();

		AutoLocker< ::Team> locker(fTeam);
		fActiveStackFrame->AddListener(this);
		locker.Unlock();

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
TeamWindow::_SetActiveFunction(FunctionInstance* functionInstance)
{
	if (functionInstance == fActiveFunction)
		return;

	AutoLocker< ::Team> locker(fTeam);

	if (fActiveFunction != NULL) {
		fActiveFunction->GetFunction()->RemoveListener(this);
		fActiveFunction->RemoveReference();
	}

	// to avoid listener feedback problems, first unset the active function and
	// set the new image, if any
	locker.Unlock();

	fActiveFunction = NULL;

	if (functionInstance != NULL) {
		_SetActiveImage(fTeam->ImageByAddress(functionInstance->Address()));
	}

	fActiveFunction = functionInstance;

	locker.Lock();

	SourceCode* sourceCode = NULL;
	Reference<SourceCode> sourceCodeReference;

	if (fActiveFunction != NULL) {
		fActiveFunction->AddReference();
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
		fActiveSourceCode->RemoveReference();

	fActiveSourceCode = sourceCode;

	if (fActiveSourceCode != NULL)
		fActiveSourceCode->AddReference();

	fSourceView->SetSourceCode(fActiveSourceCode);

	_ScrollToActiveFunction();
}

void
TeamWindow::_UpdateCpuState()
{
	// get the CPU state
	CpuState* cpuState = NULL;
	Reference<CpuState> cpuStateReference;
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
	// Scroll to the active function, if it doesn't match the stack frame (i.e.
	// has been selected manually).
	if (fActiveFunction != NULL && fActiveSourceCode != NULL
		&& (fActiveStackFrame == NULL
			|| fActiveStackFrame->Function() != fActiveFunction)) {
		fSourceView->ScrollToAddress(fActiveFunction->Address());
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
	Reference<StackTrace> stackTraceReference(stackTrace);
		// hold a reference until the register view has one

	locker.Unlock();

	_SetActiveStackTrace(stackTrace);
}


void
TeamWindow::_HandleStackFrameValueRetrieved(StackFrame* stackFrame,
	Variable* variable, TypeComponentPath* path)
{
	if (stackFrame != fActiveStackFrame)
		return;

	fVariablesView->StackFrameValueRetrieved(stackFrame, variable, path);
}


void
TeamWindow::_HandleImageDebugInfoChanged(image_id imageID)
{
printf("TeamWindow::_HandleImageDebugInfoChanged(%ld)\n", imageID);
	// We're only interested in the currently selected thread
	if (fActiveImage == NULL || imageID != fActiveImage->ID())
		return;

	AutoLocker< ::Team> locker(fTeam);

	ImageDebugInfo* imageDebugInfo = fActiveImage != NULL
		? fActiveImage->GetImageDebugInfo() : NULL;
printf("  image debug info: %p\n", imageDebugInfo);
	Reference<ImageDebugInfo> imageDebugInfoReference(imageDebugInfo);
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
	if (sourceCode == NULL)
		sourceCode = fActiveFunction->GetSourceCode();
	Reference<SourceCode> sourceCodeReference(sourceCode);

	locker.Unlock();

	_SetActiveSourceCode(sourceCode);
}


void
TeamWindow::_HandleUserBreakpointChanged(target_addr_t address)
{
	fSourceView->UserBreakpointChanged(address);
}


// #pragma mark - Listener


TeamWindow::Listener::~Listener()
{
}
