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


#include <PopUpMenu.h>
#include <Window.h>

#include "MiniMenuField.h"
#include "Utilities.h"


MiniMenuField::MiniMenuField(BRect frame, const char* name, BPopUpMenu* menu,
	uint32 resizeFlags, uint32 flags)
	:	BView(frame, name, resizeFlags, flags),
		fMenu(menu)
{
	SetFont(be_plain_font, B_FONT_FAMILY_AND_STYLE | B_FONT_SIZE);
}


MiniMenuField::~MiniMenuField()
{
	delete fMenu;
}


void
MiniMenuField::AttachedToWindow()
{
	if (Parent()) {
		SetViewColor(Parent()->ViewColor());
		SetLowColor(Parent()->ViewColor());
	}
	SetHighColor(0, 0, 0);
}


void
MiniMenuField::MakeFocus(bool on)
{
	Invalidate();
	BView::MakeFocus(on);
}


void
MiniMenuField::KeyDown(const char* bytes, int32 numBytes)
{
	switch (bytes[0]) {
		case B_SPACE:
		case B_DOWN_ARROW:
		case B_RIGHT_ARROW:
			// invoke from keyboard
			fMenu->Go(ConvertToScreen(BPoint(4, 4)), true, true);
			break;

		default:
			BView::KeyDown(bytes, numBytes);
			break;
	}
}


void
MiniMenuField::Draw(BRect)
{
	BRect bounds(Bounds());
	bounds.InsetBy(2, 2);
	BRect rect(bounds);
	rect.right--;
	rect.bottom--;
	
	rgb_color darkest = tint_color(kBlack, 0.6f);
	rgb_color dark = tint_color(kBlack, 0.4f);
	rgb_color medium = dark;
	rgb_color light = tint_color(kBlack, 0.03f);
	
	SetHighColor(medium);
	
	// draw frame and shadow
	BeginLineArray(10);
	AddLine(rect.RightTop(), rect.RightBottom(), darkest);
	AddLine(rect.RightBottom(), rect.LeftBottom(), darkest);
	AddLine(rect.LeftBottom(), rect.LeftTop(), medium);
	AddLine(rect.LeftTop(), rect.RightTop(), medium);
	AddLine(bounds.LeftBottom() + BPoint(2, 0), bounds.RightBottom(), dark);
	AddLine(bounds.RightTop() + BPoint(0, 1), bounds.RightBottom(), dark);
	rect.InsetBy(1, 1);
	AddLine(rect.RightTop(), rect.RightBottom(), medium);
	AddLine(rect.RightBottom(), rect.LeftBottom(), medium);
	AddLine(rect.LeftBottom(), rect.LeftTop(), light);
	AddLine(rect.LeftTop(), rect.RightTop(), light);

	EndLineArray();
	
	// draw triangle
	rect = BRect(5, 5, 15, 15);
	const rgb_color outlineColor = kBlack;
	const rgb_color middleColor = {150, 150, 150, 255};

	BeginLineArray(5);
	AddLine(BPoint(rect.left + 3, rect.top + 1),
		BPoint(rect.left + 3, rect.top + 7), outlineColor);
	AddLine(BPoint(rect.left + 3, rect.top + 1),
		BPoint(rect.left + 6, rect.top + 4), outlineColor);
	AddLine(BPoint(rect.left + 6, rect.top + 4),
		BPoint(rect.left + 3, rect.top + 7), outlineColor);

	AddLine(BPoint(rect.left + 4, rect.top + 3),
		BPoint(rect.left + 4, rect.top + 5), middleColor);
	AddLine(BPoint(rect.left + 5, rect.top + 4),
		BPoint(rect.left + 5, rect.top + 4), middleColor);
	EndLineArray();

	// draw focus if focused, else erase focus
	bounds = Bounds();
	bool focused = IsFocus() && Window()->IsActive();
	rgb_color markColor = ui_color(B_KEYBOARD_NAVIGATION_COLOR);
	rgb_color viewColor = ViewColor();
	BeginLineArray(4);
	AddLine(BPoint(bounds.left, bounds.top),
		BPoint(bounds.right, bounds.top), focused ? markColor : viewColor);
	AddLine(BPoint(bounds.right, bounds.top),
		BPoint(bounds.right, bounds.bottom), focused ? markColor : viewColor);
	AddLine(BPoint(bounds.right, bounds.bottom),
		BPoint(bounds.left, bounds.bottom), focused ? markColor : viewColor);
	AddLine(BPoint(bounds.left, bounds.bottom),
		BPoint(bounds.left, bounds.top), focused ? markColor : viewColor);
	EndLineArray();
}


void
MiniMenuField::MouseDown(BPoint)
{
	fMenu->Go(ConvertToScreen(BPoint(4, 4)), true);
}

