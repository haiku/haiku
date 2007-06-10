/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "CheckBoxTest.h"

#include <stdio.h>

#include <CheckBox.h>

#include "GroupView.h"


// constructor
CheckBoxTest::CheckBoxTest()
	: ControlTest("CheckBox"),
	  fCheckBox(new BCheckBox(""))
{
	SetView(fCheckBox);
}


// destructor
CheckBoxTest::~CheckBoxTest()
{
}


// CreateTest
Test*
CheckBoxTest::CreateTest()
{
	return new CheckBoxTest;
}


// ActivateTest
void
CheckBoxTest::ActivateTest(View* controls)
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
CheckBoxTest::DectivateTest()
{
}
