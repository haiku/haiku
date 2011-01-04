/*
 * Copyright 2008-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */


#include "DescriptionPage.h"

#include <string.h>

#include <RadioButton.h>
#include <TextView.h>


DescriptionPage::DescriptionPage(BRect frame, const char* name,
	const char* description, bool hasHeading)
	:
	WizardPageView(NULL, frame, name, B_FOLLOW_ALL,
		B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE)
{
	_BuildUI(description, hasHeading);
}


DescriptionPage::~DescriptionPage()
{
}


void
DescriptionPage::FrameResized(float width, float height)
{
	WizardPageView::FrameResized(width, height);
	_Layout();
}


void
DescriptionPage::_BuildUI(const char* description, bool hasHeading)
{
	BRect rect(Bounds());

	fDescription = CreateDescription(rect, "description", description);
	if (hasHeading)
		MakeHeading(fDescription);
	fDescription->SetTabWidth(85);
	AddChild(fDescription);

	_Layout();
}


void
DescriptionPage::_Layout()
{
	LayoutDescriptionVertically(fDescription);
}

