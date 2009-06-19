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
#include <SplitView.h>
#include <TextView.h>

#include <AutoLocker.h>

#include "ImageListView.h"
#include "MessageCodes.h"
#include "TeamDebugModel.h"


// #pragma mark - TeamWindow


TeamWindow::TeamWindow(TeamDebugModel* debugModel, Listener* listener)
	:
	BWindow(BRect(100, 100, 899, 699), "Team", B_DOCUMENT_WINDOW,
		B_ASYNCHRONOUS_CONTROLS),
	fDebugModel(debugModel),
	fActiveThread(NULL),
	fListener(listener),
	fTabView(NULL),
	fThreadListView(NULL),
	fImageListView(NULL),
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

	team->AddListener(this);
}


TeamWindow::~TeamWindow()
{
	fDebugModel->GetTeam()->RemoveListener(this);
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
//		case MSG_THREAD_CPU_STATE_CHANGED:
//		case MSG_THREAD_STACK_TRACE_CHANGED:

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
TeamWindow::ThreadStateChanged(const Team::ThreadEvent& event)
{
	BMessage message(MSG_THREAD_STATE_CHANGED);
	message.AddInt32("thread", event.GetThread()->ID());
	PostMessage(&message);
}


void
TeamWindow::ThreadCpuStateChanged(const Team::ThreadEvent& event)
{
//	BMessage message(MSG_THREAD_CPU_STATE_CHANGED);
//	message.AddInt32("thread", event.GetThread()->ID());
//	PostMessage(&message);
}


void
TeamWindow::ThreadStackTraceChanged(const Team::ThreadEvent& event)
{
//	BMessage message(MSG_THREAD_STACK_TRACE_CHANGED);
//	message.AddInt32("thread", event.GetThread()->ID());
//	PostMessage(&message);
}


void
TeamWindow::_Init()
{
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.AddSplit(B_VERTICAL, 3.0f)
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
					.Add(new BTextView("source view"), 3.0f)
					.Add(new BTextView("variables view"))
				.End()
			.End()
		.End();

	// add threads tab
	BSplitView* threadGroup = new BSplitView(B_HORIZONTAL);
	threadGroup->SetName("Threads");
	fTabView->AddTab(threadGroup);
	BLayoutBuilder::Split<>(threadGroup)
		.Add(fThreadListView = ThreadListView::Create(this))
		.Add(new BTextView("stack frames"));

	// add images tab
	BSplitView* imagesGroup = new BSplitView(B_HORIZONTAL);
	imagesGroup->SetName("Images");
	fTabView->AddTab(imagesGroup);
	BLayoutBuilder::Split<>(imagesGroup)
		.Add(fImageListView = ImageListView::Create())
		.Add(new BTextView("source files"));

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

	fActiveThread = thread;

	AutoLocker<TeamDebugModel> locker(fDebugModel);
	_UpdateRunButtons();
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


// #pragma mark - Listener


TeamWindow::Listener::~Listener()
{
}
