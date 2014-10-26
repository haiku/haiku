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
	SourceLanguage* language, UserInterfaceListener* listener)
	:
	BWindow(BRect(), "Evaluate Expression", B_FLOATING_WINDOW,
		B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE),
	fLanguage(language),
	fListener(listener)
{
	fLanguage->AcquireReference();
}


ExpressionEvaluationWindow::~ExpressionEvaluationWindow()
{
	fLanguage->ReleaseReference();
}


ExpressionEvaluationWindow*
ExpressionEvaluationWindow::Create(SourceLanguage* language,
	UserInterfaceListener* listener)
{
	ExpressionEvaluationWindow* self = new ExpressionEvaluationWindow(
		language, listener);

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
	fExpressionInput = new BTextControl("Expression:", NULL, NULL);
	BLayoutItem* labelItem = fExpressionInput->CreateLabelLayoutItem();
	labelItem->View()->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.AddGroup(B_HORIZONTAL, 4.0f)
			.Add(labelItem)
			.Add(fExpressionInput->CreateTextViewLayoutItem())
		.End()
		.AddGroup(B_HORIZONTAL, 4.0f)
			.Add(new BStringView("OutputLabelView", "Result:"))
			.Add((fExpressionOutput = new BStringView("ExpressionOutputView",
					NULL)))
		.End()
		.AddGroup(B_HORIZONTAL, 4.0f)
			.AddGlue()
			.Add((fEvaluateButton = new BButton("Evaluate",
					new BMessage(MSG_EVALUATE_EXPRESSION))))
		.End();

	fEvaluateButton->SetTarget(this);

}


void
ExpressionEvaluationWindow::Show()
{
	CenterOnScreen();
	BWindow::Show();
}


void
ExpressionEvaluationWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_EVALUATE_EXPRESSION:
		{
			Value* value = NULL;
			status_t error = fLanguage->EvaluateExpression(
				fExpressionInput->TextView()->Text(), value);
			if (error != B_OK) {
				BString errorText;
				errorText.SetToFormat("Failed to evaluate expression: %s",
					strerror(error));
				BAlert* alert = new(std::nothrow) BAlert("Evaluate Expression",
					errorText.String(), "Close");
				if (alert != NULL)
					alert->Go();
				break;
			}

			BString valueText;
			value->ToString(valueText);
			fExpressionOutput->SetText(valueText);
			break;
		}
		default:
			BWindow::MessageReceived(message);
			break;
	}

}
