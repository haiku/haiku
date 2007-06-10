/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "ControlTest.h"

#include <stdio.h>

#include <Control.h>
#include <Font.h>
#include <Message.h>

#include "CheckBox.h"
#include "GroupView.h"
#include "StringView.h"



// constructor
ControlTest::ControlTest(const char* name)
	: Test(name, NULL),
	  fControl(NULL),
	  fLongTextCheckBox(NULL),
	  fBigFontCheckBox(NULL),
	  fDefaultFont(NULL),
	  fBigFont(NULL)
{
}


// destructor
ControlTest::~ControlTest()
{
	delete fDefaultFont;
	delete fBigFont;
}


// SetView
void
ControlTest::SetView(BView* view)
{
	Test::SetView(view);
	fControl = dynamic_cast<BControl*>(view);
}


// ActivateTest
void
ControlTest::ActivateTest(View* controls)
{
	GroupView* group = new GroupView(B_VERTICAL);
	group->SetFrame(controls->Bounds());
	group->SetSpacing(0, 8);
	controls->AddChild(group);

	// long text
	fLongTextCheckBox = new LabeledCheckBox("Long label text",
		new BMessage(MSG_CHANGE_CONTROL_TEXT), this);
	group->AddChild(fLongTextCheckBox);

	// big font
	fBigFontCheckBox = new LabeledCheckBox("Big label font",
		new BMessage(MSG_CHANGE_CONTROL_FONT), this);
	group->AddChild(fBigFontCheckBox);

	UpdateControlText();
	UpdateControlFont();
}


// DectivateTest
void
ControlTest::DectivateTest()
{
}


// MessageReceived
void
ControlTest::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_CHANGE_CONTROL_TEXT:
			UpdateControlText();
			break;
		case MSG_CHANGE_CONTROL_FONT:
			UpdateControlFont();
			break;
		default:
			Test::MessageReceived(message);
			break;
	}
}


// UpdateControlText
void
ControlTest::UpdateControlText()
{
	if (!fLongTextCheckBox || !fControl)
		return;

	fControl->SetLabel(fLongTextCheckBox->IsSelected()
		? "Very long label for a simple control"
		: "Short label");
}


// UpdateControlFont
void
ControlTest::UpdateControlFont()
{
	if (!fBigFontCheckBox || !fControl || !fControl->Window())
		return;

	// get default font lazily
	if (!fDefaultFont) {
		fDefaultFont = new BFont;
		fControl->GetFont(fDefaultFont);

		fBigFont = new BFont(fDefaultFont);
		fBigFont->SetSize(20);
	}

	// set font
	fControl->SetFont(fBigFontCheckBox->IsSelected()
		? fBigFont : fDefaultFont);
	fControl->InvalidateLayout();
	fControl->Invalidate();
}
