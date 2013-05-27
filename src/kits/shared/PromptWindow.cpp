/*
 * Copyright 2012-2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#include "PromptWindow.h"

#include <Button.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <StringView.h>
#include <TextControl.h>


static const uint32 kAcceptInput = 'acin';


PromptWindow::PromptWindow(const char* title, const char* label,
	const char* info, BMessenger target, BMessage* message)
	:
	BWindow(BRect(), title, B_FLOATING_WINDOW, B_NOT_RESIZABLE
			| B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE),
	fTarget(target),
	fMessage(message)
{
	fInfoView = new BStringView("info", info);
	fTextControl = new BTextControl("promptcontrol", label, NULL,
		new BMessage(kAcceptInput));
	BButton* cancelButton = new BButton("Cancel", new
		BMessage(B_QUIT_REQUESTED));
	BButton* acceptButton = new BButton("Accept", new
		BMessage(kAcceptInput));
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(fInfoView)
		.Add(fTextControl)
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(cancelButton)
			.Add(acceptButton)
		.End()
	.End();

	if (info == NULL)
		fInfoView->Hide();

	fTextControl->TextView()->SetExplicitMinSize(BSize(200.0, B_SIZE_UNSET));
	fTextControl->SetTarget(this);
	acceptButton->SetTarget(this);
	cancelButton->SetTarget(this);
	fTextControl->MakeFocus(true);

	SetDefaultButton(acceptButton);
}


PromptWindow::~PromptWindow()
{
	delete fMessage;
}


void
PromptWindow::MessageReceived(BMessage* message)
{
	switch (message->what)
	{
		case kAcceptInput:
		{
			fMessage->AddString("text", fTextControl->TextView()->Text());
			fTarget.SendMessage(fMessage);
			PostMessage(B_QUIT_REQUESTED);
		}
		default:
		{
			BWindow::MessageReceived(message);
			break;
		}
	}
}


status_t
PromptWindow::SetTarget(BMessenger messenger)
{
	fTarget = messenger;
	return B_OK;
}


status_t
PromptWindow::SetMessage(BMessage* message)
{
	delete fMessage;
	fMessage = message;
	return B_OK;
}
