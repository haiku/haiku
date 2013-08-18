/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "BitmapButton.h"

#include <LayoutUtils.h>
#include <Window.h>


BitmapButton::BitmapButton(const char* name)
	:
	BitmapView(name),
	fMouseInside(false),
	fMouseDown(false)
{
}


BitmapButton::~BitmapButton()
{
}


void
BitmapButton::AttachedToWindow()
{
	// TODO: Init fMouseInside
	BitmapView::AttachedToWindow();
}

void
BitmapButton::MouseMoved(BPoint where, uint32 transit,
	const BMessage* dragMessage)
{
	int32 buttons = 0;
	if (Window() != NULL && Window()->CurrentMessage() != NULL)
		Window()->CurrentMessage()->FindInt32("buttons", &buttons);

	bool ignoreEvent = dragMessage != NULL || (!fMouseDown && buttons != 0);

	_SetMouseInside(!ignoreEvent && transit == B_INSIDE_VIEW);
}


void
BitmapButton::MouseDown(BPoint where)
{
	_SetMouseDown(true);
}


void
BitmapButton::MouseUp(BPoint where)
{
	_SetMouseDown(false);
}


BSize
BitmapButton::MinSize()
{
	BSize size = BitmapView::MinSize();
	return BLayoutUtils::ComposeSize(ExplicitMinSize(), size);
}


BSize
BitmapButton::PreferredSize()
{
	BSize size = MinSize();
	return BLayoutUtils::ComposeSize(ExplicitPreferredSize(), size);
}


BSize
BitmapButton::MaxSize()
{
	BSize size = MinSize();
	return BLayoutUtils::ComposeSize(ExplicitMaxSize(), size);
}


// #pragma mark -


void
BitmapButton::_SetMouseInside(bool inside)
{
	if (fMouseInside == inside)
		return;

	fMouseInside = inside;

	if (Message() != NULL)
		Invalidate();
}


void
BitmapButton::_SetMouseDown(bool down)
{
	if (fMouseDown == down)
		return;

	fMouseDown = down;

	if (Message() != NULL)
		Invalidate();
}
