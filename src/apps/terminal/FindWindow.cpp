/*
 * Copyright 2007, Haiku, Inc.
 * Copyright 2003-2004 Kian Duffy, myob@users.sourceforge.net
 * Parts Copyright 1998-1999 Kazuho Okui and Takashi Murai. 
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "FindWindow.h"

#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <RadioButton.h>
#include <String.h>
#include <TextControl.h>


const uint32 MSG_FIND_HIDE = 'Fhid';


FindWindow::FindWindow (BRect frame, BMessenger messenger , BString &str, 
	bool findSelection, bool matchWord, bool matchCase, bool forwardSearch)
	: BWindow(frame, "Find", B_FLOATING_WINDOW, B_NOT_RESIZABLE|B_NOT_ZOOMABLE),
	fFindDlgMessenger(messenger)
{
	AddShortcut((ulong)'W', (ulong)B_COMMAND_KEY, new BMessage(MSG_FIND_HIDE));

	//Build up view
	fFindView = new BView(Bounds(), "FindView", B_FOLLOW_ALL, B_WILL_DRAW);
	fFindView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(fFindView);

	font_height height;
	fFindView->GetFontHeight(&height);
	float lineHeight = height.ascent + height.descent + height.leading;

	//These labels are from the bottom up
	float buttonsTop = frame.Height() - 19 - lineHeight;
	float matchWordBottom = buttonsTop - 4;
	float matchWordTop = matchWordBottom - lineHeight - 8;
	float matchCaseBottom = matchWordTop - 4;
	float matchCaseTop = matchCaseBottom - lineHeight - 8;
	float forwardSearchBottom = matchCaseTop - 4;
	float forwardSearchTop = forwardSearchBottom - lineHeight - 8;

	//These things are calculated from the top
	float textRadioTop = 12;
	float textRadioBottom = textRadioTop + 2 + lineHeight + 2 + 1;
	float textRadioRight = fFindView->StringWidth("Use Text: ") + 30;
	float selectionRadioTop = textRadioBottom + 4;
	float selectionRadioBottom = selectionRadioTop + lineHeight + 8;

	//Divider
	float dividerHeight = (selectionRadioBottom + forwardSearchTop) / 2;
	
	//Button Coordinates
	float searchButtonLeft = (frame.Width() - fFindView->StringWidth("Find") - 60) / 2;
	float searchButtonRight = searchButtonLeft + fFindView->StringWidth("Find") + 60; 
	
	//Build the Views
	fTextRadio = new BRadioButton(BRect(14, textRadioTop, textRadioRight, textRadioBottom),
		"fTextRadio", "Use Text: ", NULL);
	fFindView->AddChild(fTextRadio);
	
	fFindLabel = new BTextControl(BRect(textRadioRight + 4, textRadioTop, frame.Width() - 14, textRadioBottom),
		"fFindLabel", "", "", NULL);
	fFindLabel->SetDivider(0);
	fFindView->AddChild(fFindLabel);
	if (!findSelection)
		fFindLabel->SetText(str.String());
	fFindLabel->MakeFocus(true);

	fSelectionRadio = new BRadioButton(BRect(14, selectionRadioTop, frame.Width() - 14, selectionRadioBottom),
		"fSelectionRadio", "Use Selection", NULL);
	fFindView->AddChild(fSelectionRadio);
	
	if (findSelection)
		fSelectionRadio->SetValue(B_CONTROL_ON);
	else
		fTextRadio->SetValue(B_CONTROL_ON);

	fSeparator = new BBox(BRect(6, dividerHeight, frame.Width() - 6, dividerHeight + 1));
	fFindView->AddChild(fSeparator);
	
	fForwardSearchBox = new BCheckBox(BRect(14, forwardSearchTop, frame.Width() - 14, forwardSearchBottom),
		"fForwardSearchBox", "Search Forward", NULL);
	fFindView->AddChild(fForwardSearchBox);
	if (forwardSearch)
		fForwardSearchBox->SetValue(B_CONTROL_ON);
	
	fMatchCaseBox = new BCheckBox(BRect(14, matchCaseTop, frame.Width() - 14, matchCaseBottom),
		"fMatchCaseBox", "Match Case", NULL);
	fFindView->AddChild(fMatchCaseBox);
	if (matchCase)
		fMatchCaseBox->SetValue(B_CONTROL_ON);
	
	fMatchWordBox = new BCheckBox(BRect(14, matchWordTop, frame.Width() - 14, matchWordBottom),
		"fMatchWordBox", "Match Word", NULL);
	fFindView->AddChild(fMatchWordBox);
	if (matchWord)
		fMatchWordBox->SetValue(B_CONTROL_ON);
	
	fFindButton = new BButton(BRect(searchButtonLeft, buttonsTop, searchButtonRight, frame.Height() - 14),
		"fFindButton", "Find", new BMessage(MSG_FIND));
	fFindButton->MakeDefault(true);
	fFindView->AddChild(fFindButton);
	
	Show();
}


FindWindow::~FindWindow()
{
}


void
FindWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case B_QUIT_REQUESTED:
			Quit();
			break;

		case MSG_FIND:
			_SendFindMessage();
			break;

		case MSG_FIND_HIDE:
			Quit();
			break;

		default:
			BWindow::MessageReceived(msg);
			break;
	}
}


void
FindWindow::Quit()
{
	fFindDlgMessenger.SendMessage(MSG_FIND_CLOSED);
	BWindow::Quit();
}


void
FindWindow::_SendFindMessage()
{
	BMessage message(MSG_FIND);

	if (fTextRadio->Value() == B_CONTROL_ON) {
		message.AddString("findstring", fFindLabel->Text());
		message.AddBool("findselection", false);
	} else
		message.AddBool("findselection", true);
	
	//Add the other parameters
	message.AddBool("usetext", fTextRadio->Value() == B_CONTROL_ON);
	message.AddBool("forwardsearch", fForwardSearchBox->Value() == B_CONTROL_ON);
	message.AddBool("matchcase", fMatchCaseBox->Value() == B_CONTROL_ON);
	message.AddBool("matchword", fMatchWordBox->Value() == B_CONTROL_ON);
	
	fFindDlgMessenger.SendMessage(&message);
}
