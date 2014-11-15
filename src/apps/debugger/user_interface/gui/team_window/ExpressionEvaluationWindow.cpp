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

#include "IntegerValue.h"
#include "MessageCodes.h"
#include "SourceLanguage.h"
#include "StackFrame.h"
#include "SyntheticPrimitiveType.h"
#include "Thread.h"
#include "UiUtils.h"
#include "UserInterface.h"


ExpressionEvaluationWindow::ExpressionEvaluationWindow(
	SourceLanguage* language, StackFrame* frame, ::Thread* thread,
	UserInterfaceListener* listener, BHandler* target)
	:
	BWindow(BRect(), "Evaluate Expression", B_FLOATING_WINDOW,
		B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE),
	fLanguage(language),
	fExpressionInfo(NULL),
	fExpressionInput(NULL),
	fExpressionOutput(NULL),
	fEvaluateButton(NULL),
	fListener(listener),
	fStackFrame(frame),
	fThread(thread),
	fCloseTarget(target)
{
	fLanguage->AcquireReference();

	if (fStackFrame != NULL)
		fStackFrame->AcquireReference();

	if (fThread != NULL)
		fThread->AcquireReference();
}


ExpressionEvaluationWindow::~ExpressionEvaluationWindow()
{
	fLanguage->ReleaseReference();

	if (fStackFrame != NULL)
		fStackFrame->ReleaseReference();

	if (fThread != NULL)
		fThread->ReleaseReference();

	fExpressionInfo->ReleaseReference();
}


ExpressionEvaluationWindow*
ExpressionEvaluationWindow::Create(SourceLanguage* language, StackFrame* frame,
	::Thread* thread, UserInterfaceListener* listener, BHandler* target)
{
	ExpressionEvaluationWindow* self = new ExpressionEvaluationWindow(language,
		frame, thread, listener, target);

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
	fExpressionInfo = new ExpressionInfo;
	fExpressionInfo->AddListener(this);

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

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.AddGroup(B_HORIZONTAL, 4.0f)
			.Add(labelItem)
			.Add(inputItem)
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
	fEvaluateButton->MakeDefault(true);
	fExpressionInput->TextView()->MakeFocus(true);
}


void
ExpressionEvaluationWindow::ExpressionEvaluated(ExpressionInfo* info,
	status_t result, ExpressionResult* value)
{
	BMessage message(MSG_EXPRESSION_EVALUATED);
	message.AddInt32("result", result);
	BReference<ExpressionResult> reference;
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

			fExpressionInfo->SetTo(fExpressionInput->Text());

			fListener->ExpressionEvaluationRequested(fLanguage,
				fExpressionInfo, fStackFrame, fThread);
			break;
		}
		case MSG_EXPRESSION_EVALUATED:
		{
			ExpressionResult* value = NULL;
			BReference<ExpressionResult> reference;
			if (message->FindPointer("value",
				reinterpret_cast<void**>(&value)) == B_OK) {
				reference.SetTo(value, true);
			}

			BString outputText;
			if (value != NULL) {
				if (value->Kind() == EXPRESSION_RESULT_KIND_PRIMITIVE) {
					Value* primitive = value->PrimitiveValue();
					if (dynamic_cast<IntegerValue*>(primitive) != NULL) {
						BVariant variantValue;
						primitive->ToVariant(variantValue);
						primitive->ToString(outputText);
						outputText.SetToFormat("%#" B_PRIx64 " (%s)",
							variantValue.ToUInt64(), outputText.String());
					} else
						primitive->ToString(outputText);
				} else {
					outputText.SetToFormat("Unsupported result type: %d",
						value->Kind());
				}
			} else {
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
