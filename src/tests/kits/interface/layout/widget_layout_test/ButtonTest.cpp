/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "ButtonTest.h"

#include <stdio.h>

#include <Button.h>
#include <Font.h>
#include <Message.h>

#include "CheckBox.h"
#include "GroupView.h"
#include "StringView.h"


enum {
	MSG_CHANGE_BUTTON_TEXT	= 'chbt',
	MSG_CHANGE_BUTTON_FONT	= 'chbf'
};


// constructor
ButtonTest::ButtonTest()
	: Test("Button", NULL),
	  fButton(new BButton(BRect(0, 0, 9, 9), "test button", "",
		(BMessage*)NULL, B_FOLLOW_NONE)),
	  fLongTextCheckBox(NULL),
	  fBigFontCheckBox(NULL),
	  fDefaultFont(NULL),
	  fBigFont(NULL)

{
	_SetButtonText(false);
	SetView(fButton);
}


// destructor
ButtonTest::~ButtonTest()
{
	delete fDefaultFont;
	delete fBigFont;
}


// ActivateTest
void
ButtonTest::ActivateTest(View* controls)
{
	GroupView* group = new GroupView(B_VERTICAL);
	group->SetFrame(controls->Bounds());
	group->SetSpacing(0, 8);
	controls->AddChild(group);

	// long button text
	fLongTextCheckBox = new LabeledCheckBox("Long Button Text",
		new BMessage(MSG_CHANGE_BUTTON_TEXT), this);
	group->AddChild(fLongTextCheckBox);

	// big font
	fBigFontCheckBox = new LabeledCheckBox("Big Button Font",
		new BMessage(MSG_CHANGE_BUTTON_FONT), this);
	group->AddChild(fBigFontCheckBox);

	group->AddChild(new Glue());

	_SetButtonText(false);
	_SetButtonFont(false);
}


// DectivateTest
void
ButtonTest::DectivateTest()
{
}


// MessageReceived
void
ButtonTest::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_CHANGE_BUTTON_TEXT:
			_SetButtonText(fLongTextCheckBox->IsSelected());
			break;
		case MSG_CHANGE_BUTTON_FONT:
			_SetButtonFont(fBigFontCheckBox->IsSelected());
			break;
		default:
			Test::MessageReceived(message);
			break;
	}
}


// _SetButtonText
void
ButtonTest::_SetButtonText(bool longText)
{
	if (fLongTextCheckBox)
		fLongTextCheckBox->SetSelected(longText);
	fButton->SetLabel(longText ? "very long text for a simple button"
		: "Ooh, press me!");
}


// _SetButtonFont
void
ButtonTest::_SetButtonFont(bool bigFont)
{
	if (fBigFontCheckBox)
		fBigFontCheckBox->SetSelected(bigFont);
	if (fButton->Window()) {
		// get default font lazily
		if (!fDefaultFont) {
			fDefaultFont = new BFont;
			fButton->GetFont(fDefaultFont);

			fBigFont = new BFont(fDefaultFont);
			fBigFont->SetSize(20);
		}

		// set font
		fButton->SetFont(bigFont ? fBigFont : fDefaultFont);
		fButton->InvalidateLayout();
		fButton->Invalidate();
	}
}
