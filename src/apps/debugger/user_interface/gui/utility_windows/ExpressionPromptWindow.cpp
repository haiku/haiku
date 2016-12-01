/*
 * Copyright 2014-2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#include "ExpressionPromptWindow.h"

#include <Button.h>
#include <LayoutBuilder.h>
#include <String.h>
#include <TextControl.h>

#include "AppMessageCodes.h"


ExpressionPromptWindow::ExpressionPromptWindow(BHandler* addTarget,
	BHandler* closeTarget)
	:
	BWindow(BRect(), "Add Expression", B_FLOATING_WINDOW,
		B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE),
	fExpressionInput(NULL),
	fCancelButton(NULL),
	fAddButton(NULL),
	fAddTarget(addTarget),
	fCloseTarget(closeTarget)
{
}


ExpressionPromptWindow::~ExpressionPromptWindow()
{
}


ExpressionPromptWindow*
ExpressionPromptWindow::Create(BHandler* addTarget, BHandler* closeTarget)
{
	ExpressionPromptWindow* self = new ExpressionPromptWindow(addTarget,
		closeTarget);

	try {
		self->_Init();
	} catch (...) {
		delete self;
		throw;
	}

	return self;

}


void
ExpressionPromptWindow::_Init()
{
	fExpressionInput = new BTextControl("Expression:", NULL, NULL);
	BLayoutItem* labelItem = fExpressionInput->CreateLabelLayoutItem();
	BLayoutItem* inputItem = fExpressionInput->CreateTextViewLayoutItem();
	inputItem->SetExplicitMinSize(BSize(200.0, B_SIZE_UNSET));
	inputItem->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	labelItem->View()->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.AddGroup(B_HORIZONTAL, 4.0f)
			.Add(labelItem)
			.Add(inputItem)
		.End()
		.AddGroup(B_HORIZONTAL, 4.0f)
			.AddGlue()
			.Add((fCancelButton = new BButton("Cancel",
					new BMessage(B_QUIT_REQUESTED))))
			.Add((fAddButton = new BButton("Add",
					new BMessage(MSG_ADD_NEW_EXPRESSION))))
		.End();

	fExpressionInput->SetTarget(this);
	fCancelButton->SetTarget(this);
	fAddButton->SetTarget(this);
	fAddButton->MakeDefault(true);
	fExpressionInput->TextView()->MakeFocus(true);
}


void
ExpressionPromptWindow::Show()
{
	CenterOnScreen();
	BWindow::Show();
}


bool
ExpressionPromptWindow::QuitRequested()
{
	BMessenger messenger(fCloseTarget);
	messenger.SendMessage(MSG_EXPRESSION_PROMPT_WINDOW_CLOSED);

	return BWindow::QuitRequested();
}


void
ExpressionPromptWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_ADD_NEW_EXPRESSION:
		{
			BMessage addMessage(MSG_ADD_NEW_EXPRESSION);
			addMessage.AddString("expression", fExpressionInput->Text());
			addMessage.AddBool("persistent", true);

			BMessenger(fAddTarget).SendMessage(&addMessage);
			Quit();
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}

}
