/*
 * Copyright 2002-2016 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm, darkwyrm@earthlink.net
 *		John Scipione, jscipione@gmail.com
 */


#include "ColorPreview.h"

#include <algorithm>

#include <stdio.h>

#include <Bitmap.h>
#include <Message.h>
#include <MessageRunner.h>
#include <String.h>
#include <View.h>
#include <Window.h>

#include "defs.h"



static const int32 kMsgMessageRunner = 'MsgR';


//	#pragma mark - ColorPreview


ColorPreview::ColorPreview(BRect frame, BMessage* message, uint32 resizingMode,
	uint32 flags)
	:
	BControl(frame, "ColorPreview", "", message, resizingMode,
		flags | B_WILL_DRAW),
	fColor(ui_color(B_PANEL_BACKGROUND_COLOR)),
	fDisabledColor((rgb_color){ 128, 128, 128 }),
	fMessageRunner(NULL),
	fIsRectangle(true)
{
	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(0, 0, 0);
}


ColorPreview::~ColorPreview(void)
{
}


void
ColorPreview::Draw(BRect updateRect)
{
	rgb_color color;
	if (IsEnabled())
		color = fColor;
	else
		color = fDisabledColor;

	if (fIsRectangle) {
		if (IsEnabled()) {
			rgb_color background = ui_color(B_PANEL_BACKGROUND_COLOR);
			rgb_color shadow = tint_color(background, B_DARKEN_1_TINT);
			rgb_color darkShadow = tint_color(background, B_DARKEN_3_TINT);
			rgb_color light = tint_color(background, B_LIGHTEN_MAX_TINT);

			BRect bounds(Bounds());

			BeginLineArray(4);
			AddLine(BPoint(bounds.left, bounds.bottom),
			BPoint(bounds.left, bounds.top), shadow);
			AddLine(BPoint(bounds.left + 1.0, bounds.top),
			BPoint(bounds.right, bounds.top), shadow);
			AddLine(BPoint(bounds.right, bounds.top + 1.0),
			BPoint(bounds.right, bounds.bottom), light);
			AddLine(BPoint(bounds.right - 1.0, bounds.bottom),
			BPoint(bounds.left + 1.0, bounds.bottom), light);
			EndLineArray();
			bounds.InsetBy(1.0, 1.0);

			BeginLineArray(4);
			AddLine(BPoint(bounds.left, bounds.bottom),
			BPoint(bounds.left, bounds.top), darkShadow);
			AddLine(BPoint(bounds.left + 1.0, bounds.top),
			BPoint(bounds.right, bounds.top), darkShadow);
			AddLine(BPoint(bounds.right, bounds.top + 1.0),
			BPoint(bounds.right, bounds.bottom), background);
			AddLine(BPoint(bounds.right - 1.0, bounds.bottom),
			BPoint(bounds.left + 1.0, bounds.bottom), background);
			EndLineArray();
			bounds.InsetBy(1.0, 1.0);

			SetHighColor(color);
			FillRect(bounds);
		} else {
			SetHighColor(color);
			FillRect(Bounds());
		}
	} else {
		// fill background
		SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		FillRect(updateRect);

		SetHighColor(color);
		FillEllipse(Bounds());

		if (IsEnabled())
			StrokeEllipse(Bounds(), B_SOLID_LOW);
	}
}


void
ColorPreview::MessageReceived(BMessage* message)
{
	// If we received a dropped message, see if it contains color data
	if (message->WasDropped()) {
		rgb_color* color;
		ssize_t size;
		if (message->FindData(kRGBColor, B_RGB_COLOR_TYPE,
				(const void**)&color, &size) == B_OK) {
			BMessage setColorMessage(SET_CURRENT_COLOR);
			setColorMessage.AddData(kRGBColor, B_RGB_COLOR_TYPE, color,
				sizeof(rgb_color));
			Invoke(&setColorMessage);
		}
	} else if ((int32)message->what == kMsgMessageRunner) {
		BPoint where;
		uint32 buttons;
		GetMouse(&where, &buttons);

		_DragColor(where);
	}

	BControl::MessageReceived(message);
}


void
ColorPreview::MouseDown(BPoint where)
{
	BWindow* window = Window();
	if (window != NULL)
		window->Activate();

	fMessageRunner = new BMessageRunner(this, new BMessage(kMsgMessageRunner),
		300000, 1);

	SetMouseEventMask(B_POINTER_EVENTS,
		B_SUSPEND_VIEW_FOCUS | B_LOCK_WINDOW_FOCUS);

	BRect rect = Bounds().InsetByCopy(2.0f, 2.0f);
	rect.top = roundf(rect.bottom / 2.0f + 1);

	if (rect.Contains(where)) {
		Invalidate();
		Invoke();
	}

	BControl::MouseDown(where);
}


void
ColorPreview::MouseMoved(BPoint where, uint32 transit, const BMessage* message)
{
	if (fMessageRunner != NULL)
		_DragColor(where);

	BControl::MouseMoved(where, transit, message);
}


void
ColorPreview::MouseUp(BPoint where)
{
	delete fMessageRunner;
	fMessageRunner = NULL;

	BControl::MouseUp(where);
}


rgb_color
ColorPreview::Color(void) const
{
	return fColor;
}


void
ColorPreview::SetColor(rgb_color color)
{
	color.alpha = 255;

	SetHighColor(color);
	fColor = color;

	Invalidate();
	Invoke();
}


void
ColorPreview::SetColor(uint8 red, uint8 green, uint8 blue)
{
	SetHighColor(red, green, blue);
	fColor.red = red;
	fColor.green = green;
	fColor.blue = blue;
	fColor.alpha = 255;

	Invalidate();
	Invoke();
}


void
ColorPreview::SetMode(bool rectangle)
{
	fIsRectangle = rectangle;
}


//	#pragma mark - ColorPreview private methods


void
ColorPreview::_DragColor(BPoint where)
{
	BString hexStr;
	hexStr.SetToFormat("#%.2X%.2X%.2X", fColor.red, fColor.green, fColor.blue);

	BMessage message(B_PASTE);
	message.AddData("text/plain", B_MIME_TYPE, hexStr.String(),
		hexStr.Length());
	message.AddData(kRGBColor, B_RGB_COLOR_TYPE, &fColor, sizeof(fColor));

	BRect rect(0.0f, 0.0f, 20.0f, 20.0f);

	BBitmap* bitmap = new BBitmap(rect, B_RGB32, true);
	if (bitmap->Lock()) {
		BView* view = new BView(rect, "", B_FOLLOW_NONE, B_WILL_DRAW);
		bitmap->AddChild(view);

		view->SetHighColor(B_TRANSPARENT_COLOR);
		view->FillRect(view->Bounds());

		++rect.top;
		++rect.left;

		view->SetHighColor(0, 0, 0, 100);
		view->FillRect(rect);
		rect.OffsetBy(-1.0f, -1.0f);

		view->SetHighColor(std::min(255, (int)(1.2 * fColor.red + 40)),
			std::min(255, (int)(1.2 * fColor.green + 40)),
			std::min(255, (int)(1.2 * fColor.blue + 40)));
		view->StrokeRect(rect);

		++rect.left;
		++rect.top;

		view->SetHighColor((int32)(0.8 * fColor.red),
			(int32)(0.8 * fColor.green),
			(int32)(0.8 * fColor.blue));
		view->StrokeRect(rect);

		--rect.right;
		--rect.bottom;

		view->SetHighColor(fColor.red, fColor.green, fColor.blue);
		view->FillRect(rect);
		view->Sync();

		bitmap->Unlock();
	}

	DragMessage(&message, bitmap, B_OP_ALPHA, BPoint(14.0f, 14.0f));

	MouseUp(where);
}
