/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "TeamWindow.h"

#include <stdio.h>

#include <Button.h>
#include <LayoutBuilder.h>
#include <Message.h>
#include <TabView.h>
#include <ScrollView.h>
#include <SplitView.h>
#include <TextView.h>

#include <AutoLocker.h>

#include "CpuState.h"
#include "ImageListView.h"
#include "MessageCodes.h"
#include "RegisterView.h"
#include "SourceCode.h"
#include "StackTrace.h"
#include "StackTraceView.h"


// #pragma mark - TeamWindow


TeamWindow::TeamWindow(TeamDebugModel* debugModel, Listener* listener)
	:
	BWindow(BRect(100, 100, 899, 699), "Team", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS),
	fDebugModel(debugModel),
	fActiveThread(NULL),
	fActiveStackTrace(NULL),
	fActiveStackFrame(NULL),
	fActiveSourceCode(NULL),
	fListener(listener),
	fTabView(NULL),
	fLocalsTabView(NULL),
	fThreadListView(NULL),
	fImageListView(NULL),
	fRegisterView(NULL),
	fStackTraceView(NULL),
	fSourceView(NULL),
	fRunButton(NULL),
	fStepOverButton(NULL),
	fStepIntoButton(NULL),
	fStepOutButton(NULL)
{
	::Team* team = debugModel->GetTeam();
	BString name = team->Name();
	if (team->ID() >= 0)
		name << " (" << team->ID() << ")";
	SetTitle(name.String());

	fDebugModel->AddListener(this);
	team->AddListener(this);
}


TeamWindow::~TeamWindow()
{
	if (fThreadListView != NULL)
		fThreadListView->UnsetListener();
	if (fStackTraceView != NULL)
		fStackTraceView->UnsetListener();
	if (fSourceView != NULL)
		fSourceView->UnsetListener();

	fDebugModel->GetTeam()->RemoveListener(this);
	fDebugModel->RemoveListener(this);

	if (fActiveSourceCode != NULL)
		fActiveSourceCode->RemoveReference();
	if (fActiveStackFrame != NULL)
		fActiveStackFrame->RemoveReference();
	if (fActiveStackTrace != NULL)
		fActiveStackTrace->RemoveReference();
	if (fActiveThread != NULL)
		fActiveThread->RemoveReference();
}


/*static*/ TeamWindow*
TeamWindow::Create(TeamDebugModel* debugModel, Listener* listener)
{
	TeamWindow* self = new TeamWindow(debugModel, listener);

	try {
		self->_Init();
	} catch (...) {
		delete self;
		throw;
	}

	return self;
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
				fListener->ThreadActionRequested(this, fActiveThread->ID(),
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

		case MSG_USER_BREAKPOINT_CHANGED:
		{
			uint64 address;
			if (message->FindUInt64("address", &address) != B_OK)
				break;

			_HandleUserBreakpointChanged(address);
			break;
		}

		case MSG_STACK_FRAME_SOURCE_CODE_CHANGED:
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
	return fListener->TeamWindowQuitRequested(this);
}


void
TeamWindow::ThreadSelectionChanged(::Thread* thread)
{
	_SetActiveThread(thread);
}


void
TeamWindow::StackFrameSelectionChanged(StackFrame* frame)
{
	_SetActiveStackFrame(frame);
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
TeamWindow::UserBreakpointChanged(const TeamDebugModel::BreakpointEvent& event)
{
	BMessage message(MSG_USER_BREAKPOINT_CHANGED);
	message.AddUInt64("address", event.GetBreakpoint()->Address());
	PostMessage(&message);
}


void
TeamWindow::StackFrameSourceCodeChanged(StackFrame* frame)
{
printf("TeamWindow::StackFrameSourceCodeChanged(%p): source: %p, state: %d\n",
frame, frame->GetSourceCode(), frame->SourceCodeState());
	PostMessage(MSG_STACK_FRAME_SOURCE_CODE_CHANGED);
}


void
TeamWindow::_Init()
{
	BScrollView* sourceScrollView;

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.AddSplit(B_VERTICAL, 3.0f)
			.AddGroup(B_VERTICAL, 0.0f, 0.4f)
				.Add(fTabView = new BTabView("tab view"))
				.SetInsets(4.0f, 4.0f, 4.0f, 0.0f)
			.End()
			.AddGroup(B_VERTICAL, 4.0f)
				.AddGroup(B_HORIZONTAL, 4.0f)
					.Add(fRunButton = new BButton("Run"))
					.Add(fStepOverButton = new BButton("Step Over"))
					.Add(fStepIntoButton = new BButton("Step Into"))
					.Add(fStepOutButton = new BButton("Step Out"))
					.AddGlue()
					.SetInsets(4.0f, 0.0f, 4.0f, 0.0f)
				.End()
				.AddSplit(B_HORIZONTAL, 3.0f)
					.Add(sourceScrollView = new BScrollView("source scroll",
						NULL, 0, true, true), 3.0f)
					.AddGroup(B_VERTICAL)
						.Add(fLocalsTabView = new BTabView("locals view"))
						.SetInsets(0.0f, 0.0f, 4.0f, 4.0f)
					.End()
				.End()
			.End()
		.End();

	// add source view
	sourceScrollView->SetTarget(fSourceView = SourceView::Create(fDebugModel,
		this));

	// add threads tab
	BSplitView* threadGroup = new BSplitView(B_HORIZONTAL);
	threadGroup->SetName("Threads");
	fTabView->AddTab(threadGroup);
	BLayoutBuilder::Split<>(threadGroup)
		.Add(fThreadListView = ThreadListView::Create(this))
		.Add(fStackTraceView = StackTraceView::Create(this));

	// add images tab
	BSplitView* imagesGroup = new BSplitView(B_HORIZONTAL);
	imagesGroup->SetName("Images");
	fTabView->AddTab(imagesGroup);
	BLayoutBuilder::Split<>(imagesGroup)
		.Add(fImageListView = ImageListView::Create())
		.Add(new BTextView("source files"));

	// add local variables tab
	BView* tab = new BTextView("Variables");
	fLocalsTabView->AddTab(tab);

	// add registers tab
	tab = fRegisterView = RegisterView::Create(fDebugModel->GetArchitecture());
	fLocalsTabView->AddTab(tab);

	fThreadListView->SetTeam(fDebugModel->GetTeam());
	fImageListView->SetTeam(fDebugModel->GetTeam());

	fRunButton->SetMessage(new BMessage(MSG_THREAD_RUN));
	fStepOverButton->SetMessage(new BMessage(MSG_THREAD_STEP_OVER));
	fStepIntoButton->SetMessage(new BMessage(MSG_THREAD_STEP_INTO));
	fStepOutButton->SetMessage(new BMessage(MSG_THREAD_STEP_OUT));
	fRunButton->SetTarget(this);
	fStepOverButton->SetTarget(this);
	fRunButton->SetTarget(this);
	fStepOutButton->SetTarget(this);

	AutoLocker<TeamDebugModel> locker(fDebugModel);
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

	AutoLocker<TeamDebugModel> locker(fDebugModel);
	_UpdateRunButtons();

	StackTrace* stackTrace = fActiveThread != NULL
		? fActiveThread->GetStackTrace() : NULL;
	Reference<StackTrace> stackTraceReference(stackTrace);
		// hold a reference until the register view has one

	locker.Unlock();

	_SetActiveStackTrace(stackTrace);
	_UpdateCpuState();
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

	AutoLocker<TeamDebugModel> locker(fDebugModel);

	if (fActiveStackFrame != NULL) {
		fActiveStackFrame->RemoveListener(this);
		fActiveStackFrame->RemoveReference();
	}

	fActiveStackFrame = frame;

	SourceCode* sourceCode = NULL;
	Reference<SourceCode> sourceCodeReference;
	bool setSourceCode = false;

	if (fActiveStackFrame != NULL) {
		fActiveStackFrame->AddReference();
		fActiveStackFrame->AddListener(this);

		sourceCode = fActiveStackFrame->GetSourceCode();
		sourceCodeReference.SetTo(sourceCode);
		setSourceCode = true;

		// If the source code is not loaded yet, request it.
		if (fActiveStackFrame->SourceCodeState() == STACK_SOURCE_NOT_LOADED)
			fListener->StackFrameSourceCodeRequested(this, fActiveStackFrame);
	}

	_UpdateCpuState();

	locker.Unlock();

	if (setSourceCode)
		_SetActiveSourceCode(sourceCode);

	fStackTraceView->SetStackFrame(fActiveStackFrame);
	fSourceView->SetStackFrame(fActiveStackFrame);
}


void
TeamWindow::_SetActiveSourceCode(SourceCode* sourceCode)
{
	if (sourceCode == fActiveSourceCode)
		return;

	if (fActiveSourceCode != NULL)
		fActiveSourceCode->RemoveReference();

	fActiveSourceCode = sourceCode;

	if (fActiveSourceCode != NULL)
		fActiveSourceCode->AddReference();

	fSourceView->SetSourceCode(fActiveSourceCode);
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
			AutoLocker<TeamDebugModel> locker(fDebugModel);
			cpuState = fActiveThread->GetCpuState();
			cpuStateReference.SetTo(cpuState);
			locker.Unlock();
		} else
			cpuState = fActiveStackFrame->GetCpuState();
	}

	fRegisterView->SetCpuState(cpuState);
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
			fRunButton->SetLabel("Stop");
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
TeamWindow::_HandleThreadStateChanged(thread_id threadID)
{
	// ATM we're only interested in the currently selected thread
	if (fActiveThread == NULL || threadID != fActiveThread->ID())
		return;

	AutoLocker<TeamDebugModel> locker(fDebugModel);
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

	AutoLocker<TeamDebugModel> locker(fDebugModel);

	StackTrace* stackTrace = fActiveThread != NULL
		? fActiveThread->GetStackTrace() : NULL;
	Reference<StackTrace> stackTraceReference(stackTrace);
		// hold a reference until the register view has one

	locker.Unlock();

	_SetActiveStackTrace(stackTrace);
}


void
TeamWindow::_HandleSourceCodeChanged()
{
	// If we don't have an active stack frame anymore, the message is obsolete.
	if (fActiveStackFrame == NULL)
		return;

	// get a reference to the source code
	AutoLocker<TeamDebugModel> locker(fDebugModel);

	SourceCode* sourceCode = fActiveStackFrame->GetSourceCode();
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
