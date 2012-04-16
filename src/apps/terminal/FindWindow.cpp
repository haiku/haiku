/*
 * Copyright 2007-2010, Haiku, Inc. All rights reserved.
 * Copyright 2003-2004 Kian Duffy, myob@users.sourceforge.net
 * Parts Copyright 1998-1999 Kazuho Okui and Takashi Murai.
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "FindWindow.h"


#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <ControlLook.h>
#include <GridLayoutBuilder.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <Locale.h>
#include <RadioButton.h>
#include <String.h>
#include <TextControl.h>


const uint32 MSG_FIND_HIDE = 'Fhid';
const uint32 TOGGLE_FIND_CONTROL = 'MTFG';
const BRect kWindowFrame(10, 30, 250, 200);

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Terminal FindWindow"

FindWindow::FindWindow(BMessenger messenger, const BString& str,
		bool findSelection, bool matchWord, bool matchCase, bool forwardSearch)
	:
	BWindow(kWindowFrame, B_TRANSLATE("Find"), B_FLOATING_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_CLOSE_ON_ESCAPE
		| B_AUTO_UPDATE_SIZE_LIMITS),
	fFindDlgMessenger(messenger)
{
	SetLayout(new BGroupLayout(B_VERTICAL));

	BBox *separator = new BBox("separator");
	separator->SetExplicitMinSize(BSize(250.0, B_SIZE_UNSET));
	separator->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 1.0));

	BRadioButton* useSelection = NULL;
	const float spacing = be_control_look->DefaultItemSpacing();
	AddChild(BGroupLayoutBuilder(B_VERTICAL, 5.0)
		.SetInsets(spacing, spacing, spacing, spacing)
		.Add(BGridLayoutBuilder()
			.Add(fTextRadio = new BRadioButton(B_TRANSLATE("Use text:"),
				new BMessage(TOGGLE_FIND_CONTROL)), 0, 0)
			.Add(fFindLabel = new BTextControl(NULL, NULL, NULL), 1, 0)
			.Add(useSelection = new BRadioButton(B_TRANSLATE("Use selection"),
				new BMessage(TOGGLE_FIND_CONTROL)), 0, 1))
		.Add(separator)
		.Add(fForwardSearchBox = new BCheckBox(B_TRANSLATE("Search forward")))
		.Add(fMatchCaseBox = new BCheckBox(B_TRANSLATE("Match case")))
		.Add(fMatchWordBox = new BCheckBox(B_TRANSLATE("Match word")))
		.Add(fFindButton = new BButton(B_TRANSLATE("Find"),
				new BMessage(MSG_FIND)))
		.TopView());

	fFindLabel->SetDivider(0.0);

	if (!findSelection) {
		fFindLabel->SetText(str.String());
		fFindLabel->MakeFocus(true);
	} else {
		fFindLabel->SetEnabled(false);
	}

	if (findSelection)
		useSelection->SetValue(B_CONTROL_ON);
	else
		fTextRadio->SetValue(B_CONTROL_ON);

	if (forwardSearch)
		fForwardSearchBox->SetValue(B_CONTROL_ON);

	if (matchCase)
		fMatchCaseBox->SetValue(B_CONTROL_ON);

	if (matchWord)
		fMatchWordBox->SetValue(B_CONTROL_ON);

	fFindButton->MakeDefault(true);

	AddShortcut((uint32)'W', B_COMMAND_KEY, new BMessage(MSG_FIND_HIDE));

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

		case TOGGLE_FIND_CONTROL:
			fFindLabel->SetEnabled(fTextRadio->Value() == B_CONTROL_ON);
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
