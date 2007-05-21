/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "Test.h"


Test::Test(const char* name, const char* description, BView* view)
	: fName(name),
	  fDescription(description),
	  fView(view)
{
}


Test::~Test()
{
}


const char*
Test::Name() const
{
	return fName.String();
}


const char*
Test::Description() const
{
	return fDescription.String();
}


BView*
Test::GetView() const
{
	return fView;
}


void
Test::SetView(BView* view)
{
	fView = view;
}


void
Test::ActivateTest(View* controls)
{
}


void
Test::DectivateTest()
{
}
