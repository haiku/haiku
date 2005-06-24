// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2004, Haiku
//
//  This software is part of the Haiku distribution and is covered 
//  by the Haiku license.
//
//
//  File:        KeymapTextView.h
//  Author:      Jérôme Duval
//  Description: Keymap Preferences
//  Created :    July 14, 2004
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#ifndef KEYMAPTEXTVIEW_H
#define KEYMAPTEXTVIEW_H

#include <TextView.h>

class KeymapTextView : public BTextView
{
public:
	KeymapTextView(BRect frame,
					const char	*name,
					BRect			textRect,
					uint32		resizeMask,
					uint32		flags = B_WILL_DRAW | B_PULSE_NEEDED);
	virtual	void KeyDown(const char *bytes, int32 numBytes);
	virtual	void FakeKeyDown(const char *bytes, int32 numBytes);
};


#endif //KEYMAPTEXTVIEW_H
