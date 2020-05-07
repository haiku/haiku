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


#include "DialogPane.h"

#include <ControlLook.h>
#include <LayoutUtils.h>

#include "Utilities.h"
#include "Window.h"


const rgb_color kNormalColor = {150, 150, 150, 255};
const rgb_color kHighlightColor = {100, 100, 0, 255};


//	#pragma mark - PaneSwitch


PaneSwitch::PaneSwitch(BRect frame, const char* name, bool leftAligned,
		uint32 resizeMask, uint32 flags)
	:
	BControl(frame, name, "", 0, resizeMask, flags),
	fLeftAligned(leftAligned),
	fPressing(false),
	fLabelOn(NULL),
	fLabelOff(NULL)
{
}


PaneSwitch::PaneSwitch(const char* name, bool leftAligned, uint32 flags)
	:
	BControl(name, "", 0, flags),
	fLeftAligned(leftAligned),
	fPressing(false),
	fLabelOn(NULL),
	fLabelOff(NULL)
{
}


PaneSwitch::~PaneSwitch()
{
	free(fLabelOn);
	free(fLabelOff);
}


void
PaneSwitch::Draw(BRect)
{
	BRect bounds(Bounds());

	// Draw the label, if any
	const char* label = fLabelOff;
	if (fLabelOn != NULL && Value() == B_CONTROL_ON)
		label = fLabelOn;

	if (label != NULL) {
		BPoint point;
		float latchSize = be_plain_font->Size();
		float labelDist = latchSize + ceilf(latchSize / 2.0);
		if (fLeftAligned)
			point.x = labelDist;
		else
			point.x = bounds.right - labelDist - StringWidth(label);

		SetHighUIColor(B_PANEL_TEXT_COLOR);
		font_height fontHeight;
		GetFontHeight(&fontHeight);
		point.y = (bounds.top + bounds.bottom
			- ceilf(fontHeight.ascent) - ceilf(fontHeight.descent)) / 2
			+ ceilf(fontHeight.ascent);

		DrawString(label, point);
	}

	// draw the latch
	if (fPressing)
		DrawInState(kPressed);
	else if (Value())
		DrawInState(kExpanded);
	else
		DrawInState(kCollapsed);

	// ...and the focus indication
	if (!IsFocus() || !Window()->IsActive())
		return;

	rgb_color markColor = ui_color(B_KEYBOARD_NAVIGATION_COLOR);

	BeginLineArray(2);
	AddLine(BPoint(bounds.left + 2, bounds.bottom - 1),
		BPoint(bounds.right - 2, bounds.bottom - 1), markColor);
	AddLine(BPoint(bounds.left + 2, bounds.bottom),
		BPoint(bounds.right - 2, bounds.bottom), kWhite);
	EndLineArray();
}


void
PaneSwitch::MouseDown(BPoint)
{
	if (!IsEnabled())
		return;

	fPressing = true;
	SetMouseEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
	Invalidate();
}


void
PaneSwitch::MouseMoved(BPoint point, uint32 code, const BMessage* message)
{
	int32 buttons;
	BMessage* currentMessage = Window()->CurrentMessage();
	if (currentMessage == NULL
		|| currentMessage->FindInt32("buttons", &buttons) != B_OK) {
		buttons = 0;
	}

	if (buttons != 0) {
		BRect bounds(Bounds());
		bounds.InsetBy(-3, -3);

		bool newPressing = bounds.Contains(point);
		if (newPressing != fPressing) {
			fPressing = newPressing;
			Invalidate();
		}
	}

	BControl::MouseMoved(point, code, message);
}


void
PaneSwitch::MouseUp(BPoint point)
{
	BRect bounds(Bounds());
	bounds.InsetBy(-3, -3);

	fPressing = false;
	Invalidate();
	if (bounds.Contains(point)) {
		SetValue(!Value());
		Invoke();
	}

	BControl::MouseUp(point);
}


void
PaneSwitch::GetPreferredSize(float* _width, float* _height)
{
	BSize size = MinSize();
	if (_width != NULL)
		*_width = size.width;

	if (_height != NULL)
		*_height = size.height;
}


BSize
PaneSwitch::MinSize()
{
	BSize size;
	float onLabelWidth = StringWidth(fLabelOn);
	float offLabelWidth = StringWidth(fLabelOff);
	float labelWidth = max_c(onLabelWidth, offLabelWidth);
	float latchSize = be_plain_font->Size();
	size.width = latchSize;
	if (labelWidth > 0.0)
		size.width += ceilf(latchSize / 2.0) + labelWidth;

	font_height fontHeight;
	GetFontHeight(&fontHeight);
	size.height = ceilf(fontHeight.ascent) + ceilf(fontHeight.descent);
	size.height = max_c(size.height, latchSize);

	return BLayoutUtils::ComposeSize(ExplicitMinSize(), size);
}


BSize
PaneSwitch::MaxSize()
{
	return BLayoutUtils::ComposeSize(ExplicitMaxSize(), MinSize());
}


BSize
PaneSwitch::PreferredSize()
{
	return BLayoutUtils::ComposeSize(ExplicitPreferredSize(), MinSize());
}


void
PaneSwitch::SetLabels(const char* labelOn, const char* labelOff)
{
	free(fLabelOn);
	free(fLabelOff);

	if (labelOn != NULL)
		fLabelOn = strdup(labelOn);
	else
		fLabelOn = NULL;

	if (labelOff != NULL)
		fLabelOff = strdup(labelOff);
	else
		fLabelOff = NULL;

	Invalidate();
	InvalidateLayout();
}


void
PaneSwitch::DrawInState(PaneSwitch::State state)
{
	BRect rect(0, 0, be_plain_font->Size(), be_plain_font->Size());
	rect.OffsetBy(1, 1);

	rgb_color arrowColor = state == kPressed ? kHighlightColor : kNormalColor;
	int32 arrowDirection = BControlLook::B_RIGHT_ARROW;
	float tint = IsEnabled() && Window()->IsActive() ? B_DARKEN_3_TINT
		: B_DARKEN_1_TINT;

	switch (state) {
		case kCollapsed:
			arrowDirection = BControlLook::B_RIGHT_ARROW;
			break;

		case kPressed:
			arrowDirection = BControlLook::B_RIGHT_DOWN_ARROW;
			break;

		case kExpanded:
			arrowDirection = BControlLook::B_DOWN_ARROW;
			break;
	}

	SetDrawingMode(B_OP_COPY);
	be_control_look->DrawArrowShape(this, rect, rect, arrowColor,
		arrowDirection, 0, tint);
}
