/*
 * Copyright 2003-2009, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Phipps
 *		Axel Dörfler, axeld@pinc-software.de
 */

#include "ScreenCornerSelector.h"
#include "Constants.h"
#include "Utility.h"

#include <Rect.h>
#include <Point.h>
#include <Shape.h>
#include <Screen.h>
#include <Window.h>

#include <stdio.h>


static const float kAspectRatio = 4.0f / 3.0f;
static const float kMonitorBorderSize = 3.0f;
static const float kArrowSize = 11.0f;
static const float kStopSize = 15.0f;


ScreenCornerSelector::ScreenCornerSelector(BRect frame, const char *name,
		BMessage* message, uint32 resizingMode)
	: BControl(frame, name, NULL, message, resizingMode,
		B_WILL_DRAW | B_NAVIGABLE),
	fCurrentCorner(NO_CORNER),
	fPreviousCorner(-1)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
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

	return BRect((Bounds().Width() - width) / 2,
		(Bounds().Height() - height) / 2,
		(Bounds().Width() + width) / 2, (Bounds().Height() + height) / 2);
}


BRect
ScreenCornerSelector::_InnerFrame(BRect monitorFrame) const
{
	return monitorFrame.InsetByCopy(kMonitorBorderSize + 3,
		kMonitorBorderSize + 3);
}


BRect
ScreenCornerSelector::_CenterFrame(BRect innerFrame) const
{
	return innerFrame.InsetByCopy(kArrowSize, kArrowSize);
}


void
ScreenCornerSelector::Draw(BRect update)
{
	rgb_color darkColor = {160, 160, 160, 255};
	rgb_color blackColor = {0, 0, 0, 255};
	rgb_color redColor = {228, 0, 0, 255};

	BRect outerRect = _MonitorFrame();
	BRect innerRect(outerRect.InsetByCopy(kMonitorBorderSize + 2,
		kMonitorBorderSize + 2));

	SetDrawingMode(B_OP_OVER);

	if (!_InnerFrame(outerRect).Contains(update)) {
		// frame & background

		// if the focus is changing, we don't redraw the whole view, but only
		// the part that's affected by the change
		if (!IsFocusChanging()) {
			SetHighColor(darkColor);
			FillRoundRect(outerRect, kMonitorBorderSize * 3 / 2,
				kMonitorBorderSize * 3 / 2);
		}

		if (IsFocus() && Window()->IsActive())
			SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
		else
			SetHighColor(blackColor);

		StrokeRoundRect(outerRect, kMonitorBorderSize * 3 / 2,
			kMonitorBorderSize * 3 / 2);

		if (!IsFocusChanging()) {
			SetHighColor(210, 210, 255);
			FillRoundRect(innerRect, kMonitorBorderSize, kMonitorBorderSize);
		}

		if (IsFocus() && Window()->IsActive())
			SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
		else
			SetHighColor(blackColor);
		StrokeRoundRect(innerRect, kMonitorBorderSize, kMonitorBorderSize);

		if (IsFocusChanging())
			return;

		// power light

		SetHighColor(redColor);
		BPoint powerPos(outerRect.left + kMonitorBorderSize * 2, outerRect.bottom
			- kMonitorBorderSize);
		StrokeLine(powerPos, BPoint(powerPos.x + 2, powerPos.y));
	}

	innerRect = _InnerFrame(outerRect);

	if (fCurrentCorner != NO_CORNER)
		_DrawArrow(innerRect);
	else
		_DrawStop(innerRect);

	SetDrawingMode(B_OP_COPY);
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
		case UP_LEFT_CORNER:
		case UP_RIGHT_CORNER:
		case DOWN_LEFT_CORNER:
		case DOWN_RIGHT_CORNER:
		case NO_CORNER:
			break;

		default:
			corner = NO_CORNER;
	}
	if ((screen_corner)corner == fCurrentCorner)
		return;

	fCurrentCorner = (screen_corner)corner;
	Invalidate(_InnerFrame(_MonitorFrame()));
	Invoke();
}


screen_corner
ScreenCornerSelector::Corner() const
{
	return fCurrentCorner;
}


void
ScreenCornerSelector::SetCorner(screen_corner corner)
{
	// redirected to SetValue() to make sure only valid values are set
	SetValue((int32)corner);
}


void
ScreenCornerSelector::_DrawStop(BRect innerFrame)
{
	BRect centerRect = _CenterFrame(innerFrame);
	float size = kStopSize;
	BRect rect;
	rect.left = centerRect.left + (centerRect.Width() - size) / 2;
	rect.top = centerRect.top + (centerRect.Height() - size) / 2;
	if (rect.left < centerRect.left || rect.top < centerRect.top) {
		size = centerRect.Height();
		rect.top = centerRect.top;
		rect.left = centerRect.left + (centerRect.Width() - size) / 2;
	}
	rect.right = rect.left + size - 1;
	rect.bottom = rect.top + size - 1;

	SetHighColor(255, 0, 0);
	SetPenSize(2);
	StrokeEllipse(rect);

	size -= ceilf(sin(M_PI / 4) * size + 2);
	rect.InsetBy(size, size);
	StrokeLine(rect.RightTop(), rect.LeftBottom());

	SetPenSize(1);
}


void
ScreenCornerSelector::_DrawArrow(BRect innerFrame)
{
	float size = kArrowSize;
	float sizeX = fCurrentCorner == UP_LEFT_CORNER
		|| fCurrentCorner == DOWN_LEFT_CORNER ? size : -size;
	float sizeY = fCurrentCorner == UP_LEFT_CORNER
		|| fCurrentCorner == UP_RIGHT_CORNER ? size : -size;

	innerFrame.InsetBy(2, 2);
	BPoint origin(sizeX < 0 ? innerFrame.right : innerFrame.left,
		sizeY < 0 ? innerFrame.bottom : innerFrame.top);

	SetHighColor(kBlack);
	FillTriangle(BPoint(origin.x, origin.y), BPoint(origin.x, origin.y + sizeY),
		BPoint(origin.x + sizeX, origin.y));
}


screen_corner
ScreenCornerSelector::_ScreenCorner(BPoint point,
	screen_corner previousCorner) const
{
	BRect innerFrame = _InnerFrame(_MonitorFrame());

	if (!innerFrame.Contains(point))
		return previousCorner;

	if (_CenterFrame(innerFrame).Contains(point))
		return NO_CORNER;

	float centerX = innerFrame.left + innerFrame.Width() / 2;
	float centerY = innerFrame.top + innerFrame.Height() / 2;
	if (point.x < centerX)
		return point.y < centerY ? UP_LEFT_CORNER : DOWN_LEFT_CORNER;

	return point.y < centerY ? UP_RIGHT_CORNER : DOWN_RIGHT_CORNER;
}


void
ScreenCornerSelector::MouseDown(BPoint where)
{
	fPreviousCorner = Value();

	SetValue(_ScreenCorner(where, (screen_corner)fPreviousCorner));
	SetMouseEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
}


void
ScreenCornerSelector::MouseUp(BPoint where)
{
	fPreviousCorner = -1;
}


void
ScreenCornerSelector::MouseMoved(BPoint where, uint32 transit,
	const BMessage* dragMessage)
{
	if (fPreviousCorner == -1)
		return;

	SetValue(_ScreenCorner(where, (screen_corner)fPreviousCorner));
}


void
ScreenCornerSelector::KeyDown(const char* bytes, int32 numBytes)
{
	switch (bytes[0]) {
		// arrow keys

		case B_LEFT_ARROW:
		case '4':
			if (Corner() == UP_RIGHT_CORNER)
				SetCorner(UP_LEFT_CORNER);
			else if (Corner() == DOWN_RIGHT_CORNER)
				SetCorner(DOWN_LEFT_CORNER);
			break;
		case B_RIGHT_ARROW:
		case '6':
			if (Corner() == UP_LEFT_CORNER)
				SetCorner(UP_RIGHT_CORNER);
			else if (Corner() == DOWN_LEFT_CORNER)
				SetCorner(DOWN_RIGHT_CORNER);
			break;
		case B_UP_ARROW:
		case '8':
			if (Corner() == DOWN_LEFT_CORNER)
				SetCorner(UP_LEFT_CORNER);
			else if (Corner() == DOWN_RIGHT_CORNER)
				SetCorner(UP_RIGHT_CORNER);
			break;
		case B_DOWN_ARROW:
		case '2':
			if (Corner() == UP_LEFT_CORNER)
				SetCorner(DOWN_LEFT_CORNER);
			else if (Corner() == UP_RIGHT_CORNER)
				SetCorner(DOWN_RIGHT_CORNER);
			break;

		// numlock keys

		case B_HOME:
		case '7':
			SetCorner(UP_LEFT_CORNER);
			break;
		case B_PAGE_UP:
		case '9':
			SetCorner(UP_RIGHT_CORNER);
			break;
		case B_PAGE_DOWN:
		case '3':
			SetCorner(DOWN_RIGHT_CORNER);
			break;
		case B_END:
		case '1':
			SetCorner(DOWN_LEFT_CORNER);
			break;

		default:
			BControl::KeyDown(bytes, numBytes);
	}
}

