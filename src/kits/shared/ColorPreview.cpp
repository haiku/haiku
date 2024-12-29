/*
 * Copyright 2009-2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm, darkwyrm@earthlink.net
 *		John Scipione, jscipione@gmail.com
 */


#include <ColorPreview.h>

#include <stdio.h>

#include <algorithm>
#include <iostream>

#include <Application.h>
#include <Bitmap.h>
#include <Message.h>
#include <MessageRunner.h>
#include <String.h>
#include <Window.h>


static const int32 kMsgMessageRunner = 'MsgR';

static const rgb_color kRed = make_color(255, 0, 0, 255);


namespace BPrivate {


BColorPreview::BColorPreview(const char* name, const char* label, BMessage* message, uint32 flags)
	:
	BControl(name, label, message, flags),
	fColor(kRed),
	fMessageRunner(NULL)
{
	SetExplicitSize(BSize(roundf(StringWidth("M") * 6), roundf(StringWidth("M") * 6)));
	SetExplicitAlignment(BAlignment(B_ALIGN_HORIZONTAL_CENTER, B_ALIGN_VERTICAL_CENTER));
}


BColorPreview::~BColorPreview()
{
}


void
BColorPreview::Draw(BRect updateRect)
{
	rgb_color color = fColor;

	rgb_color bg = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color outer = tint_color(bg, B_DARKEN_1_TINT);
	rgb_color inner = tint_color(bg, B_DARKEN_3_TINT);
	rgb_color light = tint_color(bg, B_LIGHTEN_MAX_TINT);

	BRect bounds(Bounds());

	BeginLineArray(4);
	AddLine(BPoint(bounds.left, bounds.bottom), BPoint(bounds.left, bounds.top), outer);
	AddLine(BPoint(bounds.left + 1, bounds.top), BPoint(bounds.right, bounds.top), outer);
	AddLine(BPoint(bounds.right, bounds.top + 1), BPoint(bounds.right, bounds.bottom), light);
	AddLine(BPoint(bounds.right - 1, bounds.bottom), BPoint(bounds.left + 1, bounds.bottom),
		light);
	EndLineArray();

	bounds.InsetBy(1, 1);

	BeginLineArray(4);
	AddLine(BPoint(bounds.left, bounds.bottom), BPoint(bounds.left, bounds.top), inner);
	AddLine(BPoint(bounds.left + 1, bounds.top), BPoint(bounds.right, bounds.top), inner);
	AddLine(BPoint(bounds.right, bounds.top + 1), BPoint(bounds.right, bounds.bottom), bg);
	AddLine(BPoint(bounds.right - 1, bounds.bottom), BPoint(bounds.left + 1, bounds.bottom), bg);
	EndLineArray();

	bounds.InsetBy(1, 1);

	SetHighColor(color);
	FillRect(bounds);
}


status_t
BColorPreview::Invoke(BMessage* message)
{
	if (message == NULL)
		message = new BMessage(B_PASTE);

	if (message->CountNames(B_RGB_COLOR_TYPE) == 0)
		message->AddData("RGBColor", B_RGB_COLOR_TYPE, &fColor, sizeof(fColor));

	return BControl::Invoke(message);
}


void
BColorPreview::MessageReceived(BMessage* message)
{
	if (message->WasDropped()) {
		char* name;
		type_code type;
		rgb_color* color;
		ssize_t size;
		if (message->GetInfo(B_RGB_COLOR_TYPE, 0, &name, &type) == B_OK
			&& message->FindData(name, type, (const void**)&color, &size) == B_OK) {
			SetColor(*color);
			Invoke(message);
		}
	}

	switch (message->what) {
		case kMsgMessageRunner:
		{
			BPoint where;
			uint32 buttons;
			GetMouse(&where, &buttons);

			_DragColor(where);
			break;
		}

		default:
			BControl::MessageReceived(message);
			break;
	}
}


void
BColorPreview::MouseDown(BPoint where)
{
	fMessageRunner = new BMessageRunner(this, new BMessage(kMsgMessageRunner), 300000, 1);

	BControl::MouseDown(where);
}


void
BColorPreview::MouseMoved(BPoint where, uint32 transit, const BMessage* message)
{
	if (fMessageRunner != NULL)
		_DragColor(where);

	BControl::MouseMoved(where, transit, message);
}


void
BColorPreview::MouseUp(BPoint where)
{
	delete fMessageRunner;
	fMessageRunner = NULL;

	BControl::MouseUp(where);
}


rgb_color
BColorPreview::Color() const
{
	return fColor;
}


void
BColorPreview::SetColor(rgb_color color)
{
	color.alpha = 255;
	fColor = color;

	Invalidate();
}


void
BColorPreview::_DragColor(BPoint where)
{
	BString hexStr;
	hexStr.SetToFormat("#%.2X%.2X%.2X", fColor.red, fColor.green, fColor.blue);

	BMessage message(B_PASTE);
	message.AddData("text/plain", B_MIME_TYPE, hexStr.String(), hexStr.Length());
	message.AddData("RGBColor", B_RGB_COLOR_TYPE, &fColor, sizeof(fColor));
	message.AddMessenger("be:sender", BMessenger(this));
	message.AddPointer("source", this);
	message.AddInt64("when", (int64)system_time());

	BRect rect(0, 0, 16, 16);

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
		rect.OffsetBy(-1, -1);

		int32 red = std::min(255, (int)(1.2 * fColor.red + 40));
		int32 green = std::min(255, (int)(1.2 * fColor.green + 40));
		int32 blue = std::min(255, (int)(1.2 * fColor.blue + 40));

		view->SetHighColor(red, green, blue);
		view->StrokeRect(rect);

		++rect.left;
		++rect.top;

		red = (int32)(0.8 * fColor.red);
		green = (int32)(0.8 * fColor.green);
		blue = (int32)(0.8 * fColor.blue);

		view->SetHighColor(red, green, blue);
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


} // namespace BPrivate
