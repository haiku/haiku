/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "AlphaSlider.h"

#include <stdio.h>

#include <AppDefs.h>
#include <Bitmap.h>
#include <Message.h>
#include <Window.h>

#include "ui_defines.h"
#include "support_ui.h"

// constructor
AlphaSlider::AlphaSlider(orientation dir, BMessage* message)
	: BControl(dir == B_HORIZONTAL ? BRect(0, 0, 255 + 4, 7 + 4)
								   : BRect(0, 0, 7 + 4, 255 + 4),
			   "alpha slider", NULL, message,
								   
			   B_FOLLOW_NONE,
			   B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE),

	  fBitmap(NULL),
	  fColor(kBlack),
	  fDragging(false),
	  fOrientation(dir)
{
	FrameResized(Bounds().Width(), Bounds().Height());

	SetViewColor(B_TRANSPARENT_32_BIT);
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	SetValue(255);
}

// destructor
AlphaSlider::~AlphaSlider()
{
	delete fBitmap;
}

#if LIB_LAYOUT
// layoutprefs
minimax
AlphaSlider::layoutprefs()
{
	mpm.mini.x = 256 + 4;
	mpm.maxi.x = mpm.mini.x + 10000;
	mpm.mini.y = 8 + 4;
	mpm.maxi.y = mpm.mini.y + 10;

	mpm.weight = 1.0;

	return mpm;
}

// layout
BRect
AlphaSlider::layout(BRect frame)
{
	MoveTo(frame.LeftTop());
	ResizeTo(frame.Width(), frame.Height());
	return Frame();
}
#endif // LIB_LAYOUT

// WindowActivated
void
AlphaSlider::WindowActivated(bool active)
{
	if (IsFocus())
		Invalidate();
}

// MakeFocus
void
AlphaSlider::MakeFocus(bool focus)
{
	if (focus != IsFocus()) {
		BControl::MakeFocus(focus);
		Invalidate();
	}
}

// MouseDown
void
AlphaSlider::MouseDown(BPoint where)
{
	if (!IsEnabled())
		return;

//	if (!IsFocus())
//		MakeFocus(true);

	fDragging = true;
	SetValue(_ValueFor(where));

	SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
}

// MouseUp
void
AlphaSlider::MouseUp(BPoint where)
{
	fDragging = false;
}

// MouseMoved
void
AlphaSlider::MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage)
{
	if (!IsEnabled() || !fDragging)
		return;

	SetValue(_ValueFor(where));
}

// KeyDown
void
AlphaSlider::KeyDown(const char* bytes, int32 numBytes)
{
	if (!IsEnabled() || numBytes <= 0) {
		BControl::KeyDown(bytes, numBytes);
		return;
	}

	switch (bytes[0]) {
		case B_HOME:
			SetValue(0);
			break;
		case B_END:
			SetValue(255);
			break;

		case B_LEFT_ARROW:
		case B_UP_ARROW:
			SetValue(max_c(0, Value() - 1));
			break;
		case B_RIGHT_ARROW:
		case B_DOWN_ARROW:
			SetValue(min_c(255, Value() + 1));
			break;

		default:
			BControl::KeyDown(bytes, numBytes);
			break;
	}
}

// Draw
void
AlphaSlider::Draw(BRect updateRect)
{
	BRect b = _BitmapRect();
	b.InsetBy(-2.0, -2.0);

	bool isFocus = IsFocus() && Window()->IsActive();

	rgb_color bg = LowColor();
	rgb_color shadow;
	rgb_color darkShadow;
	rgb_color light;
	rgb_color black;

	if (IsEnabled()) {
		shadow = tint_color(bg, B_DARKEN_1_TINT);
		darkShadow = tint_color(bg, B_DARKEN_3_TINT);
		light = tint_color(bg, B_LIGHTEN_MAX_TINT);
		black = tint_color(bg, B_DARKEN_MAX_TINT);
	} else {
		shadow = bg;
		darkShadow = tint_color(bg, B_DARKEN_1_TINT);
		light = tint_color(bg, B_LIGHTEN_2_TINT);
		black = tint_color(bg, B_DARKEN_2_TINT);
	}

	rgb_color focus = isFocus ? ui_color(B_KEYBOARD_NAVIGATION_COLOR)
							  : black;

	stroke_frame(this, b, shadow, shadow, light, light);
	b.InsetBy(1.0, 1.0);
	if (isFocus)
		stroke_frame(this, b, focus, focus, focus, focus);
	else
		stroke_frame(this, b, darkShadow, darkShadow, bg, bg);
	b.InsetBy(1.0, 1.0);

	DrawBitmap(fBitmap, b.LeftTop());

	// value marker
	if (fOrientation == B_HORIZONTAL) {
		float pos = floorf(b.left + Value() * b.Width() / 255.0 + 0.5);
	
		if (pos - 2 >= b.left) {
			SetHighColor(kWhite);
			StrokeLine(BPoint(pos - 2, b.top), BPoint(pos - 2, b.bottom));
		}
		if (pos - 1 >= b.left) {
			SetHighColor(kBlack);
			StrokeLine(BPoint(pos - 1, b.top), BPoint(pos - 1, b.bottom));
		}
		if (pos + 1 <= b.right) {
			SetHighColor(kBlack);
			StrokeLine(BPoint(pos + 1, b.top), BPoint(pos + 1, b.bottom));
		}
		if (pos + 2 <= b.right) {
			SetHighColor(kWhite);
			StrokeLine(BPoint(pos + 2, b.top), BPoint(pos + 2, b.bottom));
		}
	} else {
		float pos = floorf(b.top + Value() * b.Height() / 255.0 + 0.5);
	
		if (pos - 2 >= b.top) {
			SetHighColor(kWhite);
			StrokeLine(BPoint(b.left, pos - 2), BPoint(b.right, pos - 2));
		}
		if (pos - 1 >= b.top) {
			SetHighColor(kBlack);
			StrokeLine(BPoint(b.left, pos - 1), BPoint(b.right, pos - 1));
		}
		if (pos + 1 <= b.bottom) {
			SetHighColor(kBlack);
			StrokeLine(BPoint(b.left, pos + 1), BPoint(b.right, pos + 1));
		}
		if (pos + 2 <= b.bottom) {
			SetHighColor(kWhite);
			StrokeLine(BPoint(b.left, pos + 2), BPoint(b.right, pos + 2));
		}
	}
}

// FrameResized
void
AlphaSlider::FrameResized(float width, float height)
{
	BRect r = _BitmapRect();
	_AllocBitmap(r.IntegerWidth() + 1, r.IntegerHeight() + 1);
	_UpdateColors();
	Invalidate();
	
}

// SetValue
void
AlphaSlider::SetValue(int32 value)
{
	if (value == Value())
		return;

	Invoke(Message());
	BControl::SetValue(value);
}

// SetEnabled
void
AlphaSlider::SetEnabled(bool enabled)
{
	if (enabled == IsEnabled())
		return;

	BControl::SetEnabled(enabled);

	_UpdateColors();
	Invalidate();
}

// #pragma mark -

// SetColor
void
AlphaSlider::SetColor(const rgb_color& color)
{
	if ((uint32&)fColor == (uint32&)color)
		return;

	fColor = color;

	_UpdateColors();
	Invalidate();
}

// #pragma mark -

// blend_colors
inline void
blend_colors(uint8* d, uint8 alpha, uint8 c1, uint8 c2, uint8 c3)
{
	if (alpha > 0) {
		if (alpha == 255) {
			d[0] = c1;
			d[1] = c2;
			d[2] = c3;
		} else {
			d[0] += (uint8)(((c1 - d[0]) * alpha) >> 8);
			d[1] += (uint8)(((c2 - d[1]) * alpha) >> 8);
			d[2] += (uint8)(((c3 - d[2]) * alpha) >> 8);
		}
	}
}

// _UpdateColors
void
AlphaSlider::_UpdateColors()
{
	if (!fBitmap || !fBitmap->IsValid())
		return;

	// fill in top row with alpha gradient
	uint8* topRow = (uint8*)fBitmap->Bits();
	uint32 width = fBitmap->Bounds().IntegerWidth() + 1;

	uint8* p = topRow;
	rgb_color color = fColor;
	if (!IsEnabled()) {
		// blend low color and color to give disabled look
		rgb_color bg = LowColor();
		color.red = (uint8)(((uint32)bg.red + color.red) / 2);
		color.green = (uint8)(((uint32)bg.green + color.green) / 2);
		color.blue = (uint8)(((uint32)bg.blue + color.blue) / 2);
	}
	for (uint32 x = 0; x < width; x++) {
		p[0] = color.blue;
		p[1] = color.green;
		p[2] = color.red;
		p[3] = x * 255 / width;
		p += 4;
	}
	// copy top row to rest of bitmap
	uint32 height = fBitmap->Bounds().IntegerHeight() + 1;
	uint32 bpr = fBitmap->BytesPerRow();
	uint8* dstRow = topRow + bpr;
	for (uint32 i = 1; i < height; i++) {
		memcpy(dstRow, topRow, bpr);
		dstRow += bpr;
	}
	// post process bitmap to underlay it with a pattern to visualize alpha
	uint8* row = topRow;
	for (uint32 i = 0; i < height; i++) {
		uint8* p = row;
		for (uint32 x = 0; x < width; x++) {
			uint8 alpha = p[3];
			if (alpha < 255) {
				p[3] = 255;
				alpha = 255 - alpha;
				if (x % 8 >= 4) {
					if (i % 8 >= 4) {
						blend_colors(p, alpha,
									 kAlphaLow.blue,
									 kAlphaLow.green,
									 kAlphaLow.red);
					} else {
						blend_colors(p, alpha,
									 kAlphaHigh.blue,
									 kAlphaHigh.green,
									 kAlphaHigh.red);
					}
				} else {
					if (i % 8 >= 4) {
						blend_colors(p, alpha,
									 kAlphaHigh.blue,
									 kAlphaHigh.green,
									 kAlphaHigh.red);
					} else {
						blend_colors(p, alpha,
									 kAlphaLow.blue,
									 kAlphaLow.green,
									 kAlphaLow.red);
					}
				}
			}
			p += 4;
		}
		row += bpr;
	}
}

// _AllocBitmap
void
AlphaSlider::_AllocBitmap(int32 width, int32 height)
{
	if (width < 2 || height < 2)
		return;

	delete fBitmap;
	fBitmap = new BBitmap(BRect(0, 0, width - 1, height - 1), 0, B_RGB32);
}

// _BitmapRect
BRect
AlphaSlider::_BitmapRect() const
{
	BRect r = Bounds();
	r.InsetBy(2, 2);
	return r;
}

// _ValueFor
int32
AlphaSlider::_ValueFor(BPoint where) const
{
	BRect r = _BitmapRect();

	int32 value;
	if (fOrientation == B_HORIZONTAL)
		value = (int32)(255 * (where.x - r.left) / r.Width() + 0.5);
	else
		value = (int32)(255 * (where.y - r.top) / r.Height() + 0.5);

	value = max_c(0, value);
	value = min_c(255, value);

	return value;
}

