/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/

// defines the status area drawn in the bottom left corner of a Tracker window


#include <Button.h>
#include <Screen.h>

#include "OverrideAlert.h"


OverrideAlert::OverrideAlert(const char* title, const char* text,
	const char* button1, uint32 modifiers1,
	const char* button2, uint32 modifiers2,
	const char* button3, uint32 modifiers3,
	button_width width, alert_type type)
	:	BAlert(title, text, button1, button2, button3, width, type),
		fCurModifiers(0)
{
	fButtonModifiers[0] = modifiers1;
	fButtonModifiers[1] = modifiers2;
	fButtonModifiers[2] = modifiers3;
	UpdateButtons(modifiers(), true);
	
	BPoint where = OverPosition(Frame().Width(), Frame().Height());
	MoveTo(where.x, where.y);
}


OverrideAlert::OverrideAlert(const char* title, const char* text,
	const char* button1, uint32 modifiers1,
	const char* button2, uint32 modifiers2,
	const char* button3, uint32 modifiers3,
	button_width width, button_spacing spacing, alert_type type)
	:	BAlert(title, text, button1, button2, button3, width, spacing, type),
		fCurModifiers(0)
{
	fButtonModifiers[0] = modifiers1;
	fButtonModifiers[1] = modifiers2;
	fButtonModifiers[2] = modifiers3;
	UpdateButtons(modifiers(), true);
	
	BPoint where = OverPosition(Frame().Width(), Frame().Height());
	MoveTo(where.x, where.y);
}


OverrideAlert::~OverrideAlert()
{
}


void
OverrideAlert::DispatchMessage(BMessage* message, BHandler* handler)
{
	if (message->what == B_KEY_DOWN || message->what == B_KEY_UP
		|| message->what == B_UNMAPPED_KEY_DOWN
		|| message->what == B_UNMAPPED_KEY_UP) {
		uint32 modifiers;
		if (message->FindInt32("modifiers", (int32*)&modifiers) == B_OK) 
			UpdateButtons(modifiers);
	}
	BAlert::DispatchMessage(message, handler);
}


BPoint
OverrideAlert::OverPosition(float width, float height)
{
	// This positions the alert window like a normal alert, put
	// places it on top of the calling window if possible.
	
	BWindow* window = dynamic_cast<BWindow*>(BLooper::LooperForThread(find_thread(NULL)));
	BRect screenFrame;
	BRect desirableRect;
	screenFrame = BScreen(window).Frame();
	
	if (window) {
		// If we found a window associated with this calling thread,
		// place alert over that window so that the first button is
		// on top of it.  This allows name editing confirmations to
		// work with focus follows mouse -- when the alert goes away,
		// the underlying window will still have focus.
		
		desirableRect = window->Frame();
		float midX = (desirableRect.left + desirableRect.right) / 2.0f;
		float midY = (desirableRect.top * 3.0f + desirableRect.bottom) / 4.0f;
	
		desirableRect.left = midX - ceilf(width / 2.0f);
		desirableRect.right = desirableRect.left+width;
		desirableRect.top = midY - ceilf(height / 3.0f);
		desirableRect.bottom = desirableRect.top + height;
		
	} else {
		// Otherwise, just place alert in center of screen.
		
		desirableRect = screenFrame;
		float midX = (desirableRect.left + desirableRect.right) / 2.0f;
		float midY = (desirableRect.top * 3.0f + desirableRect.bottom) / 4.0f;
	
		desirableRect.left = midX - ceilf(width / 2.0f);
		desirableRect.right = desirableRect.left + width;
		desirableRect.top = midY - ceilf(height / 3.0f);
		desirableRect.bottom = desirableRect.top + height;
	}
	
	return desirableRect.LeftTop();
}


void
OverrideAlert::UpdateButtons(uint32 modifiers, bool force)
{
	if (modifiers == fCurModifiers && !force)
		return;
	
	fCurModifiers = modifiers;
	for (int32 i = 0; i < 3; i++) {
		BButton* button = ButtonAt(i);
		if (button)
			button->SetEnabled(((fButtonModifiers[i] & fCurModifiers) == fButtonModifiers[i]));
	}
}
