/*
 * Copyright 1999-2009 Jeremy Friesner
 * Copyright 2009-2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 *		John Scipione, jscipione@gmail.com
 */


#include "Colors.h"
#include "ScrollViewCorner.h"

#include <InterfaceKit.h>


ScrollViewCorner::ScrollViewCorner(float left, float top)
	:
	BView(BRect(left, top, left + B_V_SCROLL_BAR_WIDTH,
		top + B_H_SCROLL_BAR_HEIGHT), NULL,
		B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM, B_WILL_DRAW)
{
	SetHighColor(BeShadow);
	SetViewColor(BeInactiveGrey);
}


ScrollViewCorner::~ScrollViewCorner()
{
}


void
ScrollViewCorner::Draw(BRect updateRect)
{
	if (updateRect.bottom >= B_H_SCROLL_BAR_HEIGHT) {
		StrokeLine(BPoint(0.0, B_H_SCROLL_BAR_HEIGHT),
			BPoint(B_V_SCROLL_BAR_WIDTH, B_H_SCROLL_BAR_HEIGHT));
	}
	if (updateRect.right >= B_V_SCROLL_BAR_WIDTH) {
		StrokeLine(BPoint(B_V_SCROLL_BAR_WIDTH, 0.0),
			BPoint(B_V_SCROLL_BAR_WIDTH, B_H_SCROLL_BAR_HEIGHT - 1.0));
	}
}
