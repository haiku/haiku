/*
 * Copyright 2001 Werner Freytag - please read to the LICENSE file
 *
 * Copyright 2002-2006, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved.
 *
 */

#include "ColorField.h"

#include <stdio.h>

#include <Bitmap.h>
#include <ControlLook.h>
#include <LayoutUtils.h>
#include <OS.h>
#include <Region.h>
#include <Window.h>

#include "support_ui.h"

#include "rgb_hsv.h"

#define round(x) (int)(x +.5)

enum {
	MSG_UPDATE			= 'Updt',
};

#define MAX_X 255
#define MAX_Y 255

// constructor
ColorField::ColorField(BPoint offsetPoint, SelectedColorMode mode,
	float fixedValue, orientation orient, border_style border)
	: BControl(BRect(0.0, 0.0, MAX_X + 4.0, MAX_Y + 4.0)
			.OffsetToCopy(offsetPoint),
		"ColorField", "", new BMessage(MSG_COLOR_FIELD),
		B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_FRAME_EVENTS)
{
	_Init(mode, fixedValue, orient, border);
	FrameResized(Bounds().Width(), Bounds().Height());
}

// constructor
ColorField::ColorField(SelectedColorMode mode, float fixedValue,
	orientation orient, border_style border)
	: BControl("ColorField", "", new BMessage(MSG_COLOR_FIELD),
		B_WILL_DRAW | B_FRAME_EVENTS)
{
	_Init(mode, fixedValue, orient, border);
}

// destructor
ColorField::~ColorField()
{
	delete fBitmap;
}

// MinSize
BSize
ColorField::MinSize()
{
	BSize minSize;
	if (fOrientation == B_VERTICAL)
		minSize = BSize(4 + MAX_X / 17, 4 + MAX_Y / 5);
	else
		minSize = BSize(4 + MAX_X / 5, 4 + MAX_Y / 17);

	return BLayoutUtils::ComposeSize(ExplicitMinSize(), minSize);
}

// PreferredSize
BSize
ColorField::PreferredSize()
{
	return BLayoutUtils::ComposeSize(ExplicitPreferredSize(), MinSize());
}

// MaxSize
BSize
ColorField::MaxSize()
{
	BSize maxSize(4 + MAX_X, 4 + MAX_Y);
	return BLayoutUtils::ComposeSize(ExplicitMaxSize(), maxSize);
//	return BLayoutUtils::ComposeSize(ExplicitMaxSize(),
//		BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
}

// Invoke
status_t
ColorField::Invoke(BMessage* message)
{
	if (message == NULL)
		message = Message();

	if (message == NULL)
		return BControl::Invoke(message);

	message->RemoveName("value");

	float v1 = 0;
	float v2 = 0;

	switch (fMode) {
		case R_SELECTED:
		case G_SELECTED:
		case B_SELECTED:
			v1 = fMarkerPosition.x / Width();
			v2 = 1.0 - fMarkerPosition.y / Height();
			break;

		case H_SELECTED:
			if (fOrientation == B_VERTICAL) {
				v1 = fMarkerPosition.x / Width();
				v2 = 1.0 - fMarkerPosition.y / Height();
			} else {
				v1 = fMarkerPosition.y / Height();
				v2 = 1.0 - fMarkerPosition.x / Width();
			}
			break;

		case S_SELECTED:
		case V_SELECTED:
			v1 = fMarkerPosition.x / Width() * 6.0;
			v2 = 1.0 - fMarkerPosition.y / Height();
			break;

	}

	message->AddFloat("value", v1);
	message->AddFloat("value", v2);

	return BControl::Invoke(message);
}

// AttachedToWindow
void
ColorField::AttachedToWindow()
{
}

// Draw
void
ColorField::Draw(BRect updateRect)
{
	if (fBitmapDirty && fBitmap != NULL) {
		_FillBitmap(fBitmap, fMode, fFixedValue, fOrientation);
		fBitmapDirty = false;
	}

	BRect bounds = Bounds();

	// Frame
	if (fBorderStyle == B_FANCY_BORDER) {
		rgb_color color = LowColor();
		be_control_look->DrawTextControlBorder(this, bounds, updateRect,
			color);
		BRegion region(bounds);
		ConstrainClippingRegion(&region);
	}

	// Color field fill
	if (fBitmap != NULL)
		DrawBitmap(fBitmap, bounds.LeftTop());
	else {
		SetHighColor(255, 0, 0);
		FillRect(bounds);
	}

	// Marker
	SetHighColor(0, 0, 0);
	StrokeEllipse(fMarkerPosition + bounds.LeftTop(), 5.0, 5.0);
	SetHighColor(255.0, 255.0, 255.0);
	StrokeEllipse(fMarkerPosition + bounds.LeftTop(), 4.0, 4.0);
}

// FrameResized
void
ColorField::FrameResized(float width, float height)
{
	BRect r = _BitmapRect();
	_AllocBitmap(r.IntegerWidth() + 1, r.IntegerHeight() + 1);
	Invalidate();
}

// MouseDown
void
ColorField::MouseDown(BPoint where)
{
	fMouseDown = true;
	SetMouseEventMask(B_POINTER_EVENTS,
		B_SUSPEND_VIEW_FOCUS | B_LOCK_WINDOW_FOCUS);
	PositionMarkerAt(where);

	if (Message() != NULL) {
		BMessage message(*Message());
		message.AddBool("begin", true);
		Invoke(&message);
	} else
		Invoke();
}

// MouseUp
void
ColorField::MouseUp(BPoint where)
{
	fMouseDown = false;
}

// MouseMoved
void
ColorField::MouseMoved(BPoint where, uint32 transit,
	const BMessage* dragMessage)
{
	if (dragMessage != NULL || !fMouseDown ) {
		BView::MouseMoved(where, transit, dragMessage);
		return;
	}

	PositionMarkerAt(where);
	Invoke();
}

// SetModeAndValue
void
ColorField::SetModeAndValue(SelectedColorMode mode, float fixedValue)
{
	float R(0), G(0), B(0);
	float H(0), S(0), V(0);

	float width = Width();
	float height = Height();

	switch (fMode) {

		case R_SELECTED: {
			R = fFixedValue * 255;
			G = round(fMarkerPosition.x / width * 255.0);
			B = round(255.0 - fMarkerPosition.y / height * 255.0);
		}; break;

		case G_SELECTED: {
			R = round(fMarkerPosition.x / width * 255.0);
			G = fFixedValue * 255;
			B = round(255.0 - fMarkerPosition.y / height * 255.0);
		}; break;

		case B_SELECTED: {
			R = round(fMarkerPosition.x / width * 255.0);
			G = round(255.0 - fMarkerPosition.y / height * 255.0);
			B = fFixedValue * 255;
		}; break;

		case H_SELECTED: {
			H = fFixedValue;
			S = fMarkerPosition.x / width;
			V = 1.0 - fMarkerPosition.y / height;
		}; break;

		case S_SELECTED: {
			H = fMarkerPosition.x / width * 6.0;
			S = fFixedValue;
			V = 1.0 - fMarkerPosition.y / height;
		}; break;

		case V_SELECTED: {
			H = fMarkerPosition.x / width * 6.0;
			S = 1.0 - fMarkerPosition.y / height;
			V = fFixedValue;
		}; break;
	}

	if (fMode & (H_SELECTED | S_SELECTED | V_SELECTED)) {
		HSV_to_RGB(H, S, V, R, G, B);
		R *= 255.0; G *= 255.0; B *= 255.0;
	}

	rgb_color color = { (uint8)round(R), (uint8)round(G), (uint8)round(B), 
		255 };

	if (fFixedValue != fixedValue || fMode != mode) {
		fFixedValue = fixedValue;
		fMode = mode;

		_Update();
	}

	SetMarkerToColor(color);
}

// SetFixedValue
void
ColorField::SetFixedValue(float fixedValue)
{
	if (fFixedValue != fixedValue) {
		fFixedValue = fixedValue;
		_Update();
	}
}

// SetMarkerToColor
void
ColorField::SetMarkerToColor(rgb_color color)
{
	float h, s, v;
	RGB_to_HSV(color.red / 255.0, color.green / 255.0, color.blue / 255.0,
		h, s, v );

	fLastMarkerPosition = fMarkerPosition;

	float width = Width();
	float height = Height();

	switch (fMode) {
		case R_SELECTED:
			fMarkerPosition = BPoint(color.green / 255.0 * width,
				(255.0 - color.blue) / 255.0 * height);
			break;

		case G_SELECTED:
			fMarkerPosition = BPoint(color.red / 255.0 * width,
				(255.0 - color.blue) / 255.0 * height);
			break;

		case B_SELECTED:
			fMarkerPosition = BPoint(color.red / 255.0 * width,
				(255.0 - color.green) / 255.0 * height);
			break;

		case H_SELECTED:
			if (fOrientation == B_VERTICAL)
				fMarkerPosition = BPoint(s * width, height - v * height);
			else
				fMarkerPosition = BPoint(width - v * width, s * height);
			break;

		case S_SELECTED:
			fMarkerPosition = BPoint(h / 6.0 * width, height - v * height);
			break;

		case V_SELECTED:
			fMarkerPosition = BPoint( h / 6.0 * width, height - s * height);
			break;
	}

	Invalidate();
}

// PositionMarkerAt
void
ColorField::PositionMarkerAt(BPoint where)
{
	BRect rect = _BitmapRect();
	where.ConstrainTo(rect);
	where -= rect.LeftTop();

	fLastMarkerPosition = fMarkerPosition;
	fMarkerPosition = where;
	Invalidate();
}

// Width
float
ColorField::Width() const
{
	return _BitmapRect().IntegerWidth() + 1;
}

// Height
float
ColorField::Height() const
{
	return _BitmapRect().IntegerHeight() + 1;
}

// set_bits
static inline void
set_bits(uint8* bits, uint8 r, uint8 g, uint8 b)
{
	bits[0] = b;
	bits[1] = g;
	bits[2] = r;
	bits[3] = 255;
}

// _Init
void
ColorField::_Init(SelectedColorMode mode, float fixedValue,
	orientation orient, border_style border)
{
	fMode = mode;
	fFixedValue = fixedValue;
	fOrientation = orient;
	fBorderStyle = border;

	fMarkerPosition = BPoint(0.0, 0.0);
	fLastMarkerPosition = BPoint(-1.0, -1.0);
	fMouseDown = false;

	fBitmap = NULL;
	fBitmapDirty = true;

	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}

// _AllocBitmap
void
ColorField::_AllocBitmap(int32 width, int32 height)
{
	if (width < 2 || height < 2)
		return;

	delete fBitmap;
	fBitmap = new BBitmap(BRect(0, 0, width - 1, height - 1), 0, B_RGB32);

	fBitmapDirty = true;
}

// _Update
void
ColorField::_Update()
{
	fBitmapDirty = true;
	Invalidate();
}

// _BitmapRect
BRect
ColorField::_BitmapRect() const
{
	BRect r = Bounds();
	if (fBorderStyle == B_FANCY_BORDER)
		r.InsetBy(2, 2);
	return r;
}

// _FillBitmap
void
ColorField::_FillBitmap(BBitmap* bitmap, SelectedColorMode mode,
	float fixedValue, orientation orient) const
{
	int32 width = bitmap->Bounds().IntegerWidth();
	int32 height = bitmap->Bounds().IntegerHeight();
	uint32 bpr = bitmap->BytesPerRow();

//bigtime_t now = system_time();
	uint8* bits = (uint8*)bitmap->Bits();

	float r = 0;
	float g = 0;
	float b = 0;
	float h;
	float s;
	float v;

	switch (mode) {
		case R_SELECTED:
			r = round(fixedValue * 255);
			for (int y = height; y >= 0; y--) {
				uint8* bitsHandle = bits;
				b = y * 255 / height;
				for (int32 x = 0; x <= width; x++) {
					g = x * 255 / width;
					set_bits(bitsHandle, (uint8)r, (uint8)g, (uint8)b);
					bitsHandle += 4;
				}
				bits += bpr;
			}
			break;

		case G_SELECTED:
			g = round(fixedValue * 255);
			for (int y = height; y >= 0; y--) {
				uint8* bitsHandle = bits;
				b = y * 255 / height;
				for (int x = 0; x <= width; x++) {
					r = x * 255 / width;
					set_bits(bitsHandle, (uint8)r, (uint8)g, (uint8)b);
					bitsHandle += 4;
				}
				bits += bpr;
			}
			break;

		case B_SELECTED:
			b = round(fixedValue * 255);
			for (int y = height; y >= 0; y--) {
				uint8* bitsHandle = bits;
				g = y * 255 / height;
				for (int x = 0; x <= width; ++x) {
					r = x * 255 / width;
					set_bits(bitsHandle, (uint8)r, (uint8)g, (uint8)b);
					bitsHandle += 4;
				}
				bits += bpr;
			}
			break;

		case H_SELECTED:
			h = fixedValue;
			if (orient == B_VERTICAL) {
				for (int y = 0; y <= height; ++y) {
					v = (float)(height - y) / height;
					uint8* bitsHandle = bits;
					for (int x = 0; x <= width; ++x) {
						s = (float)x / width;
						HSV_to_RGB(h, s, v, r, g, b);
						set_bits(bitsHandle,
							round(r * 255.0),
							round(g * 255.0),
							round(b * 255.0));
						bitsHandle += 4;
					}
					bits += bpr;
				}
			} else {
				for (int y = 0; y <= height; ++y) {
					s = (float)y / height;
					uint8* bitsHandle = bits;
					for (int x = 0; x <= width; ++x) {
						v = (float)(width - x) / width;
						HSV_to_RGB(h, s, v, r, g, b);
						set_bits(bitsHandle,
							round(r * 255.0),
							round(g * 255.0),
							round(b * 255.0));
						bitsHandle += 4;
					}
					bits += bpr;
				}
			}
			break;

		case S_SELECTED:
			s = fixedValue;
			for (int y = 0; y <= height; ++y) {
				v = (float)(height - y) / height;
				uint8* bitsHandle = bits;
				for (int x = 0; x <= width; ++x) {
					h = 6.0 / width * x;
					HSV_to_RGB(h, s, v, r, g, b);
					set_bits(bitsHandle,
						round(r * 255.0),
						round(g * 255.0),
						round(b * 255.0));
					bitsHandle += 4;
				}
				bits += bpr;
			}
			break;

		case V_SELECTED:
			v = fixedValue;
			for (int y = 0; y <= height; ++y) {
				s = (float)(height - y) / height;
				uint8* bitsHandle = bits;
				for (int x = 0; x <= width; ++x) {
					h = 6.0 / width * x;
					HSV_to_RGB(h, s, v, r, g, b);
					set_bits(bitsHandle,
						round(r * 255.0),
						round(g * 255.0),
						round(b * 255.0));
					bitsHandle += 4;
				}
				bits += bpr;
			}
			break;
	}
}
