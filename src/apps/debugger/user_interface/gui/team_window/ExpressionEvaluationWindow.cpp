/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#include "ExpressionEvaluationWindow.h"

#include <Alert.h>
#include <Button.h>
#include <LayoutBuilder.h>
#include <MenuField.h>
#include <String.h>
#include <StringView.h>
#include <TextControl.h>

#include "AutoLocker.h"

#include "MessageCodes.h"
#include "SourceLanguage.h"
#include "StackFrame.h"
#include "Thread.h"
#include "UserInterface.h"
#include "Value.h"


enum {
	MSG_CHANGE_EVALUATION_TYPE = 'chet'
};


ExpressionEvaluationWindow::ExpressionEvaluationWindow(
	::Team* team, SourceLanguage* language, StackFrame* frame,
	::Thread* thread, UserInterfaceListener* listener, BHandler* target)
	:
	BWindow(BRect(), "Evaluate Expression", B_FLOATING_WINDOW,
		B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE),
	fTeam(team),
	fLanguage(language),
	fExpressionInput(NULL),
	fExpressionOutput(NULL),
	fEvaluateButton(NULL),
	fListener(listener),
	fStackFrame(frame),
	fThread(thread),
	fCloseTarget(target),
	fCurrentEvaluationType(B_INT64_TYPE)
{
	fLanguage->AcquireReference();

	if (fStackFrame != NULL)
		fStackFrame->AcquireReference();

	if (fThread != NULL)
		fThread->AcquireReference();

	AutoLocker< ::Team> teamLocker(fTeam);
	fTeam->AddListener(this);
}


ExpressionEvaluationWindow::~ExpressionEvaluationWindow()
{
	fLanguage->ReleaseReference();

	if (fStackFrame != NULL)
		fStackFrame->ReleaseReference();

	if (fThread != NULL)
		fThread->ReleaseReference();

	AutoLocker< ::Team> teamLocker(fTeam);
	fTeam->RemoveListener(this);
}


ExpressionEvaluationWindow*
ExpressionEvaluationWindow::Create(::Team* team, SourceLanguage* language,
	StackFrame* frame, ::Thread* thread, UserInterfaceListener* listener,
	BHandler* target)
{
	ExpressionEvaluationWindow* self = new ExpressionEvaluationWindow(team,
		language, frame, thread, listener, target);

	try {
		self->_Init();
	} catch (...) {
		delete self;
		throw;
	}

	return self;

}


void
ExpressionEvaluationWindow::_Init()
{
	fExpressionInput = new BTextControl("Expression:", NULL,
		new BMessage(MSG_EVALUATE_EXPRESSION));
	BLayoutItem* labelItem = fExpressionInput->CreateLabelLayoutItem();
	BLayoutItem* inputItem = fExpressionInput->CreateTextViewLayoutItem();
	inputItem->SetExplicitMinSize(BSize(200.0, B_SIZE_UNSET));
	inputItem->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	labelItem->View()->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fExpressionOutput = new BStringView("ExpressionOutputView", NULL);
	fExpressionOutput->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED,
			B_SIZE_UNSET));

	BMenuField* typeField = new BMenuField("Type:", _BuildTypesMenu());
	typeField->Menu()->SetTargetForItems(this);

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.AddGroup(B_HORIZONTAL, 4.0f)
			.Add(labelItem)
			.Add(inputItem)
			.Add(typeField->CreateLabelLayoutItem())
			.Add(typeField->CreateMenuBarLayoutItem())
		.End()
		.AddGroup(B_HORIZONTAL, 4.0f)
			.Add(new BStringView("OutputLabelView", "Result:"))
			.Add(fExpressionOutput)
		.End()
		.AddGroup(B_HORIZONTAL, 4.0f)
			.AddGlue()
			.Add((fEvaluateButton = new BButton("Evaluate",
					new BMessage(MSG_EVALUATE_EXPRESSION))))
		.End();

	fExpressionInput->SetTarget(this);
	fEvaluateButton->SetTarget(this);
	fExpressionInput->TextView()->MakeFocus(true);
}


BMenu*
ExpressionEvaluationWindow::_BuildTypesMenu()
{
	BMenu* menu = new BMenu("Types");
	menu->SetLabelFromMarked(true);

	BMessage message(MSG_CHANGE_EVALUATION_TYPE);
	message.AddInt32("type", B_INT8_TYPE);

	menu->AddItem(new BMenuItem("int8", new BMessage(message)));

	message.ReplaceInt32("type", B_UINT8_TYPE);
	menu->AddItem(new BMenuItem("uint8", new BMessage(message)));

	message.ReplaceInt32("type", B_INT16_TYPE);
	menu->AddItem(new BMenuItem("int16", new BMessage(message)));

	message.ReplaceInt32("type", B_UINT16_TYPE);
	menu->AddItem(new BMenuItem("uint16", new BMessage(message)));

	message.ReplaceInt32("type", B_INT32_TYPE);
	menu->AddItem(new BMenuItem("int32", new BMessage(message)));

	message.ReplaceInt32("type", B_UINT32_TYPE);
	menu->AddItem(new BMenuItem("uint32", new BMessage(message)));

	message.ReplaceInt32("type", B_INT64_TYPE);
	BMenuItem* item = new BMenuItem("int64", new BMessage(message));
	menu->AddItem(item);
	item->SetMarked(true);

	message.ReplaceInt32("type", B_UINT64_TYPE);
	menu->AddItem(new BMenuItem("uint64", new BMessage(message)));

	message.ReplaceInt32("type", B_FLOAT_TYPE);
	menu->AddItem(new BMenuItem("float", new BMessage(message)));

	message.ReplaceInt32("type", B_DOUBLE_TYPE);
	menu->AddItem(new BMenuItem("double", new BMessage(message)));

	return menu;
}


void
ExpressionEvaluationWindow::ExpressionEvaluated(
	const Team::ExpressionEvaluationEvent& event)
{
	BMessage message(MSG_EXPRESSION_EVALUATED);

	AutoLocker<BLooper> lock(this);
	if (!lock.IsLocked())
		return;

	if (event.GetExpression() != fExpressionInput->Text())
		return;

	lock.Unlock();
	message.AddInt32("result", event.GetResult());
	BReference<Value> reference;
	Value* value = event.GetValue();
	if (value != NULL) {
		message.AddPointer("value", value);
		reference.SetTo(value);
	}

	if (PostMessage(&message) == B_OK)
		reference.Detach();
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
		case MSG_EVALUATE_EXPRESSION:
		{
			if (fExpressionInput->TextView()->TextLength() == 0)
				break;

			fListener->ExpressionEvaluationRequested(fLanguage,
				fExpressionInput->Text(), fCurrentEvaluationType, fStackFrame,
				fThread);
			break;
		}

		case MSG_CHANGE_EVALUATION_TYPE:
		{
			fCurrentEvaluationType = message->FindInt32("type");
			break;
		}

		case MSG_EXPRESSION_EVALUATED:
		{
			Value* value = NULL;
			BReference<Value> reference;
			if (message->FindPointer("value",
				reinterpret_cast<void**>(&value)) == B_OK) {
				reference.SetTo(value, true);
			}

			BString outputText;
			if (value != NULL)
				value->ToString(outputText);
			else {
				status_t result;
				if (message->FindInt32("result", &result) != B_OK)
					result = B_ERROR;

				outputText.SetToFormat("Failed to evaluate expression: %s",
					strerror(result));
			}

			fExpressionOutput->SetText(outputText);
			break;
		}


		default:
			BWindow::MessageReceived(message);
			break;
	}

}
