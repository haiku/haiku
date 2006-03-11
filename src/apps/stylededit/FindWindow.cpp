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
#include <CheckBox.h>
#include <String.h>
#include <TextControl.h>


FindWindow::FindWindow(BRect frame, BHandler *_handler, BString *searchString,
	bool *caseState, bool *wrapState, bool *backState)
	: BWindow(frame, "FindWindow", B_MODAL_WINDOW,
		B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS,
		B_CURRENT_WORKSPACE)
{
	fFindView = new BView(Bounds(), "FindView", B_FOLLOW_ALL, B_WILL_DRAW);
	fFindView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(fFindView);

	font_height height;
	fFindView->GetFontHeight(&height);
	float lineHeight = height.ascent+height.descent + height.leading;

	float findWidth = fFindView->StringWidth("Find:") + 6;

	float searchBottom = 12 + 2 + lineHeight + 2 + 1;
	float buttonTop = frame.Height() - 19 - lineHeight;
	float wrapBoxTop = (buttonTop + searchBottom - lineHeight) / 2;
	float wrapBoxBottom = (buttonTop + searchBottom + lineHeight) / 2;
	float caseBoxTop = (searchBottom + wrapBoxTop - lineHeight) / 2;
	float caseBoxBottom = (searchBottom + wrapBoxTop + lineHeight) / 2;
	float backBoxTop = (buttonTop + wrapBoxBottom - lineHeight) / 2;
	float backBoxBottom = (buttonTop + wrapBoxBottom + lineHeight) / 2;

	fFindView->AddChild(fSearchString = new BTextControl(BRect(14, 12,
		frame.Width() - 10, searchBottom), "", "Find:", NULL, NULL));
	fSearchString->SetDivider(findWidth);

	fFindView->AddChild(fCaseSensBox = new BCheckBox(BRect(16 + findWidth, caseBoxTop,
		frame.Width() - 12, caseBoxBottom), "", "Case-sensitive", NULL));
	fFindView->AddChild(fWrapBox = new BCheckBox(BRect(16 + findWidth, wrapBoxTop,
		frame.Width() - 12, wrapBoxBottom), "", "Wrap-around search", NULL));
	fFindView->AddChild(fBackSearchBox = new BCheckBox(BRect(16 + findWidth,
		backBoxTop, frame.Width() - 12, backBoxBottom), "", "Search backwards", NULL));
	
	fFindView->AddChild(fCancelButton = new BButton(BRect(142, buttonTop, 212,
		frame.Height() - 7), "", "Cancel", new BMessage(B_QUIT_REQUESTED)));
	fFindView->AddChild(fSearchButton = new BButton(BRect(221, buttonTop, 291,
		frame.Height() - 7), "", "Find", new BMessage(MSG_SEARCH)));

	fSearchButton->MakeDefault(true);
	fHandler = _handler;

	const char *text = searchString->String();

    fSearchString->SetText(text);
	fSearchString->MakeFocus(true);

	fCaseSensBox->SetValue(*caseState ? B_CONTROL_ON : B_CONTROL_OFF);
	fWrapBox->SetValue(*wrapState ? B_CONTROL_ON : B_CONTROL_OFF);
	fBackSearchBox->SetValue(*backState ? B_CONTROL_ON : B_CONTROL_OFF);
}


void
FindWindow::MessageReceived(BMessage *msg)
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
FindWindow::DispatchMessage(BMessage *message, BHandler *handler)
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


