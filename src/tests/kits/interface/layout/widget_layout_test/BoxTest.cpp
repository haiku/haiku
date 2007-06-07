/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "BoxTest.h"

#include <stdio.h>

#include <Box.h>
#include <Message.h>

#include "GroupView.h"
#include "RadioButton.h"


// messages
enum {
	MSG_BORDER_STYLE_CHANGED	= 'bstc',
};


// BorderStyleRadioButton
class BoxTest::BorderStyleRadioButton : public LabeledRadioButton {
public:
	BorderStyleRadioButton(const char* label, border_style style)
		: LabeledRadioButton(label),
		  fBorderStyle(style)
	{
	}

	border_style	fBorderStyle;
};


// constructor
BoxTest::BoxTest()
	: Test("Box", NULL),
	  fBox(new BBox(BRect(0, 0, 9, 9), "test box", B_FOLLOW_NONE)),
	  fBorderStyleRadioGroup(NULL)
// TODO: Layout-friendly constructor
{
	SetView(fBox);
}


// destructor
BoxTest::~BoxTest()
{
	delete fBorderStyleRadioGroup;
}


// ActivateTest
void
BoxTest::ActivateTest(View* controls)
{
	// BBox sets its background color to that of its parent in
	// AttachedToWindow(). Override.
	rgb_color background = ui_color(B_PANEL_BACKGROUND_COLOR);
	fBox->SetViewColor(background);
	fBox->SetLowColor(background);

	GroupView* group = new GroupView(B_VERTICAL);
	group->SetFrame(controls->Bounds());
	group->SetSpacing(0, 8);
	controls->AddChild(group);

	// the radio button group for selecting the border style
	fBorderStyleRadioGroup = new RadioButtonGroup(
		new BMessage(MSG_BORDER_STYLE_CHANGED), this);

	// no border
	LabeledRadioButton* button = new BorderStyleRadioButton("no border",
		B_NO_BORDER);
	group->AddChild(button);
	fBorderStyleRadioGroup->AddButton(button->GetRadioButton());

	// plain border
	button = new BorderStyleRadioButton("plain border", B_PLAIN_BORDER);
	group->AddChild(button);
	fBorderStyleRadioGroup->AddButton(button->GetRadioButton());

	// fancy border
	button = new BorderStyleRadioButton("fancy border", B_FANCY_BORDER);
	group->AddChild(button);
	fBorderStyleRadioGroup->AddButton(button->GetRadioButton());

	// default to no border
	fBorderStyleRadioGroup->SelectButton(0L);

	group->AddChild(new Glue());
}


// DectivateTest
void
BoxTest::DectivateTest()
{
}


// MessageReceived
void
BoxTest::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_BORDER_STYLE_CHANGED:
			_UpdateBorderStyle();
			break;
		default:
			Test::MessageReceived(message);
			break;
	}
}


// _UpdateBorderStyle
void
BoxTest::_UpdateBorderStyle()
{
	if (fBorderStyleRadioGroup) {
		// We need to get the parent of the actually selected button, since
		// that is the labeled radio button we've derived our
		// BorderStyleRadioButton from.
		AbstractButton* selectedButton
			= fBorderStyleRadioGroup->SelectedButton();
		View* parent = (selectedButton ? selectedButton->Parent() : NULL);
		BorderStyleRadioButton* button = dynamic_cast<BorderStyleRadioButton*>(
			parent);
		if (button)
			fBox->SetBorder(button->fBorderStyle);
	}
}
