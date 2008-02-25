/*
 * Copyright 2004-2006 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 */

#include "KeymapTextView.h"


KeymapTextView::KeymapTextView(BRect frame,
					const char	*name,
					BRect			textRect,
					uint32		resizeMask,
					uint32		flags) :
		BTextView(frame, name, textRect, resizeMask, flags)
{
	

}


void
KeymapTextView::KeyDown(const char *bytes, int32 numBytes)
{
	

}


void
KeymapTextView::FakeKeyDown(const char *bytes, int32 numBytes)
{
	BTextView::KeyDown(bytes, numBytes);
}
