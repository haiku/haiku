/*
 * Copyright 2004-2006 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 */

#ifndef KEYMAPTEXTVIEW_H
#define KEYMAPTEXTVIEW_H

#include <TextView.h>

class KeymapTextView : public BTextView
{
public:
	KeymapTextView(BRect frame,
					const char	*name,
					BRect		textRect,
					uint32		resizeMask,
					uint32		flags = B_WILL_DRAW | B_PULSE_NEEDED);
	virtual	void KeyDown(const char *bytes, int32 numBytes);
	virtual	void FakeKeyDown(const char *bytes, int32 numBytes);
};


#endif //KEYMAPTEXTVIEW_H
