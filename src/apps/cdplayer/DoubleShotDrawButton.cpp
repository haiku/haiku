/*
 * Copyright (c) 2006-2007, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 */
#include "DoubleShotDrawButton.h"
#include <stdio.h>

// The only difference between this class and DrawButton is the fact that it is
// invoked twice during a mousedown-mouseup cycle. It fires when pushed, and
// then again when released.

DoubleShotDrawButton::DoubleShotDrawButton(BRect frame, const char *name,
	BBitmap *up, BBitmap *down,BMessage *msg, int32 resize, int32 flags)
	: DrawButton(frame, name, up,down, msg, resize, flags)
{
}


void
DoubleShotDrawButton::MouseDown(BPoint point)
{
	Invoke();
	DrawButton::MouseDown(point);
}
