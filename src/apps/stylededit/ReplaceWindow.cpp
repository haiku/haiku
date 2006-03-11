/*
 * Copyright 2002-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mattias Sundblad
 *		Andrew Bachmann
 */


#include "Constants.h"
#include "ReplaceWindow.h"

#include <Button.h>
#include <CheckBox.h>
#include <String.h>
#include <TextControl.h>
#include <Window.h>


ReplaceWindow::ReplaceWindow(BRect frame, BHandler *_handler, BString *searchString,
	BString *replaceString, bool *caseState, bool *wrapState, bool *backState)
	: BWindow(frame, "ReplaceWindow", B_MODAL_WINDOW,
		B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS,
		B_CURRENT_WORKSPACE) 
{
	fReplaceView = new BView(Bounds(), "ReplaceView", B_FOLLOW_ALL, B_WILL_DRAW);
	fReplaceView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(fReplaceView);

	char* findLabel = "Find:";
	float findWidth = fReplaceView->StringWidth(findLabel);
	fReplaceView->AddChild(fSearchString = new BTextControl(BRect(5, 10, 290, 50), "",
		findLabel, NULL, NULL, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE));
	
	char* replaceWithLabel = "Replace with:";
	float replaceWithWidth = fReplaceView->StringWidth(replaceWithLabel);
	fReplaceView->AddChild(fReplaceString = new BTextControl(BRect(5, 35, 290, 50), "",
		replaceWithLabel, NULL, NULL, B_FOLLOW_LEFT | B_FOLLOW_TOP,
		B_WILL_DRAW | B_NAVIGABLE));
	float maxWidth = (replaceWithWidth > findWidth ? replaceWithWidth : findWidth) + 1;
	fSearchString->SetDivider(maxWidth);
	fReplaceString->SetDivider(maxWidth);

	fReplaceView->AddChild(fCaseSensBox = new BCheckBox(BRect(maxWidth + 8, 60, 290, 52),
		"", "Case-sensitive", NULL, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE));
	fReplaceView->AddChild(fWrapBox = new BCheckBox(BRect(maxWidth + 8, 80, 290, 70),
		"", "Wrap-around search", NULL, B_FOLLOW_LEFT | B_FOLLOW_TOP,
		B_WILL_DRAW | B_NAVIGABLE));
	fReplaceView->AddChild(fBackSearchBox = new BCheckBox(BRect(maxWidth + 8, 100, 290, 95),
		"", "Search backwards", NULL, B_FOLLOW_LEFT | B_FOLLOW_TOP,
		B_WILL_DRAW | B_NAVIGABLE)); 
	fReplaceView->AddChild(fAllWindowsBox = new BCheckBox(BRect(maxWidth + 8, 120, 290, 95),
		"", "Replace in all windows", new BMessage(CHANGE_WINDOW),
		B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE));
	fUIchange = false;

	fReplaceView->AddChild(fReplaceAllButton = new BButton(BRect(10, 150, 98, 166),
		"", "Replace All", new BMessage(MSG_REPLACE_ALL), B_FOLLOW_LEFT | B_FOLLOW_TOP,
		B_WILL_DRAW | B_NAVIGABLE));
	fReplaceView->AddChild(fCancelButton = new BButton(BRect(141, 150, 211, 166),
		"", "Cancel", new BMessage(B_QUIT_REQUESTED), B_FOLLOW_LEFT | B_FOLLOW_TOP,
		B_WILL_DRAW | B_NAVIGABLE));
	fReplaceView->AddChild(fReplaceButton = new BButton(BRect(221, 150, 291, 166),
		"", "Replace", new BMessage(MSG_REPLACE), B_FOLLOW_LEFT | B_FOLLOW_TOP,
		B_WILL_DRAW | B_NAVIGABLE));
	fReplaceButton->MakeDefault(true);

	fHandler = _handler;

	const char *searchtext = searchString->String();
	const char *replacetext = replaceString->String(); 

	fSearchString->SetText(searchtext);
	fReplaceString->SetText(replacetext);
	fSearchString->MakeFocus(true);

	fCaseSensBox->SetValue(*caseState ? B_CONTROL_ON : B_CONTROL_OFF);
	fWrapBox->SetValue(*wrapState ? B_CONTROL_ON : B_CONTROL_OFF);
	fBackSearchBox->SetValue(*backState ? B_CONTROL_ON : B_CONTROL_OFF);
}


void
ReplaceWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what){
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
	if (!fUIchange) {
		fReplaceAllButton->MakeDefault(true);
		fReplaceButton->SetEnabled(false);
		fWrapBox->SetValue(B_CONTROL_ON);
		fWrapBox->SetEnabled(false);
		fBackSearchBox->SetEnabled(false);
		fUIchange = true;
	} else {
		fReplaceButton->MakeDefault(true);
		fReplaceButton->SetEnabled(true);
		fReplaceAllButton->SetEnabled(true);
		fWrapBox->SetValue(B_CONTROL_OFF);
		fWrapBox->SetEnabled(true);
		fBackSearchBox->SetEnabled(true);
		fUIchange = false;
	}
}


void
ReplaceWindow::DispatchMessage(BMessage *message, BHandler *handler)
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

