/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "ButtonTest.h"

#include <stdio.h>

#include <Button.h>
#include <Message.h>

#include "CheckBox.h"
#include "GroupView.h"
#include "StringView.h"


enum {
	MSG_CHANGE_BUTTON_TEXT	= 'lgbt'
};


// constructor
ButtonTest::ButtonTest()
	: Test("Button", NULL),
	  fButton(new BButton(BRect(0, 0, 9, 9), "test button", "",
		(BMessage*)NULL, B_FOLLOW_NONE)),
	  fLongTextCheckBox(NULL)

{
	_SetButtonText(false);
	SetView(fButton);
}


// destructor
ButtonTest::~ButtonTest()
{
}


// ActivateTest
void
ButtonTest::ActivateTest(View* controls)
{
	GroupView* group = new GroupView(B_VERTICAL);
	group->SetFrame(controls->Bounds());
	controls->AddChild(group);

	GroupView* longTextGroup = new GroupView(B_HORIZONTAL);
	group->AddChild(longTextGroup);
	fLongTextCheckBox = new CheckBox(new BMessage(MSG_CHANGE_BUTTON_TEXT),
		this);
	longTextGroup->AddChild(fLongTextCheckBox);
	longTextGroup->AddChild(new StringView("  Long Button Text"));

	group->AddChild(new Glue());

	_SetButtonText(false);
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
