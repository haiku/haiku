/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "TestView.h"


TestView::TestView(BSize minSize, BSize maxSize, BSize preferredSize)
	: BView("test view", B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
	  fMinSize(minSize),
	  fMaxSize(maxSize),
	  fPreferredSize(preferredSize)
{
	SetViewColor((rgb_color){150, 220, 150, 255});
	SetHighColor((rgb_color){0, 80, 0, 255});
}


BSize
TestView::MinSize()
{
	return fMinSize;
}


BSize
TestView::MaxSize()
{
	return fMaxSize;
}


BSize
TestView::PreferredSize()
{
	return fPreferredSize;
}


void
TestView::Draw(BRect updateRect)
{
	BRect bounds(Bounds());
	StrokeRect(bounds);
	StrokeLine(bounds.LeftTop(), bounds.RightBottom());
	StrokeLine(bounds.LeftBottom(), bounds.RightTop());
}
