/* 
 * Copyright 2001 Werner Freytag - please read to the LICENSE file
 *
 * Copyright 2002-2006, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved.
 *		
 */

#include "ColorPreview.h"

#include <stdio.h>

#include <Bitmap.h>
#include <Cursor.h>
#include <MessageRunner.h>
#include <String.h>
#include <Window.h>

#include "cursors.h"
#include "support_ui.h"

// constructor
ColorPreview::ColorPreview(BRect frame, rgb_color color)
	: BControl(frame, "colorpreview", "", new BMessage(MSG_COLOR_PREVIEW),
			   B_FOLLOW_TOP|B_FOLLOW_LEFT, B_WILL_DRAW),
	  fColor(color),
	  fOldColor(color),

	  fMouseDown(false),

	  fMessageRunner(0)
{
}

// AttachedToWindow
void
ColorPreview::AttachedToWindow()
{
	BControl::AttachedToWindow();
	SetViewColor(B_TRANSPARENT_COLOR);
}

// Draw
void
ColorPreview::Draw(BRect updateRect)
{
	rgb_color background = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color shadow = tint_color(background, B_DARKEN_1_TINT);
	rgb_color darkShadow = tint_color(background, B_DARKEN_3_TINT);
	rgb_color light = tint_color(background, B_LIGHTEN_MAX_TINT);

	BRect r(Bounds());
	stroke_frame(this, r, shadow, shadow, light, light);
	r.InsetBy(1.0, 1.0);
	stroke_frame(this, r, darkShadow, darkShadow, background, background);
	r.InsetBy(1.0, 1.0);

	r.bottom = r.top + r.Height() / 2.0;
	SetHighColor(fColor);
	FillRect(r);

	r.top = r.bottom + 1;
	r.bottom = Bounds().bottom - 2.0;
	SetHighColor(fOldColor);
	FillRect(r);
}

// MessageReceived
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

// MouseDown
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

// MouseUp			
void
ColorPreview::MouseUp(BPoint where)
{
	delete fMessageRunner;
	fMessageRunner = NULL;

	fMouseDown = false;
	BControl::MouseUp(where);
}

// MouseMoved
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

// Invoke
status_t
ColorPreview::Invoke(BMessage* message)
{
	if (!message)
		message = Message();

	if (message) {
		message->RemoveName("color");
		message->AddData("color", B_RGB_COLOR_TYPE, &fColor, sizeof(fColor));
	}
	
	return BControl::Invoke(message);
}

// SetColor
void
ColorPreview::SetColor(rgb_color color)
{
	color.alpha = 255;
	fColor = color;

	Invalidate();
}

// SetNewColor
void
ColorPreview::SetNewColor(rgb_color color)
{
	fColor = color;
	fOldColor = color;

	Invalidate();
}

// #pragma mark -

// _DragColor
void
ColorPreview::_DragColor(BPoint where)
{
	BBitmap* bitmap = new BBitmap(BRect(0.0, 0.0, 15.0, 15.0), B_RGB32);
	BMessage message = make_color_drop_message(fColor, bitmap);

	DragMessage(&message, bitmap, B_OP_ALPHA, BPoint(9.0, 9.0));

	MouseUp(where);
}

