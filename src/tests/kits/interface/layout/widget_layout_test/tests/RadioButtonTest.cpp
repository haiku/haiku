/*
 * Copyright 2008, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "RadioButtonTest.h"

#include <stdio.h>

#include <RadioButton.h>

#include "GroupView.h"


// constructor
RadioButtonTest::RadioButtonTest()
	: ControlTest("RadioButton"),
	  fRadioButton(new BRadioButton("", NULL))
{
	SetView(fRadioButton);
}


// destructor
RadioButtonTest::~RadioButtonTest()
{
}


// CreateTest
Test*
RadioButtonTest::CreateTest()
{
	return new RadioButtonTest;
}


// ActivateTest
void
RadioButtonTest::ActivateTest(View* controls)
{
	// BControl sets its background color to that of its parent in
	// AttachedToWindow(). Override.
	rgb_color background = ui_color(B_PANEL_BACKGROUND_COLOR);
	fControl->SetViewColor(background);
	fControl->SetLowColor(background);

	GroupView* group = new GroupView(B_VERTICAL);
	group->SetFrame(controls->Bounds());
	controls->AddChild(group);

	ControlTest::ActivateTest(group);

	group->AddChild(new Glue());
}


// DectivateTest
void
RadioButtonTest::DectivateTest()
{
}
