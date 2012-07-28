/*
 * Copyright 2002-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mattias Sundblad
 *		Andrew Bachmann
 */


#include "Constants.h"
#include "ReplaceWindow.h"

#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <GroupLayoutBuilder.h>
#include <GridLayoutBuilder.h>
#include <Handler.h>
#include <Locale.h>
#include <LayoutBuilder.h>
#include <Message.h>
#include <Messenger.h>
#include <String.h>
#include <TextControl.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "FindandReplaceWindow"

ReplaceWindow::ReplaceWindow(BRect frame, BHandler* _handler,
	BString* searchString, 	BString* replaceString,
	bool caseState, bool wrapState, bool backState)
	: BWindow(frame, "ReplaceWindow", B_MODAL_WINDOW,
		B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS,
		B_CURRENT_WORKSPACE)
{
	AddShortcut('W', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));

	fSearchString = new BTextControl("", B_TRANSLATE("Find:"), NULL, NULL);
	fReplaceString = new BTextControl("", B_TRANSLATE("Replace with:"),
		NULL, NULL);
	fCaseSensBox = new BCheckBox("", B_TRANSLATE("Case-sensitive"), NULL);
	fWrapBox = new BCheckBox("", B_TRANSLATE("Wrap-around search"), NULL);
	fBackSearchBox = new BCheckBox("", B_TRANSLATE("Search backwards"), NULL);
	fAllWindowsBox = new BCheckBox("", B_TRANSLATE("Replace in all windows"),
		new BMessage(CHANGE_WINDOW));
	fUIchange = false;

	fReplaceAllButton = new BButton("", B_TRANSLATE("Replace all"),
		new BMessage(MSG_REPLACE_ALL));
	fCancelButton = new BButton("", B_TRANSLATE("Cancel"),
		new BMessage(B_QUIT_REQUESTED));
	fReplaceButton = new BButton("", B_TRANSLATE("Replace"),
		new BMessage(MSG_REPLACE));

	SetLayout(new BGroupLayout(B_HORIZONTAL));
	AddChild(BGroupLayoutBuilder(B_VERTICAL, 4)
		.Add(BGridLayoutBuilder(6, 2)
				.Add(fSearchString->CreateLabelLayoutItem(), 0, 0)
				.Add(fSearchString->CreateTextViewLayoutItem(), 1, 0)
				.Add(fReplaceString->CreateLabelLayoutItem(), 0, 1)
				.Add(fReplaceString->CreateTextViewLayoutItem(), 1, 1)
				.Add(fCaseSensBox, 1, 2)
				.Add(fWrapBox, 1, 3)
				.Add(fBackSearchBox, 1, 4)
				.Add(fAllWindowsBox, 1, 5)
				)
		.AddGroup(B_HORIZONTAL, 10)
			.Add(fReplaceAllButton)
			.AddGlue()
			.Add(fCancelButton)
			.Add(fReplaceButton)
		.End()
		.SetInsets(10, 10, 10, 10)
	);

	fReplaceButton->MakeDefault(true);

	fHandler = _handler;

	const char* searchtext = searchString->String();
	const char* replacetext = replaceString->String();

	fSearchString->SetText(searchtext);
	fReplaceString->SetText(replacetext);
	fSearchString->MakeFocus(true);

	fCaseSensBox->SetValue(caseState ? B_CONTROL_ON : B_CONTROL_OFF);
	fWrapBox->SetValue(wrapState ? B_CONTROL_ON : B_CONTROL_OFF);
	fBackSearchBox->SetValue(backState ? B_CONTROL_ON : B_CONTROL_OFF);
}


void
ReplaceWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case MSG_REPLACE:
			_SendMessage(MSG_REPLACE);
			break;
		case CHANGE_WINDOW:
			_ChangeUI();
			break;
		case MSG_REPLACE_ALL:
			_SendMessage(MSG_REPLACE_ALL);
			break;

		default:
			BWindow::MessageReceived(msg);
			break;
	}
}


void
ReplaceWindow::_ChangeUI()
{
	fWrapBox->SetEnabled(fUIchange);
	fWrapBox->SetValue(fUIchange ? B_CONTROL_OFF : B_CONTROL_ON);

	fBackSearchBox->SetEnabled(fUIchange);

	fReplaceButton->SetEnabled(fUIchange);
	if (fUIchange)
		fReplaceButton->MakeDefault(true);
	else
		fReplaceAllButton->MakeDefault(true);

	fUIchange = !fUIchange;
}


void
ReplaceWindow::DispatchMessage(BMessage* message, BHandler* handler)
{
	if (message->what == B_KEY_DOWN) {
		int8 key;
		if (message->FindInt8("byte", 0, &key) == B_OK) {
			if (key == B_ESCAPE) {
				message->MakeEmpty();
				message->what = B_QUIT_REQUESTED;

				// This is a hack, but it actually does what is expected,
				// unlike the hack above. This kind of key filtering probably
				// ought to be handled by a BMessageFilter, though.
				BMessenger (this).SendMessage(B_QUIT_REQUESTED);
			}
		}
	}

	BWindow::DispatchMessage(message, handler);
}


void
ReplaceWindow::_SendMessage(uint32 what)
{
	BMessage message(what);

	// Add the strings
	message.AddString("FindText", fSearchString->Text());
	message.AddString("ReplaceText", fReplaceString->Text());

	// Add searchparameters from checkboxes
	message.AddBool("casesens", fCaseSensBox->Value() == B_CONTROL_ON);
	message.AddBool("wrap", fWrapBox->Value() == B_CONTROL_ON);
	message.AddBool("backsearch", fBackSearchBox->Value() == B_CONTROL_ON);
	message.AddBool("allwindows", fAllWindowsBox->Value() == B_CONTROL_ON);

	fHandler->Looper()->PostMessage(&message, fHandler);

	PostMessage(B_QUIT_REQUESTED);
}

