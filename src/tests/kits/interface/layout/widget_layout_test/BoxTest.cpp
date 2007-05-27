/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "BoxTest.h"

#include <stdio.h>

#include <Box.h>
#include <Message.h>


// constructor
BoxTest::BoxTest()
	: Test("Box", NULL),
	  fBox(new BBox(BRect(0, 0, 9, 9), "test box", B_FOLLOW_NONE))
// TODO: Layout-friendly constructor
{
	SetView(fBox);
}


// destructor
BoxTest::~BoxTest()
{
}


// ActivateTest
void
BoxTest::ActivateTest(View* controls)
{
/*	GroupView* group = new GroupView(B_VERTICAL);
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
*/
}


// DectivateTest
void
BoxTest::DectivateTest()
{
}
