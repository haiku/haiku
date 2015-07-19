/* 
 * Copyright 2001 Werner Freytag - please read to the LICENSE file
 *
 * Copyright 2002-2015, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved.
 *		
 */

#include "ColorPreview.h"

#include <stdio.h>

#include <Bitmap.h>
#include <ControlLook.h>
#include <Cursor.h>
#include <LayoutUtils.h>
#include <MessageRunner.h>
#include <String.h>
#include <Window.h>

#include "cursors.h"
#include "support_ui.h"


ColorPreview::ColorPreview(BRect frame, rgb_color color)
	:
	BControl(frame, "colorpreview", "", new BMessage(MSG_COLOR_PREVIEW),
		B_FOLLOW_TOP | B_FOLLOW_LEFT, B_WILL_DRAW),
	fColor(color),
	fOldColor(color),

	fMouseDown(false),

	fMessageRunner(0),
	fBorderStyle(B_FANCY_BORDER)
{
}


ColorPreview::ColorPreview(rgb_color color)
	:
	BControl("colorpreview", "", new BMessage(MSG_COLOR_PREVIEW), B_WILL_DRAW),
	fColor(color),
	fOldColor(color),

	fMouseDown(false),

	fMessageRunner(0),
	fBorderStyle(B_FANCY_BORDER)
{
}


BSize
ColorPreview::MinSize()
{
	BSize minSize(32, 36);
	return BLayoutUtils::ComposeSize(ExplicitMinSize(), minSize);
}


BSize
ColorPreview::PreferredSize()
{
	BSize preferredSize(66, 70);
	return BLayoutUtils::ComposeSize(ExplicitPreferredSize(), preferredSize);
}


BSize
ColorPreview::MaxSize()
{
	BSize maxSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED);
	return BLayoutUtils::ComposeSize(ExplicitMaxSize(), maxSize);
}


void
ColorPreview::AttachedToWindow()
{
	BControl::AttachedToWindow();
	SetViewColor(B_TRANSPARENT_COLOR);
}


void
ColorPreview::Draw(BRect updateRect)
{
	BRect bounds(Bounds());

	// Frame
	if (fBorderStyle == B_FANCY_BORDER) {
		rgb_color color = LowColor();
		be_control_look->DrawTextControlBorder(this, bounds, updateRect, color);
	}
	
	BRect r(bounds.left, bounds.top, bounds.right,
		bounds.top + bounds.Height() / 2);
	SetHighColor(fColor);
	FillRect(r);

	r.top = r.bottom + 1;
	r.bottom = bounds.bottom;
	SetHighColor(fOldColor);
	FillRect(r);
}


void
ColorPreview::MessageReceived(BMessage* message)
{
	if (message->what == MSG_MESSAGERUNNER) {

		BPoint	where;
		uint32	buttons;

		GetMouse(&where, &buttons);

		_DragColor(where);

	} else {
#ifdef HAIKU_TARGET_PLATFORM_DANO
		const
#endif
		char* nameFound;
		type_code typeFound;

		if (message->GetInfo(B_RGB_COLOR_TYPE, 0,
							 &nameFound, &typeFound) != B_OK) {
			BControl::MessageReceived(message);
			return;
		}
		
	   	rgb_color* color;
		ssize_t numBytes;
		message->FindData(nameFound, typeFound,
						  (const void**)&color, &numBytes);
		
		BPoint where;
		bool droppedOnNewArea = false;
		if (message->FindPoint("_drop_point_", &where) == B_OK) {
			ConvertFromScreen(&where);
			if (where.y > Bounds().top + (Bounds().IntegerHeight() >> 1))
				droppedOnNewArea = true;
		}
	
		if (droppedOnNewArea)
			SetNewColor(*color);
		else
			SetColor(*color);
		Invoke();
	}
}


void
ColorPreview::MouseDown(BPoint where)
{
	Window()->Activate();
	
	fMouseDown = true;

	fMessageRunner = new BMessageRunner(
		this, new BMessage(MSG_MESSAGERUNNER), 300000, 1);

	SetMouseEventMask(B_POINTER_EVENTS,
					  B_SUSPEND_VIEW_FOCUS | B_LOCK_WINDOW_FOCUS);

	BRect rect = Bounds().InsetByCopy(2.0, 2.0);
	rect.top = rect.bottom/2 + 1;
	
	if (rect.Contains( where ) ) {
		fColor = fOldColor;
		Draw( Bounds() );
		Invoke();
	}
	
}


void
ColorPreview::MouseUp(BPoint where)
{
	delete fMessageRunner;
	fMessageRunner = NULL;

	fMouseDown = false;
	BControl::MouseUp(where);
}


void 
ColorPreview::MouseMoved(BPoint where, uint32 transit, const BMessage* message)
{
	if (transit == B_ENTERED_VIEW) {
		BCursor cursor(kDropperCursor);
		SetViewCursor(&cursor, true);
	}
	if (fMouseDown)
		_DragColor(where);
}


status_t
ColorPreview::Invoke(BMessage* message)
{
	if (message == NULL)
		message = Message();

	if (message) {
		message->RemoveName("color");
		message->AddData("color", B_RGB_COLOR_TYPE, &fColor, sizeof(fColor));
	}
	
	return BControl::Invoke(message);
}


void
ColorPreview::SetColor(rgb_color color)
{
	color.alpha = 255;
	fColor = color;

	Invalidate();
}


void
ColorPreview::SetNewColor(rgb_color color)
{
	fColor = color;
	fOldColor = color;

	Invalidate();
}


// #pragma mark -


void
ColorPreview::_DragColor(BPoint where)
{
	BBitmap* bitmap = new BBitmap(BRect(0.0, 0.0, 15.0, 15.0), B_RGB32);
	BMessage message = make_color_drop_message(fColor, bitmap);

	DragMessage(&message, bitmap, B_OP_ALPHA, BPoint(9.0, 9.0));

	MouseUp(where);
}

