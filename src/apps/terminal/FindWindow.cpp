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
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <RadioButton.h>
#include <String.h>
#include <TextControl.h>


const uint32 MSG_FIND_HIDE = 'Fhid';

const BRect kWindowFrame(0, 0, 240, 170);

FindWindow::FindWindow(BMessenger messenger, BString &str,
		bool findSelection, bool matchWord, bool matchCase,
		bool forwardSearch)
	:
	BWindow(kWindowFrame, "Find", B_FLOATING_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_CLOSE_ON_ESCAPE
		| B_AUTO_UPDATE_SIZE_LIMITS),
	fFindDlgMessenger(messenger)
{
	SetLayout(new BGroupLayout(B_VERTICAL));
	
	BBox *separator = new BBox("separator");
	separator->SetExplicitMinSize(BSize(200, B_SIZE_UNSET));
	separator->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 1));

	BView *layoutView = BGroupLayoutBuilder(B_VERTICAL, 10)
		.SetInsets(5, 5, 5, 5)
		.Add(fTextRadio = new BRadioButton("fTextRadio", "Use Text: ",
			NULL))
		.Add(fFindLabel = new BTextControl("fFindLabel", "", "", NULL))
		.Add(fSelectionRadio = new BRadioButton("fSelectionRadio",
			"Use Selection", NULL))
		.Add(separator)
		.Add(fForwardSearchBox = new BCheckBox("fForwardSearchBox",
			"Search Forward", NULL))
		.Add(fMatchCaseBox = new BCheckBox("fMatchCaseBox",
			"Match Case", NULL))
		.Add(fMatchWordBox = new BCheckBox("fMatchWordBox",
			"Match Word", NULL))
		.Add(fFindButton = new BButton("fFindButton", "Find",
			new BMessage(MSG_FIND)))
		.End();
	
	AddChild(layoutView);	
	
	layoutView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	fFindLabel->SetDivider(0);
	
	if (!findSelection)
		fFindLabel->SetText(str.String());
	fFindLabel->MakeFocus(true);
	
	if (findSelection)
		fSelectionRadio->SetValue(B_CONTROL_ON);
	else
		fTextRadio->SetValue(B_CONTROL_ON);

	if (forwardSearch)
		fForwardSearchBox->SetValue(B_CONTROL_ON);

	if (matchCase)
		fMatchCaseBox->SetValue(B_CONTROL_ON);
	
	if (matchWord)
		fMatchWordBox->SetValue(B_CONTROL_ON);

	fFindButton->MakeDefault(true);

	AddShortcut((ulong)'W', (ulong)B_COMMAND_KEY,
		new BMessage(MSG_FIND_HIDE));
	
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
	// TODO: "usetext" is never checked for elsewhere and seems
	// redundant with "findselection", why is it here?
	message.AddBool("usetext", fTextRadio->Value() == B_CONTROL_ON);
	message.AddBool("forwardsearch", fForwardSearchBox->Value() == B_CONTROL_ON);
	message.AddBool("matchcase", fMatchCaseBox->Value() == B_CONTROL_ON);
	message.AddBool("matchword", fMatchWordBox->Value() == B_CONTROL_ON);

	fFindDlgMessenger.SendMessage(&message);
}
