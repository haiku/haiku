/*
 * Copyright 2006-2012, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "SwatchView.h"

#include <new>
#include <stdio.h>

#include <Bitmap.h>
#include <Cursor.h>
#include <Looper.h>
#include <Message.h>
#include <TypeConstants.h>
#include <Window.h>

#include "cursors.h"
#include "ui_defines.h"
#include "support.h"
#include "support_ui.h"


#define DRAG_INIT_DIST 10.0


SwatchView::SwatchView(const char* name, BMessage* message, BHandler* target,
	rgb_color color, float width, float height)
	:
	BView(BRect(0.0, 0.0, width, height), name, B_FOLLOW_NONE, B_WILL_DRAW),
	fColor(color),
	fTrackingStart(-1.0, -1.0),
	fActive(false),
	fDropInvokes(false),
	fClickMessage(message),
	fDroppedMessage(NULL),
	fTarget(target),
	fWidth(width),
	fHeight(height)
{
	SetViewColor(B_TRANSPARENT_32_BIT);
	SetHighColor(fColor);
}


SwatchView::~SwatchView()
{
	delete fClickMessage;
	delete fDroppedMessage;
}


inline void
blend_color(rgb_color& a, const rgb_color& b, float alpha)
{
	float alphaInv = 1.0 - alpha;
	a.red = (uint8)(b.red * alphaInv + a.red * alpha);
	a.green = (uint8)(b.green * alphaInv + a.green * alpha);
	a.blue = (uint8)(b.blue * alphaInv + a.blue * alpha);
}


void
SwatchView::Draw(BRect updateRect)
{
	BRect r(Bounds());

	rgb_color colorLight = tint_color(fColor, B_LIGHTEN_2_TINT);
	rgb_color colorShadow = tint_color(fColor, B_DARKEN_2_TINT);

	if (fColor.alpha < 255) {
		// left/top
		float alpha = fColor.alpha / 255.0;

		rgb_color h = colorLight;
		blend_color(h, kAlphaHigh, alpha);
		rgb_color l = colorLight;
		blend_color(l, kAlphaLow, alpha);

		SetHighColor(h);
		SetLowColor(l);
		
		StrokeLine(BPoint(r.left, r.bottom - 1),
				   BPoint(r.left, r.top), kDottedBig);
		StrokeLine(BPoint(r.left + 1, r.top),
				   BPoint(r.right, r.top), kDottedBig);

		// right/bottom
		h = colorShadow;
		blend_color(h, kAlphaHigh, alpha);
		l = colorShadow;
		blend_color(l, kAlphaLow, alpha);

		SetHighColor(h);
		SetLowColor(l);
		
		StrokeLine(BPoint(r.right, r.top + 1),
				   BPoint(r.right, r.bottom), kDottedBig);
		StrokeLine(BPoint(r.right - 1, r.bottom),
				   BPoint(r.left, r.bottom), kDottedBig);

		// fill
		r.InsetBy(1.0, 1.0);

		h = fColor;
		blend_color(h, kAlphaHigh, alpha);
		l = fColor;
		blend_color(l, kAlphaLow, alpha);

		SetHighColor(h);
		SetLowColor(l);		

		FillRect(r, kDottedBig);
	} else {
		_StrokeRect(r, colorLight, colorShadow);
		r.InsetBy(1.0, 1.0);
		SetHighColor(fColor);
		FillRect(r);
	}
}


void
SwatchView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_PASTE:
		{
			rgb_color color;
			if (restore_color_from_message(message, color) == B_OK) {
				SetColor(color);
				_Invoke(fDroppedMessage);
			}
			break;
		}

		default:
			BView::MessageReceived(message);
			break;
	}
}


void
SwatchView::MouseDown(BPoint where)
{
	if (Bounds().Contains(where))
		fTrackingStart = where;
}


void
SwatchView::MouseUp(BPoint where)
{
	if (Bounds().Contains(where) && Bounds().Contains(fTrackingStart))
		_Invoke(fClickMessage);

	fTrackingStart.x = -1.0;
	fTrackingStart.y = -1.0;
}


void
SwatchView::MouseMoved(BPoint where, uint32 transit,
	const BMessage* dragMessage)
{
	if (transit == B_ENTERED_VIEW) {
		BCursor cursor(kDropperCursor);
		SetViewCursor(&cursor, true);
	}

	if (Bounds().Contains(fTrackingStart)) {
		if (point_point_distance(where, fTrackingStart) > DRAG_INIT_DIST
			|| transit == B_EXITED_VIEW) {
			_DragColor();
			fTrackingStart.x = -1.0;
			fTrackingStart.y = -1.0;
		}
	}
}


void
SwatchView::SetColor(rgb_color color)
{
	fColor = color;
	SetHighColor(fColor);
	Invalidate();
}


void
SwatchView::SetClickedMessage(BMessage* message)
{
	if (message != fClickMessage)
		delete fClickMessage;
	fClickMessage = message;
}


void
SwatchView::SetDroppedMessage(BMessage* message)
{
	if (message != fDroppedMessage)
		delete fDroppedMessage;
	fDroppedMessage = message;
}


void
SwatchView::_Invoke(const BMessage* _message)
{
	if (_message == NULL)
		return;

	BHandler* target = fTarget;
	if (target == NULL)
		target = Window();

	if (target == NULL)
		return;

	BLooper* looper = target->Looper();
	if (looper == NULL)
		return;

	BMessage message(*_message);
	message.AddPointer("be:source", (void*)this);
	message.AddInt64("be:when", system_time());
	message.AddBool("begin", true);
	store_color_in_message(&message, fColor);
	looper->PostMessage(&message, target);
}


void
SwatchView::_StrokeRect(BRect r, rgb_color leftTop, rgb_color rightBottom)
{
	BeginLineArray(4);

	AddLine(BPoint(r.left, r.bottom - 1), BPoint(r.left, r.top), leftTop);
	AddLine(BPoint(r.left + 1, r.top), BPoint(r.right, r.top), leftTop);
	AddLine(BPoint(r.right, r.top + 1), BPoint(r.right, r.bottom),
		rightBottom);
	AddLine(BPoint(r.right - 1, r.bottom), BPoint(r.left, r.bottom),
		rightBottom);

	EndLineArray();
}


void
SwatchView::_DragColor()
{
	BBitmap* bitmap = new(std::nothrow) BBitmap(BRect(0.0, 0.0, 15.0, 15.0),
		B_RGB32);

	BMessage message = make_color_drop_message(fColor, bitmap);

	DragMessage(&message, bitmap, B_OP_ALPHA, BPoint(9.0, 9.0));
}

