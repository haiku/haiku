/*
 * Copyright 2003-2013 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Phipps
 *		Axel Dörfler, axeld@pinc-software.de
 */

#include "ScreenCornerSelector.h"

#include <stdio.h>

#include <Cursor.h>
#include <Point.h>
#include <Rect.h>
#include <Screen.h>
#include <Shape.h>
#include <Window.h>


static const float kAspectRatio = 4.0f / 3.0f;
static const float kMonitorBorderSize = 3.0f;
static const float kArrowSize = 11.0f;
static const float kStopSize = 15.0f;


ScreenCornerSelector::ScreenCornerSelector(BRect frame, const char* name, BMessage* message,
	uint32 resizingMode)
	: BControl(frame, name, NULL, message, resizingMode,
		B_WILL_DRAW | B_NAVIGABLE | B_FULL_UPDATE_ON_RESIZE),
	fCurrentCorner(B_DESKBAR_RIGHT_TOP | kExpandBit),
	fDragging(false)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
}


BRect
ScreenCornerSelector::_MonitorFrame() const
{
	float width = Bounds().Width();
	float height = Bounds().Height();

	if (width / kAspectRatio > height)
		width = height * kAspectRatio;
	else if (height * kAspectRatio > width)
		height = width / kAspectRatio;

	return BRect((Bounds().Width() - width) / 2, (Bounds().Height() - height) / 2,
		(Bounds().Width() + width) / 2, (Bounds().Height() + height) / 2);
}


BRect
ScreenCornerSelector::_InnerFrame(BRect monitorFrame) const
{
	return monitorFrame.InsetByCopy(kMonitorBorderSize + 3, kMonitorBorderSize + 3);
}


void
ScreenCornerSelector::Draw(BRect updateRect)
{
	rgb_color darkColor = {160, 160, 160, 255};
	rgb_color blackColor = {0, 0, 0, 255};
	rgb_color redColor = {228, 0, 0, 255};

	BRect outerRect = _MonitorFrame();
	BRect innerRect(outerRect.InsetByCopy(kMonitorBorderSize + 2, kMonitorBorderSize + 2));

	SetDrawingMode(B_OP_OVER);

	if (!_InnerFrame(outerRect).Contains(updateRect)) {
		// frame & background

		// if the focus is changing, we don't redraw the whole view, but only
		// the part that's affected by the change
		if (!IsFocusChanging()) {
			SetHighColor(darkColor);
			FillRoundRect(outerRect, kMonitorBorderSize * 3 / 2, kMonitorBorderSize * 3 / 2);
		}

		if (IsFocus() && Window()->IsActive())
			SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
		else
			SetHighColor(blackColor);

		StrokeRoundRect(outerRect, kMonitorBorderSize * 3 / 2, kMonitorBorderSize * 3 / 2);

		if (IsFocusChanging())
			return;

		// power light

		SetHighColor(redColor);
		BPoint powerPos(outerRect.left + kMonitorBorderSize * 2,
			outerRect.bottom - kMonitorBorderSize);
		StrokeLine(powerPos, BPoint(powerPos.x + 2, powerPos.y));
	}

	if (!IsFocusChanging()) {
		SetHighColor(210, 210, 255);
		FillRoundRect(innerRect, kMonitorBorderSize, kMonitorBorderSize);
	}

	if (IsFocus() && Window()->IsActive())
		SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
	else
		SetHighColor(blackColor);
	StrokeRoundRect(innerRect, kMonitorBorderSize, kMonitorBorderSize);

	innerRect = _InnerFrame(outerRect);

	_DrawArrow(innerRect);

	SetDrawingMode(B_OP_COPY);
}


status_t
ScreenCornerSelector::Invoke(BMessage* message)
{
	if (message == NULL)
		message = Message();

	BMessage clone(*message);
	clone.AddInt32("location", Value() & ~kExpandBit);
	clone.AddBool("expand", (Value() & kExpandBit) != 0);
		// TODO add support for "expand" values as well

	return BInvoker::Invoke(&clone);
}


int32
ScreenCornerSelector::Value()
{
	return (int32)fCurrentCorner;
}


void
ScreenCornerSelector::SetValue(int32 corner)
{
	switch (corner) {
		case B_DESKBAR_TOP:
		case B_DESKBAR_BOTTOM:
		case B_DESKBAR_LEFT_TOP:
		case B_DESKBAR_RIGHT_TOP:
		case B_DESKBAR_LEFT_BOTTOM:
		case B_DESKBAR_RIGHT_BOTTOM:
		case B_DESKBAR_LEFT_TOP | kExpandBit:
		case B_DESKBAR_RIGHT_TOP | kExpandBit:
#if 0
		case B_DESKBAR_LEFT_BOTTOM | kExpandBit:
		case B_DESKBAR_RIGHT_BOTTOM | kExpandBit:
#endif
			break;

		default:
			return;
	}

	if (corner == fCurrentCorner)
		return;

	fCurrentCorner = corner;
	Invalidate(_InnerFrame(_MonitorFrame()));
}


deskbar_location
ScreenCornerSelector::Corner() const
{
	return (deskbar_location)(fCurrentCorner & ~kExpandBit);
}


void
ScreenCornerSelector::SetCorner(deskbar_location corner)
{
	// redirected to SetValue() to make sure only valid values are set
	SetValue((int32)corner);
}


void
ScreenCornerSelector::_DrawArrow(BRect innerFrame)
{
	// TODO: support horizontal mini-mode

	switch (fCurrentCorner) {
		case B_DESKBAR_TOP:
			innerFrame.bottom = innerFrame.top + kArrowSize / 2;
			break;

		case B_DESKBAR_BOTTOM:
			innerFrame.top = innerFrame.bottom - kArrowSize / 2;
			break;

		case B_DESKBAR_LEFT_TOP:
			innerFrame.right = innerFrame.left + kArrowSize;
			innerFrame.bottom = innerFrame.top + kArrowSize;
			break;

		case B_DESKBAR_RIGHT_TOP:
			innerFrame.left = innerFrame.right - kArrowSize;
			innerFrame.bottom = innerFrame.top + kArrowSize;
			break;

		case B_DESKBAR_LEFT_BOTTOM:
			innerFrame.right = innerFrame.left + kArrowSize;
			innerFrame.top = innerFrame.bottom - kArrowSize;
			break;

		case B_DESKBAR_RIGHT_BOTTOM:
			innerFrame.left = innerFrame.right - kArrowSize;
			innerFrame.top = innerFrame.bottom - kArrowSize;
			break;

		case B_DESKBAR_LEFT_TOP | kExpandBit:
			innerFrame.right = innerFrame.left + kArrowSize;
			innerFrame.bottom = innerFrame.top + 2 * kArrowSize;
			break;

		case B_DESKBAR_RIGHT_TOP | kExpandBit:
			innerFrame.left = innerFrame.right - kArrowSize;
			innerFrame.bottom = innerFrame.top + 2 * kArrowSize;
			break;
#if 0
		case B_DESKBAR_LEFT_BOTTOM | kExpandBit:
			innerFrame.right = innerFrame.left + kArrowSize;
			innerFrame.top = innerFrame.bottom - 2 * kArrowSize;
			break;

		case B_DESKBAR_RIGHT_BOTTOM | kExpandBit:
			innerFrame.left = innerFrame.right - kArrowSize;
			innerFrame.top = innerFrame.bottom - 2 * kArrowSize;
			break;
#endif
	}

	innerFrame.InsetBy(1, 1);
	SetHighColor(make_color(0, 0, 0, 255));
	FillRect(innerFrame);
}


int32
ScreenCornerSelector::_ScreenCorner(BPoint point) const
{
	BRect innerFrame = _InnerFrame(_MonitorFrame());

	float leftX = innerFrame.left + innerFrame.Width() / 3;
	float rightX = innerFrame.left + 2 * innerFrame.Width() / 3;

	float topY = innerFrame.top + innerFrame.Height() / 3;
	float centerY = innerFrame.top + innerFrame.Height() / 2;
	float bottomY = innerFrame.top + 2 * innerFrame.Height() / 3;

	// TODO: support horizontal mini-mode

	// Note: expanded mode from the bottom is not supported at the moment
	if (point.x < leftX) {
		if (point.y < topY)
			return B_DESKBAR_LEFT_TOP;

		if (point.y > bottomY)
			return B_DESKBAR_LEFT_BOTTOM;

		return B_DESKBAR_LEFT_TOP | kExpandBit;
	}

	if (point.x > rightX) {
		if (point.y < topY)
			return B_DESKBAR_RIGHT_TOP;

		if (point.y > bottomY)
			return B_DESKBAR_RIGHT_BOTTOM;

		return B_DESKBAR_RIGHT_TOP | kExpandBit;
	}

	if (point.y < centerY)
		return B_DESKBAR_TOP;

	return B_DESKBAR_BOTTOM;
}


void
ScreenCornerSelector::MouseDown(BPoint where)
{
	fDragging = true;
	SetValue(_ScreenCorner(where));
	Invoke(Message());
	SetMouseEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
	static BCursor grabCursor(B_CURSOR_ID_GRAB);
	SetViewCursor(&grabCursor);
}


void
ScreenCornerSelector::MouseUp(BPoint where)
{
	fDragging = false;
	static BCursor defaultCursor(B_CURSOR_ID_SYSTEM_DEFAULT);
	SetViewCursor(&defaultCursor);
}


void
ScreenCornerSelector::MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage)
{
	if (!fDragging)
		return;

	SetValue(_ScreenCorner(where));
	Invoke(Message());
}


void
ScreenCornerSelector::KeyDown(const char* bytes, int32 numBytes)
{
	switch (bytes[0]) {
		case B_LEFT_ARROW:
		case '4':
			SetValue(B_DESKBAR_LEFT_TOP | kExpandBit);
			break;

		case B_RIGHT_ARROW:
		case '6':
			SetValue(B_DESKBAR_RIGHT_TOP | kExpandBit);
			break;

		case B_UP_ARROW:
		case '8':
			SetValue(B_DESKBAR_TOP);
			break;

		case B_DOWN_ARROW:
		case '2':
			SetValue(B_DESKBAR_BOTTOM);
			break;

		case B_HOME:
		case '7':
			SetValue(B_DESKBAR_LEFT_TOP);
			break;

		case B_PAGE_UP:
		case '9':
			SetValue(B_DESKBAR_RIGHT_TOP);
			break;

		case B_PAGE_DOWN:
		case '3':
			SetValue(B_DESKBAR_RIGHT_BOTTOM);
			break;

		case B_END:
		case '1':
			SetValue(B_DESKBAR_LEFT_BOTTOM);
			break;

		default:
			return BControl::KeyDown(bytes, numBytes);
	}

	Invoke(Message());
}
