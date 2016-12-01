/*
 * Copyright 2014-2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#include "ExpressionEvaluationWindow.h"

#include <Button.h>
#include <LayoutBuilder.h>
#include <MenuField.h>
#include <String.h>
#include <TextControl.h>

#include <AutoDeleter.h>
#include <AutoLocker.h>

#include "AppMessageCodes.h"
#include "CppLanguage.h"
#include "FunctionDebugInfo.h"
#include "FunctionInstance.h"
#include "MessageCodes.h"
#include "SourceLanguage.h"
#include "SpecificImageDebugInfo.h"
#include "StackFrame.h"
#include "StackTrace.h"
#include "Thread.h"
#include "UiUtils.h"
#include "UserInterface.h"
#include "ValueNodeManager.h"


enum {
	MSG_THREAD_ADDED				= 'thad',
	MSG_THREAD_REMOVED				= 'thar',

	MSG_THREAD_SELECTION_CHANGED	= 'thsc',
	MSG_FRAME_SELECTION_CHANGED		= 'frsc'
};


ExpressionEvaluationWindow::ExpressionEvaluationWindow(BHandler* closeTarget,
		::Team* team, UserInterfaceListener* listener)
	:
	BWindow(BRect(), "Evaluate Expression", B_TITLED_WINDOW,
		B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE),
	fExpressionInput(NULL),
	fThreadList(NULL),
	fFrameList(NULL),
	fVariablesView(NULL),
	fCloseButton(NULL),
	fEvaluateButton(NULL),
	fCloseTarget(closeTarget),
	fCurrentLanguage(NULL),
	fFallbackLanguage(NULL),
	fTeam(team),
	fSelectedThread(NULL),
	fSelectedFrame(NULL),
	fListener(listener)
{
	team->AddListener(this);
}


ExpressionEvaluationWindow::~ExpressionEvaluationWindow()
{
	fTeam->RemoveListener(this);

	if (fCurrentLanguage != NULL)
		fCurrentLanguage->ReleaseReference();

	if (fFallbackLanguage != NULL)
		fFallbackLanguage->ReleaseReference();

	if (fSelectedThread != NULL)
		fSelectedThread->ReleaseReference();

	if (fSelectedFrame != NULL)
		fSelectedFrame->ReleaseReference();
}


ExpressionEvaluationWindow*
ExpressionEvaluationWindow::Create(BHandler* closeTarget, ::Team* team,
	UserInterfaceListener* listener)
{
	ExpressionEvaluationWindow* self = new ExpressionEvaluationWindow(
		closeTarget, team, listener);

	try {
		self->_Init();
	} catch (...) {
		delete self;
		throw;
	}

	return self;

}


void
ExpressionEvaluationWindow::Show()
{
	CenterOnScreen();
	BWindow::Show();
}


bool
ExpressionEvaluationWindow::QuitRequested()
{
	BMessenger messenger(fCloseTarget);
	messenger.SendMessage(MSG_EXPRESSION_WINDOW_CLOSED);

	return BWindow::QuitRequested();
}


void
ExpressionEvaluationWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_THREAD_SELECTION_CHANGED:
		{
			int32 threadID;
			if (message->FindInt32("thread", &threadID) != B_OK)
				threadID = -1;

			_HandleThreadSelectionChanged(threadID);
			break;
		}

		case MSG_FRAME_SELECTION_CHANGED:
		{
			if (fSelectedThread == NULL)
				break;

			int32 frameIndex;
			if (message->FindInt32("frame", &frameIndex) != B_OK)
				frameIndex = -1;

			_HandleFrameSelectionChanged(frameIndex);
			break;
		}

		case MSG_EVALUATE_EXPRESSION:
		{
			BMessage message(MSG_ADD_NEW_EXPRESSION);
			message.AddString("expression", fExpressionInput->Text());
			BMessenger(fVariablesView).SendMessage(&message);
			break;
		}

		case MSG_THREAD_ADDED:
		{
			int32 threadID;
			if (message->FindInt32("thread", &threadID) == B_OK)
				_HandleThreadAdded(threadID);
			break;
		}

		case MSG_THREAD_REMOVED:
		{
			int32 threadID;
			if (message->FindInt32("thread", &threadID) == B_OK)
				_HandleThreadRemoved(threadID);
			break;
		}

		case MSG_THREAD_STATE_CHANGED:
		{
			int32 threadID;
			if (message->FindInt32("thread", &threadID) == B_OK)
				_HandleThreadStateChanged(threadID);
			break;
		}

		case MSG_THREAD_STACK_TRACE_CHANGED:
		{
			int32 threadID;
			if (message->FindInt32("thread", &threadID) == B_OK)
				_HandleThreadStackTraceChanged(threadID);
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}

}


void
ExpressionEvaluationWindow::ThreadAdded(const Team::ThreadEvent& event)
{
	BMessage message(MSG_THREAD_ADDED);
	message.AddInt32("thread", event.GetThread()->ID());
	PostMessage(&message);
}


void
ExpressionEvaluationWindow::ThreadRemoved(const Team::ThreadEvent& event)
{
	BMessage message(MSG_THREAD_REMOVED);
	message.AddInt32("thread", event.GetThread()->ID());
	PostMessage(&message);
}


void
ExpressionEvaluationWindow::ThreadStateChanged(const Team::ThreadEvent& event)
{
	BMessage message(MSG_THREAD_STATE_CHANGED);
	message.AddInt32("thread", event.GetThread()->ID());
	PostMessage(&message);
}


void
ExpressionEvaluationWindow::ThreadStackTraceChanged(
	const Team::ThreadEvent& event)
{
	BMessage message(MSG_THREAD_STACK_TRACE_CHANGED);
	message.AddInt32("thread", event.GetThread()->ID());
	PostMessage(&message);
}


void
ExpressionEvaluationWindow::ValueNodeValueRequested(CpuState* cpuState,
	ValueNodeContainer* container, ValueNode* valueNode)
{
	fListener->ValueNodeValueRequested(cpuState, container, valueNode);
}


void
ExpressionEvaluationWindow::ExpressionEvaluationRequested(ExpressionInfo* info,
	StackFrame* frame, ::Thread* thread)
{
	SourceLanguage* language = fCurrentLanguage;
	if (fCurrentLanguage == NULL)
		language = fFallbackLanguage;
	fListener->ExpressionEvaluationRequested(language, info, frame, thread);
}


void
ExpressionEvaluationWindow::ValueNodeWriteRequested(ValueNode* node,
	CpuState* state, Value* newValue)
{
}


void
ExpressionEvaluationWindow::_Init()
{
	ValueNodeManager* nodeManager = new ValueNodeManager(false);
	fExpressionInput = new BTextControl("Expression:", NULL, NULL);
	BLayoutItem* labelItem = fExpressionInput->CreateLabelLayoutItem();
	BLayoutItem* inputItem = fExpressionInput->CreateTextViewLayoutItem();
	inputItem->SetExplicitMinSize(BSize(200.0, B_SIZE_UNSET));
	inputItem->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	labelItem->View()->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.AddGroup(B_HORIZONTAL, 4.0f)
			.Add((fThreadList = new BMenuField("threadList", "Thread:",
					new BMenu("Thread"))))
			.Add((fFrameList = new BMenuField("frameList", "Frame:",
					new BMenu("Frame"))))
		.End()
		.AddGroup(B_HORIZONTAL, 4.0f)
			.Add(labelItem)
			.Add(inputItem)
		.End()
		.Add(fVariablesView = VariablesView::Create(this, nodeManager))
		.AddGroup(B_HORIZONTAL, 4.0f)
			.AddGlue()
			.Add((fCloseButton = new BButton("Close",
					new BMessage(B_QUIT_REQUESTED))))
			.Add((fEvaluateButton = new BButton("Evaluate",
					new BMessage(MSG_EVALUATE_EXPRESSION))))
		.End();

	fCloseButton->SetTarget(this);
	fEvaluateButton->SetTarget(this);
	fExpressionInput->TextView()->MakeFocus(true);

	fThreadList->Menu()->SetLabelFromMarked(true);
	fFrameList->Menu()->SetLabelFromMarked(true);

	fThreadList->Menu()->AddItem(new BMenuItem("<None>",
			new BMessage(MSG_THREAD_SELECTION_CHANGED)));
	fFrameList->Menu()->AddItem(new BMenuItem("<None>",
			new BMessage(MSG_FRAME_SELECTION_CHANGED)));

	_UpdateThreadList();

	fFallbackLanguage = new CppLanguage();
}


void
ExpressionEvaluationWindow::_HandleThreadSelectionChanged(int32 threadID)
{
	if (fSelectedThread != NULL) {
		fSelectedThread->ReleaseReference();
		fSelectedThread = NULL;
	}

	AutoLocker< ::Team> teamLocker(fTeam);
	fSelectedThread = fTeam->ThreadByID(threadID);
	if (fSelectedThread != NULL)
		fSelectedThread->AcquireReference();
	else if (fThreadList->Menu()->FindMarked() == NULL) {
		// if the selected thread was cleared due to a thread event
		// rather than user selection, we need to reset the marked item
		// to reflect the new state.
		fThreadList->Menu()->ItemAt(0)->SetMarked(true);
	}

	_UpdateFrameList();

	fVariablesView->SetStackFrame(fSelectedThread, fSelectedFrame);
}


void
ExpressionEvaluationWindow::_HandleFrameSelectionChanged(int32 index)
{
	if (fSelectedFrame != NULL) {
		fSelectedFrame->ReleaseReference();
		fSelectedFrame = NULL;
	}

	if (fCurrentLanguage != NULL) {
		fCurrentLanguage->ReleaseReference();
		fCurrentLanguage = NULL;
	}

	AutoLocker< ::Team> teamLocker(fTeam);
	StackTrace* stackTrace = fSelectedThread->GetStackTrace();
	if (stackTrace != NULL) {
		fSelectedFrame = stackTrace->FrameAt(index);
		if (fSelectedFrame != NULL) {
			fSelectedFrame->AcquireReference();

			FunctionInstance* instance = fSelectedFrame->Function();
			if (instance != NULL) {
				FunctionDebugInfo* functionInfo
					= instance->GetFunctionDebugInfo();
				SpecificImageDebugInfo* imageInfo =
					functionInfo->GetSpecificImageDebugInfo();

				if (imageInfo->GetSourceLanguage(functionInfo,
						fCurrentLanguage) == B_OK) {
					fCurrentLanguage->AcquireReference();
				}
			}
		}
	}

	fVariablesView->SetStackFrame(fSelectedThread, fSelectedFrame);
}


void
ExpressionEvaluationWindow::_HandleThreadAdded(int32 threadID)
{
	AutoLocker< ::Team> teamLocker(fTeam);
	::Thread* thread = fTeam->ThreadByID(threadID);
	if (thread == NULL)
		return;

	if (thread->State() != THREAD_STATE_STOPPED)
		return;

	BMenuItem* item = NULL;
	if (_CreateThreadMenuItem(thread, item) != B_OK)
		return;

	BMenu* threadMenu = fThreadList->Menu();
	int32 index = 1;
	// find appropriate insertion index to keep menu sorted in thread order.
	for (; index < threadMenu->CountItems(); index++) {
		BMenuItem* threadItem = threadMenu->ItemAt(index);
		BMessage* message = threadItem->Message();
		if (message->FindInt32("thread") > threadID)
			break;
	}

	bool added = false;
	if (index == threadMenu->CountItems())
		added = threadMenu->AddItem(item);
	else
		added = threadMenu->AddItem(item, index);

	if (!added)
		delete item;
}


void
ExpressionEvaluationWindow::_HandleThreadRemoved(int32 threadID)
{
	BMenu* threadMenu = fThreadList->Menu();
	for (int32 i = 0; i < threadMenu->CountItems(); i++) {
		BMenuItem* item = threadMenu->ItemAt(i);
		BMessage* message = item->Message();
		if (message->FindInt32("thread") == threadID) {
			threadMenu->RemoveItem(i);
			delete item;
			break;
		}
	}

	if (fSelectedThread != NULL && threadID == fSelectedThread->ID())
		_HandleThreadSelectionChanged(-1);
}


void
ExpressionEvaluationWindow::_HandleThreadStateChanged(int32 threadID)
{
	AutoLocker< ::Team> teamLocker(fTeam);

	::Thread* thread = fTeam->ThreadByID(threadID);
	if (thread == NULL)
		return;

	if (thread->State() == THREAD_STATE_STOPPED)
		_HandleThreadAdded(threadID);
	else
		_HandleThreadRemoved(threadID);
}


void
ExpressionEvaluationWindow::_HandleThreadStackTraceChanged(int32 threadID)
{
	AutoLocker< ::Team> teamLocker(fTeam);

	::Thread* thread = fTeam->ThreadByID(threadID);
	if (thread == NULL)
		return;

	if (thread != fSelectedThread)
		return;

	_UpdateFrameList();
}


void
ExpressionEvaluationWindow::_UpdateThreadList()
{
	AutoLocker< ::Team> teamLocker(fTeam);

	BMenu* frameMenu = fFrameList->Menu();
	while (frameMenu->CountItems() > 1)
		delete frameMenu->RemoveItem(1);

	BMenu* threadMenu = fThreadList->Menu();
	while (threadMenu->CountItems() > 1)
		delete threadMenu->RemoveItem(1);

	const ThreadList& threads = fTeam->Threads();
	for (ThreadList::ConstIterator it = threads.GetIterator();
		::Thread* thread = it.Next();) {
		if (thread->State() != THREAD_STATE_STOPPED)
			continue;

		BMenuItem* item = NULL;
		if (_CreateThreadMenuItem(thread, item) != B_OK)
			return;

		ObjectDeleter<BMenuItem> itemDeleter(item);
		if (!threadMenu->AddItem(item))
			return;

		itemDeleter.Detach();
		if (fSelectedThread == NULL) {
			item->SetMarked(true);
			_HandleThreadSelectionChanged(thread->ID());
		}
	}

	if (fSelectedThread == NULL)
		frameMenu->ItemAt(0L)->SetMarked(true);

}


void
ExpressionEvaluationWindow::_UpdateFrameList()
{
	AutoLocker< ::Team> teamLocker(fTeam);

	BMenu* frameMenu = fFrameList->Menu();
	while (frameMenu->CountItems() > 1)
		delete frameMenu->RemoveItem(1);

	frameMenu->ItemAt(0L)->SetMarked(true);

	if (fSelectedThread == NULL)
		return;

	StackTrace* stackTrace = fSelectedThread->GetStackTrace();
	if (stackTrace == NULL)
		return;

 	char buffer[128];
	for (int32 i = 0; i < stackTrace->CountFrames(); i++) {
		StackFrame* frame = stackTrace->FrameAt(i);
		UiUtils::FunctionNameForFrame(frame, buffer, sizeof(buffer));

		BMessage* message = new(std::nothrow) BMessage(
			MSG_FRAME_SELECTION_CHANGED);
		if (message == NULL)
			return;

		message->AddInt32("frame", i);

		BMenuItem* item = new(std::nothrow) BMenuItem(buffer,
			message);
		if (item == NULL)
			return;

		if (!frameMenu->AddItem(item))
			return;

		if (fSelectedFrame == NULL) {
			item->SetMarked(true);
			_HandleFrameSelectionChanged(i);
		}
	}
}


status_t
ExpressionEvaluationWindow::_CreateThreadMenuItem(::Thread* thread,
	BMenuItem*& _item) const
{
	BString nameString;
	nameString.SetToFormat("%" B_PRId32 ": %s", thread->ID(),
		thread->Name());

	BMessage* message = new(std::nothrow) BMessage(
		MSG_THREAD_SELECTION_CHANGED);
	if (message == NULL)
		return B_NO_MEMORY;

	ObjectDeleter<BMessage> messageDeleter(message);
	message->AddInt32("thread", thread->ID());
	_item = new(std::nothrow) BMenuItem(nameString,
		message);
	if (_item == NULL)
		return B_NO_MEMORY;

	messageDeleter.Detach();
	return B_OK;
}
