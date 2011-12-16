/*
 * Copyright 2001-2009, Haiku.
 * Copyright 2002, Thomas Kurschel.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Rafael Romo
 *		Thomas Kurschel
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "MonitorView.h"

#include <stdio.h>

#include <Catalog.h>
#include <Locale.h>
#include <Roster.h>
#include <Screen.h>

#include "Constants.h"

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "Monitor View"


MonitorView::MonitorView(BRect rect, const char *name, int32 width, int32 height)
	: BView(rect, name, B_FOLLOW_ALL, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
	fMaxWidth(1920),
	fMaxHeight(1200),
	fWidth(width),
	fHeight(height),
	fDPI(0)
{
	BScreen screen(B_MAIN_SCREEN_ID);
	fDesktopColor = screen.DesktopColor(current_workspace());
}


MonitorView::~MonitorView()
{
}


void
MonitorView::AttachedToWindow()
{
	SetViewColor(B_TRANSPARENT_COLOR);
	if (Parent())
		fBackgroundColor = Parent()->ViewColor();
	else
		fBackgroundColor = ui_color(B_PANEL_BACKGROUND_COLOR);

	_UpdateDPI();
}


void
MonitorView::MouseDown(BPoint point)
{
	be_roster->Launch(kBackgroundsSignature);
}


void
MonitorView::Draw(BRect updateRect)
{
	rgb_color darkColor = {160, 160, 160, 255};
	rgb_color blackColor = {0, 0, 0, 255};
	rgb_color redColor = {228, 0, 0, 255};
	rgb_color whiteColor = {255, 255, 255, 255};
	BRect outerRect = _MonitorBounds();

	SetHighColor(fBackgroundColor);
	FillRect(updateRect);

	SetDrawingMode(B_OP_OVER);

	// frame & background

	SetHighColor(darkColor);
	FillRoundRect(outerRect, 3.0, 3.0);

	SetHighColor(blackColor);
	StrokeRoundRect(outerRect, 3.0, 3.0);

	SetHighColor(fDesktopColor);

	BRect innerRect(outerRect.InsetByCopy(4, 4));
	FillRoundRect(innerRect, 2.0, 2.0);

	SetHighColor(blackColor);
	StrokeRoundRect(innerRect, 2.0, 2.0);

	SetDrawingMode(B_OP_COPY);

	// power light

	SetHighColor(redColor);
	BPoint powerPos(outerRect.left + 5, outerRect.bottom - 2);
	StrokeLine(powerPos, BPoint(powerPos.x + 2, powerPos.y));

	// DPI

	if (fDPI == 0)
		return;

	font_height fontHeight;
	GetFontHeight(&fontHeight);
	float height = ceilf(fontHeight.ascent + fontHeight.descent);

	char text[64];
	snprintf(text, sizeof(text), B_TRANSLATE("%ld dpi"), fDPI);

	float width = StringWidth(text);
	if (width > innerRect.Width() || height > innerRect.Height())
		return;

	SetLowColor(fDesktopColor);
	SetHighColor(whiteColor);

	DrawString(text, BPoint(innerRect.left + (innerRect.Width() - width) / 2,
		innerRect.top + fontHeight.ascent + (innerRect.Height() - height) / 2));
}


void
MonitorView::SetResolution(int32 width, int32 height)
{
	if (fWidth == width && fHeight == height)
		return;

	fWidth = width;
	fHeight = height;

	_UpdateDPI();
	Invalidate();
}


void
MonitorView::SetMaxResolution(int32 width, int32 height)
{
	if (fMaxWidth == width && fMaxHeight == height)
		return;

	fMaxWidth = width;
	fMaxHeight = height;

	Invalidate();
}


void
MonitorView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case UPDATE_DESKTOP_MSG:
		{
			int32 width, height;
			if (message->FindInt32("width", &width) == B_OK
				&& message->FindInt32("height", &height) == B_OK)
				SetResolution(width, height);
			break;
		}

		case UPDATE_DESKTOP_COLOR_MSG:
		{
			BScreen screen(Window());
			rgb_color color = screen.DesktopColor(current_workspace());
			if (color != fDesktopColor) {
				fDesktopColor = color;
				Invalidate();
			}
			break;
		}

		default:
			BView::MessageReceived(message);
			break;
	}
}


BRect
MonitorView::_MonitorBounds()
{
	float maxWidth = Bounds().Width();
	float maxHeight = Bounds().Height();
	if (maxWidth / maxHeight > (float)fMaxWidth / fMaxHeight)
		maxWidth = maxHeight / fMaxHeight * fMaxWidth;
	else
		maxHeight = maxWidth / fMaxWidth * fMaxHeight;

	float factorX = (float)fWidth / fMaxWidth;
	float factorY = (float)fHeight / fMaxHeight;

	if (factorX > factorY && factorX > 1) {
		factorY /= factorX;
		factorX = 1;
	} else if (factorY > factorX && factorY > 1) {
		factorX /= factorY;
		factorY = 1;
	}

	float width = maxWidth * factorX;
	float height = maxHeight * factorY;

	BSize size = Bounds().Size();
	return BRect((size.width - width) / 2, (size.height - height) / 2,
		(size.width + width) / 2, (size.height + height) / 2);
}


void
MonitorView::_UpdateDPI()
{
	fDPI = 0;

	BScreen screen(Window());
	monitor_info info;
	if (screen.GetMonitorInfo(&info) != B_OK)
		return;

	double x = info.width / 2.54;
	double y = info.height / 2.54;

	fDPI = (int32)round((fWidth / x + fHeight / y) / 2);
}
