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
#ifndef _OVERRIDE_ALERT_H
#define _OVERRIDE_ALERT_H

// This is a special BAlert for which you can specify modifier
// keys that must be down for various buttons to be enabled.  It
// is used when confirming changes in the BeOS directory to force
// the user to hold shift when confirming.

// The alert also positions itself slightly differently than a
// normal BAlert, attempting to be on top of the calling window.
// This allows it to work when confirming rename operations with
// Focus Follows Mouse turned on.


#include <Alert.h>


namespace BPrivate {

class OverrideAlert : public BAlert {
public:
	OverrideAlert(const char* title, const char* text,
		const char* button1, uint32 modifiers1,
		const char* button2, uint32 modifiers2,
		const char* button3, uint32 modifiers3,
		button_width width = B_WIDTH_AS_USUAL,
		alert_type type = B_INFO_ALERT);
	OverrideAlert(const char* title, const char* text,
		const char* button1, uint32 modifiers1,
		const char* button2, uint32 modifiers2,
		const char* button3, uint32 modifiers3,
		button_width width, button_spacing spacing,
		alert_type type = B_INFO_ALERT);
	virtual ~OverrideAlert();

	virtual void DispatchMessage(BMessage*, BHandler*);

	static BPoint OverPosition(float width, float height);

private:
	void UpdateButtons(uint32 modifiers, bool force = false);

	uint32 fCurModifiers;
	uint32 fButtonModifiers[3];
};

} // namespace BPrivate

using namespace BPrivate;

#endif	// _OVERRIDE_ALERT_H
