// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2004, Haiku
//
//  This software is part of the Haiku distribution and is covered 
//  by the Haiku license.
//
//
//  File:        KeymapTextView.cpp
//  Author:      Jérôme Duval
//  Description: Keymap Preferences
//  Created :    July 14, 2004
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#include "KeymapTextView.h"


KeymapTextView::KeymapTextView(BRect frame,
					const char	*name,
					BRect			textRect,
					uint32		resizeMask,
					uint32		flags = B_WILL_DRAW | B_PULSE_NEEDED) :
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
