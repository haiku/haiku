/*
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#include "PromptWindow.h"

#include <Button.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <TextControl.h>


static const uint32 kAcceptInput = 'acin';


PromptWindow::PromptWindow(const char* title, const char* label,
	BMessenger target, BMessage* message)
	:
	BWindow(BRect(), title, B_FLOATING_WINDOW, B_NOT_RESIZABLE
			| B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE),
	fTarget(target),
	fMessage(message)
{
	fTextControl = new BTextControl("promptcontrol", label, NULL,
		new BMessage(kAcceptInput));
	BButton* cancelButton = new BButton("Cancel", new
		BMessage(B_QUIT_REQUESTED));
	BButton* acceptButton = new BButton("Accept", new
		BMessage(kAcceptInput));
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(fTextControl)
		.AddGroup(B_HORIZONTAL)
			.Add(acceptButton)
			.Add(cancelButton);

	fTextControl->TextView()->SetExplicitMinSize(BSize(
			fTextControl->TextView()->StringWidth("1234567890"), B_SIZE_UNSET));
	fTextControl->SetTarget(this);
	acceptButton->SetTarget(this);
	cancelButton->SetTarget(this);
	fTextControl->MakeFocus(true);
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
