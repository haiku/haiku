/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */

#include "PopupView.h"

#include "PopupWindow.h"

// constructor
PopupView::PopupView(const char* name)
	: BView(BRect(0.0, 0.0, 10.0, 10.0), name, 
			B_FOLLOW_NONE, B_WILL_DRAW),
	  fWindow(NULL)
{
}

// destructor
PopupView::~PopupView()
{
}

// SetPopupWindow
void
PopupView::SetPopupWindow(PopupWindow* window)
{
	fWindow = window;
	if (fWindow)
		SetEventMask(B_POINTER_EVENTS);
	else
		SetEventMask(0);
}

// PopupDown
void
PopupView::PopupDone(bool canceled)
{
	if (fWindow)
		fWindow->PopupDone(canceled);
}

