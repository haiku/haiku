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

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered
trademarks of Be Incorporated in the United States and other countries. Other
brand product names are registered trademarks or trademarks of their respective
holders.

All rights reserved.
*/


#include "TruncatableMenuItem.h"

#include <StackOrHeapArray.h>

#include <stdlib.h>

#include "BarApp.h"
#include "BarView.h"
#include "TeamMenuItem.h"


const float kVPad = 2.0f;


//	#pragma mark - TTruncatableMenuItem


TTruncatableMenuItem::TTruncatableMenuItem(const char* label, BMessage* message,
	char shortcut, uint32 modifiers)
	:
	BMenuItem(label, message, shortcut, modifiers)
{
	fTruncatedString = label;
}


TTruncatableMenuItem::TTruncatableMenuItem(BMenu* menu, BMessage* message)
	:
	BMenuItem(menu, message)
{
}


TTruncatableMenuItem::TTruncatableMenuItem(BMessage* data)
	:
	BMenuItem(data)
{
}


TTruncatableMenuItem::~TTruncatableMenuItem()
{
}


const char*
TTruncatableMenuItem::Label()
{
	return BMenuItem::Label();
}


const char*
TTruncatableMenuItem::Label(float width)
{
	BMenu* menu = Menu();

	float maxWidth = menu->MaxContentWidth() - kVPad * 2;
	if (dynamic_cast<TTeamMenuItem*>(this) != NULL
		&& static_cast<TBarApp*>(be_app)->BarView()->Vertical()
		&& static_cast<TBarApp*>(be_app)->Settings()->superExpando) {
		maxWidth -= kSwitchWidth;
	}

	const char* label = Label();
	if (width > 0 && maxWidth > 0 && width > maxWidth) {
		BStackOrHeapArray<char, 128> truncatedLabel(strlen(label) + 4);
		if (truncatedLabel.IsValid()) {
			float labelWidth = menu->StringWidth(label);
			float offset = width - labelWidth;
			TruncateLabel(maxWidth - offset, truncatedLabel);
			fTruncatedString.SetTo(truncatedLabel);
			return fTruncatedString.String();
		}
	}

	fTruncatedString.SetTo(label);

	return label;
}


void
TTruncatableMenuItem::SetLabel(const char* label)
{
	fTruncatedString.SetTo(label);

	BMenuItem::SetLabel(label);
}
