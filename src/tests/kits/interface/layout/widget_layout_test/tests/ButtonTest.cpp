/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "ButtonTest.h"

#include <stdio.h>

#include <Button.h>

#include "GroupView.h"


// constructor
ButtonTest::ButtonTest()
	: ControlTest("Button"),
	  fButton(new BButton(""))
{
	SetView(fButton);
}


// destructor
ButtonTest::~ButtonTest()
{
}


// CreateTest
Test*
ButtonTest::CreateTest()
{
	return new ButtonTest;
}


// ActivateTest
void
ButtonTest::ActivateTest(View* controls)
{
	GroupView* group = new GroupView(B_VERTICAL);
	group->SetFrame(controls->Bounds());
	controls->AddChild(group);

	ControlTest::ActivateTest(group);

	group->AddChild(new Glue());
}


// DectivateTest
void
ButtonTest::DectivateTest()
{
}
