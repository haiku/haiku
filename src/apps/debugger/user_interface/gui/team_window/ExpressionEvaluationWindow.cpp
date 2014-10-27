/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#include "ExpressionEvaluationWindow.h"

#include <Alert.h>
#include <Button.h>
#include <LayoutBuilder.h>
#include <String.h>
#include <StringView.h>
#include <TextControl.h>

#include "MessageCodes.h"
#include "SourceLanguage.h"
#include "UserInterface.h"
#include "Value.h"


enum {
	MSG_EVALUATE_EXPRESSION = 'evex'
};


ExpressionEvaluationWindow::ExpressionEvaluationWindow(
	SourceLanguage* language, UserInterfaceListener* listener,
	BHandler* target)
	:
	BWindow(BRect(), "Evaluate Expression", B_FLOATING_WINDOW,
		B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE),
	fLanguage(language),
	fListener(listener),
	fCloseTarget(target)
{
	fLanguage->AcquireReference();
}


ExpressionEvaluationWindow::~ExpressionEvaluationWindow()
{
	fLanguage->ReleaseReference();
}


ExpressionEvaluationWindow*
ExpressionEvaluationWindow::Create(SourceLanguage* language,
	UserInterfaceListener* listener, BHandler* target)
{
	ExpressionEvaluationWindow* self = new ExpressionEvaluationWindow(
		language, listener, target);

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
			Value* value = NULL;
			BString outputText;
			status_t error = fLanguage->EvaluateExpression(
				fExpressionInput->TextView()->Text(), B_INT64_TYPE, value);
			if (error != B_OK) {
				if (value != NULL)
					value->ToString(outputText);
				else {
					outputText.SetToFormat("Failed to evaluate expression: %s",
						strerror(error));
				}
			} else
				value->ToString(outputText);

			fExpressionOutput->SetText(outputText);
			break;
		}
		default:
			BWindow::MessageReceived(message);
			break;
	}

}
