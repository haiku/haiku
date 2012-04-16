/*
 * Copyright 2002-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mattias Sundblad
 *		Andrew Bachmann
 */


#include "Constants.h"
#include "FindWindow.h"

#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <GroupLayoutBuilder.h>
#include <GridLayoutBuilder.h>
#include <Locale.h>
#include <LayoutBuilder.h>
#include <String.h>
#include <TextControl.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "FindandReplaceWindow"

FindWindow::FindWindow(BRect frame, BHandler* _handler, BString* searchString,
	bool caseState, bool wrapState, bool backState)
	: BWindow(frame, "FindWindow", B_MODAL_WINDOW,
		B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS,
		B_CURRENT_WORKSPACE)
{
	AddShortcut('W', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));

	fSearchString = new BTextControl("", B_TRANSLATE("Find:"), NULL, NULL);
	fCaseSensBox = new BCheckBox("", B_TRANSLATE("Case-sensitive"), NULL);
	fWrapBox = new BCheckBox("", B_TRANSLATE("Wrap-around search"), NULL);
	fBackSearchBox = new BCheckBox("", B_TRANSLATE("Search backwards"), NULL);
	fCancelButton = new BButton("", B_TRANSLATE("Cancel"),
		new BMessage(B_QUIT_REQUESTED));
	fSearchButton = new BButton("", B_TRANSLATE("Find"),
		new BMessage(MSG_SEARCH));

	SetLayout(new BGroupLayout(B_HORIZONTAL));
	AddChild(BGroupLayoutBuilder(B_VERTICAL, 4)
		.Add(BGridLayoutBuilder(6, 2)
				.Add(fSearchString->CreateLabelLayoutItem(), 0, 0)
				.Add(fSearchString->CreateTextViewLayoutItem(), 1, 0)
				.Add(fCaseSensBox, 1, 1)
				.Add(fWrapBox, 1, 2)
				.Add(fBackSearchBox, 1, 3)
				)
		.AddGroup(B_HORIZONTAL, 10)
			.AddGlue()
			.Add(fCancelButton)
			.Add(fSearchButton)
		.End()
		.SetInsets(10, 10, 10, 10)
	);

	fSearchButton->MakeDefault(true);
	fHandler = _handler;

	const char* text = searchString->String();

	fSearchString->SetText(text);
	fSearchString->MakeFocus(true);

	fCaseSensBox->SetValue(caseState ? B_CONTROL_ON : B_CONTROL_OFF);
	fWrapBox->SetValue(wrapState ? B_CONTROL_ON : B_CONTROL_OFF);
	fBackSearchBox->SetValue(backState ? B_CONTROL_ON : B_CONTROL_OFF);
}


void
FindWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case B_QUIT_REQUESTED:
			Quit();
			break;
		case MSG_SEARCH:
			_SendMessage();
			break;

		default:
			BWindow::MessageReceived(msg);
			break;
	}
}


void
FindWindow::DispatchMessage(BMessage* message, BHandler* handler)
{
	if (message->what == B_KEY_DOWN) {
		int8 key;
		if (message->FindInt8("byte", 0, &key) == B_OK) {
			if (key == B_ESCAPE) {
				message->MakeEmpty();
				message->what = B_QUIT_REQUESTED;
			}
		}
	}

	BWindow::DispatchMessage(message, handler);
}


void
FindWindow::_SendMessage()
{
	BMessage message(MSG_SEARCH);

	// Add the string
	message.AddString("findtext", fSearchString->Text());

	// Add searchparameters from checkboxes
	message.AddBool("casesens", fCaseSensBox->Value() == B_CONTROL_ON);
	message.AddBool("wrap", fWrapBox->Value() == B_CONTROL_ON);
	message.AddBool("backsearch", fBackSearchBox->Value() == B_CONTROL_ON);

	fHandler->Looper()->PostMessage(&message, fHandler);

	PostMessage(B_QUIT_REQUESTED);
}


