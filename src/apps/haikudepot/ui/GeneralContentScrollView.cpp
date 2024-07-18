/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2018-2022, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "GeneralContentScrollView.h"

#include <ControlLook.h>


GeneralContentScrollView::GeneralContentScrollView(
	const char* name, BView* target)
	:
	BScrollView(name, target, 0, false, true, B_NO_BORDER)
{
}


void
GeneralContentScrollView::DoLayout()
{
	float scrollBarWidth = be_control_look->GetScrollBarWidth(B_VERTICAL);
	BRect innerFrame = Bounds();
	innerFrame.right -= scrollBarWidth + 1;

	BView* target = Target();
	if (target != NULL) {
		Target()->MoveTo(innerFrame.left, innerFrame.top);
		Target()->ResizeTo(innerFrame.Width(), innerFrame.Height());
	}

	BScrollBar* scrollBar = ScrollBar(B_VERTICAL);

	if (scrollBar != NULL) {
		BRect rect = innerFrame;
		rect.left = rect.right + 1;
		rect.right = rect.left + scrollBarWidth;

		scrollBar->MoveTo(rect.left, rect.top);
		scrollBar->ResizeTo(rect.Width(), rect.Height());
	}
}
