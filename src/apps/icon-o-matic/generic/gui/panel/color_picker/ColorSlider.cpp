/*
 * Copyright 2001 Werner Freytag - please read to the LICENSE file
 *
 * Copyright 2002-2015, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved.
 *
 */

#include "ColorSlider.h"

#include <stdio.h>

#include <Bitmap.h>
#include <ControlLook.h>
#include <LayoutUtils.h>
#include <OS.h>
#include <Window.h>
#include <math.h>

#include "support_ui.h"

#include "rgb_hsv.h"


#define round(x) (int)(x+.5)

enum {
	MSG_UPDATE			= 'Updt',
};

#define MAX_X 255
#define MAX_Y 255


ColorSlider::ColorSlider(SelectedColorMode mode,
	float value1, float value2, orientation dir, border_style border)
	:  BControl("ColorSlider", "", new BMessage(MSG_COLOR_SLIDER),
		B_WILL_DRAW | B_FRAME_EVENTS)
{
	_Init(mode, value1, value2, dir, border);
	FrameResized(Bounds().Width(), Bounds().Height());
}


ColorSlider::ColorSlider(BPoint offsetPoint, SelectedColorMode mode,
	float value1, float value2, orientation dir, border_style border)
	:  BControl(BRect(0.0, 0.0, 35.0, 265.0).OffsetToCopy(offsetPoint),
		"ColorSlider", "", new BMessage(MSG_COLOR_SLIDER),
		B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_FRAME_EVENTS)
{
	_Init(mode, value1, value2, dir, border);
	FrameResized(Bounds().Width(), Bounds().Height());
}


ColorSlider::~ColorSlider()
{
	delete fBitmap;
}


BSize
ColorSlider::MinSize()
{
	BSize minSize;
	if (fOrientation == B_VERTICAL)
		minSize = BSize(36, 10 + MAX_Y / 17);
	else
		minSize = BSize(10 + MAX_X / 17, 10);
	return BLayoutUtils::ComposeSize(ExplicitMinSize(), minSize);
}


BSize
ColorSlider::PreferredSize()
{
	return BLayoutUtils::ComposeSize(ExplicitPreferredSize(), MinSize());
}


BSize
ColorSlider::MaxSize()
{
	BSize maxSize;
	if (fOrientation == B_VERTICAL)
		maxSize = BSize(36, 10 + MAX_Y);
	else
		maxSize = BSize(10 + MAX_X, 18);
	return BLayoutUtils::ComposeSize(ExplicitMaxSize(), maxSize);
}


void
ColorSlider::AttachedToWindow()
{
}


status_t
ColorSlider::Invoke(BMessage* message)
{
	if (message == NULL)
		message = Message();

	if (message == NULL)
		return BControl::Invoke(message);

	message->RemoveName("value");
	message->RemoveName("begin");

	switch (fMode) {
		case R_SELECTED:
		case G_SELECTED:
		case B_SELECTED:
			message->AddFloat("value", 1.0 - (float)Value() / 255);
			break;

		case H_SELECTED:
			message->AddFloat("value", (1.0 - (float)Value() / 255) * 6);
			break;

		case S_SELECTED:
		case V_SELECTED:
			message->AddFloat("value", 1.0 - (float)Value() / 255);
			break;
	}

	// some other parts of WonderBrush rely on this.
	// if the flag is present, it triggers generating an undo action
	// fMouseDown is not set yet the first message is sent
	if (!fMouseDown)
		message->AddBool("begin", true);

	return BControl::Invoke(message);
}


void
ColorSlider::Draw(BRect updateRect)
{
	if (fBitmapDirty && fBitmap != NULL) {
		_FillBitmap(fBitmap, fMode, fFixedValue1, fFixedValue2, fOrientation);
		fBitmapDirty = false;
	}

	BRect bounds = _BitmapRect();

	// Frame
	if (fBorderStyle == B_FANCY_BORDER) {
		bounds.InsetBy(-2, -2);
		rgb_color color = LowColor();
		be_control_look->DrawTextControlBorder(this, bounds, updateRect,
			color);
	}

	// Color slider fill
	if (fBitmap != NULL)
		DrawBitmap(fBitmap, bounds.LeftTop());
	else {
		SetHighColor(255, 0, 0);
		FillRect(bounds);
	}

	// Marker background
	if (fOrientation == B_VERTICAL) {
		bounds.InsetBy(-2, -2);
		BRect r = Bounds();
		FillRect(BRect(r.left, r.top, bounds.left - 1, r.bottom),
			B_SOLID_LOW);
		FillRect(BRect(bounds.right + 1, r.top, r.right, r.bottom),
			B_SOLID_LOW);
		FillRect(
			BRect(bounds.left, r.top, bounds.right, bounds.top - 1),
			B_SOLID_LOW);
		FillRect(
			BRect(bounds.left, bounds.bottom + 1, bounds.right, r.bottom),
			B_SOLID_LOW);
	}

	// marker
	if (fOrientation == B_VERTICAL) {
		// draw the triangle markers
		SetHighColor(0, 0, 0);
		BRect r = Bounds();
		float offset = Value() * (r.Height() - 10) / 255.0;
		_DrawTriangle(
			BPoint(r.left, offset),
			BPoint(r.left + 5.0, offset + 5.0),
			BPoint(r.left, offset + 10.0));

		_DrawTriangle(
			BPoint(r.right, offset),
			BPoint(r.right - 5.0, offset + 5.0),
			BPoint(r.right, offset + 10.0));
	} else {
		BRect r = bounds;
		float x = bounds.left - 2 + (255 - Value()) * bounds.Width() / 255.0;
		if (x > r.left) {
			SetHighColor(255, 255, 255);
			StrokeLine(BPoint(x, bounds.top),
				BPoint(x, bounds.bottom));
		}
		if (x + 1 > r.left) {
			SetHighColor(0, 0, 0);
			StrokeLine(BPoint(x + 1, bounds.top),
				BPoint(x + 1, bounds.bottom));
		}
		if (x + 3 < r.right) {
			SetHighColor(0, 0, 0);
			StrokeLine(BPoint(x + 3, bounds.top),
				BPoint(x + 3, bounds.bottom));
		}
		if (x + 4 < r.right) {
			SetHighColor(255, 255, 255);
			StrokeLine(BPoint(x + 4, bounds.top),
				BPoint(x + 4, bounds.bottom));
		}
	}
}


void
ColorSlider::FrameResized(float width, float height)
{
	BRect r = _BitmapRect();
	_AllocBitmap(r.IntegerWidth() + 1, r.IntegerHeight() + 1);
	Invalidate();
}


void
ColorSlider::MouseDown(BPoint where)
{
	SetMouseEventMask(B_POINTER_EVENTS,
		B_SUSPEND_VIEW_FOCUS | B_LOCK_WINDOW_FOCUS);
	_TrackMouse(where);
	fMouseDown = true;
}


void
ColorSlider::MouseUp(BPoint where)
{
	fMouseDown = false;
}


void
ColorSlider::MouseMoved( BPoint where, uint32 code,
	const BMessage* dragMessage)
{
	if (dragMessage != NULL || !fMouseDown)
		return;

	_TrackMouse(where);
}


void
ColorSlider::SetValue(int32 value)
{
	value = max_c(min_c(value, 255), 0);
	if (value != Value()) {
		BControl::SetValue(value);
		Invalidate();
	}
}


void
ColorSlider::SetModeAndValues(SelectedColorMode mode,
							  float value1, float value2)
{
	float R = 0;
	float G = 0;
	float B = 0;
	float h = 0;
	float s = 0;
	float v = 0;

	switch (fMode) {
		case R_SELECTED:
			R = 255 - Value();
			G = round(fFixedValue1 * 255.0);
			B = round(fFixedValue2 * 255.0);
			break;

		case G_SELECTED:
			R = round(fFixedValue1 * 255.0);
			G = 255 - Value();
			B = round(fFixedValue2 * 255.0);
			break;

		case B_SELECTED:
			R = round(fFixedValue1 * 255.0);
			G = round(fFixedValue2 * 255.0);
			B = 255 - Value();
			break;

		case H_SELECTED:
			h = (1.0 - (float)Value()/255.0)*6.0;
			s = fFixedValue1;
			v = fFixedValue2;
			break;

		case S_SELECTED:
			h = fFixedValue1;
			s = 1.0 - (float)Value()/255.0;
			v = fFixedValue2;
			break;

		case V_SELECTED:
			h = fFixedValue1;
			s = fFixedValue2;
			v = 1.0 - (float)Value()/255.0;
			break;
	}

	if ((fMode & (H_SELECTED | S_SELECTED | V_SELECTED)) != 0) {
		HSV_to_RGB(h, s, v, R, G, B);
		R *= 255.0;
		G *= 255.0;
		B *= 255.0;
	}

	rgb_color color = { (uint8)round(R), (uint8)round(G), (uint8)round(B), 
		255 };

	fMode = mode;
	SetOtherValues(value1, value2);

	SetMarkerToColor(color);
	_Update();
}


void
ColorSlider::SetOtherValues(float value1, float value2)
{
	fFixedValue1 = value1;
	fFixedValue2 = value2;

	if (fMode != H_SELECTED)
		_Update();
}


void
ColorSlider::GetOtherValues(float* value1, float* value2) const
{
	if (value1 != NULL)
		*value1 = fFixedValue1;

	if (value2 != NULL)
		*value2 = fFixedValue2;
}


void
ColorSlider::SetMarkerToColor(rgb_color color)
{
	float h = 0;
	float s = 0;
	float v = 0;
	if ((fMode & (H_SELECTED | S_SELECTED | V_SELECTED)) != 0) {
		RGB_to_HSV(
			(float)color.red / 255.0,
			(float)color.green / 255.0,
			(float)color.blue / 255.0,
			h, s, v
		);
	}

	switch (fMode) {
		case R_SELECTED:
			SetValue(255 - color.red);
			break;

		case G_SELECTED:
			SetValue(255 - color.green);
			break;

		case B_SELECTED:
			SetValue(255 - color.blue);
			break;

		case H_SELECTED:
			SetValue(255.0 - round(h / 6.0 * 255.0));
			break;

		case S_SELECTED:
			SetValue(255.0 - round(s * 255.0));
			break;

		case V_SELECTED:
			SetValue(255.0 - round(v * 255.0));
			break;
	}
}


// #pragma mark - private


void
ColorSlider::_Init(SelectedColorMode mode, float value1, float value2,
	orientation dir, border_style border)
{
	fMode = mode;
	fFixedValue1 = value1;
	fFixedValue2 = value2;
	fMouseDown = false;
	fBitmap = NULL;
	fBitmapDirty = true;
	fOrientation = dir;
	fBorderStyle = border;

	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


void
ColorSlider::_AllocBitmap(int32 width, int32 height)
{
	if (width < 2 || height < 2)
		return;

	delete fBitmap;
	fBitmap = new BBitmap(BRect(0, 0, width - 1, height - 1), 0, B_RGB32);

	fBitmapDirty = true;
}


void
ColorSlider::_Update()
{
	fBitmapDirty = true;
	Invalidate();
}


BRect
ColorSlider::_BitmapRect() const
{
	BRect r = Bounds();
	if (fOrientation == B_VERTICAL)
		r.InsetBy(7, 3);
	if (fBorderStyle == B_FANCY_BORDER)
		r.InsetBy(2, 2);
	return r;
}


void
ColorSlider::_FillBitmap(BBitmap* bitmap, SelectedColorMode mode,
	float fixedValue1, float fixedValue2, orientation orient) const
{
	int32 width = bitmap->Bounds().IntegerWidth();
	int32 height = bitmap->Bounds().IntegerHeight();
	uint32 bpr = bitmap->BytesPerRow();

	uint8* bits = (uint8*)bitmap->Bits();

	float r = 0;
	float g = 0;
	float b = 0;
	float h;
	float s;
	float v;

	switch (mode) {
		case R_SELECTED:
			g = round(fixedValue1 * 255);
			b = round(fixedValue2 * 255);
			if (orient == B_VERTICAL) {
				for (int y = 0; y <= height; y++) {
					r = 255 - y * 255 / height;
					_DrawColorLineY(bits, width, r, g, b);
					bits += bpr;
				}
			} else {
				for (int x = 0; x <= width; x++) {
					r = x * 255 / width;
					_DrawColorLineX(bits, height, bpr, r, g, b);
					bits += 4;
				}
			}
			break;

		case G_SELECTED:
			r = round(fixedValue1 * 255);
			b = round(fixedValue2 * 255);
			if (orient == B_VERTICAL) {
				for (int y = 0; y <= height; y++) {
					g = 255 - y * 255 / height;
					_DrawColorLineY(bits, width, r, g, b);
					bits += bpr;
				}
			} else {
				for (int x = 0; x <= width; x++) {
					g = 255 - x * 255 / width;
					_DrawColorLineX(bits, height, bpr, r, g, b);
					bits += 4;
				}
			}
			break;

		case B_SELECTED:
			r = round(fixedValue1 * 255);
			g = round(fixedValue2 * 255);
			if (orient == B_VERTICAL) {
				for (int y = 0; y <= height; y++) {
					b = 255 - y * 255 / height;
					_DrawColorLineY(bits, width, r, g, b);
					bits += bpr;
				}
			} else {
				for (int x = 0; x <= width; x++) {
					b = x * 255 / width;
					_DrawColorLineX(bits, height, bpr, r, g, b);
					bits += 4;
				}
			}

		case H_SELECTED:
			s = 1.0;//fixedValue1;
			v = 1.0;//fixedValue2;
			if (orient == B_VERTICAL) {
				for (int y = 0; y <= height; y++) {
					HSV_to_RGB(6.0 - (float)y * 6.0 / height, s, v, r, g, b);
					_DrawColorLineY(bits, width, r * 255, g * 255, b * 255);
					bits += bpr;
				}
			} else {
				for (int x = 0; x <= width; x++) {
					HSV_to_RGB((float)x * 6.0 / width, s, v, r, g, b);
					_DrawColorLineX(bits, height, bpr,
						r * 255, g * 255, b * 255);
					bits += 4;
				}
			}
			break;

		case S_SELECTED:
			h = fixedValue1;
			v = 1.0;//fixedValue2;
			if (orient == B_VERTICAL) {
				for (int y = 0; y <= height; y++) {
					HSV_to_RGB(h, 1.0 - (float)y / height, v, r, g, b);
					_DrawColorLineY(bits, width, r * 255, g * 255, b * 255);
					bits += bpr;
				}
			} else {
				for (int x = 0; x <= width; x++) {
					HSV_to_RGB(h, 1.0 - (float)x / width, v, r, g, b);
					_DrawColorLineX(bits, height, bpr,
						r * 255, g * 255, b * 255);
					bits += 4;
				}
			}
			break;

		case V_SELECTED:
			h = fixedValue1;
			s = 1.0;//fixedValue2;
			if (orient == B_VERTICAL) {
				for (int y = 0; y <= height; y++) {
					HSV_to_RGB(h, s, 1.0 - (float)y / height, r, g, b);
					_DrawColorLineY(bits, width, r * 255, g * 255, b * 255);
					bits += bpr;
				}
			} else {
				for (int x = 0; x <= width; x++) {
					HSV_to_RGB(h, s, (float)x / width, r, g, b);
					_DrawColorLineX(bits, height, bpr,
						r * 255, g * 255, b * 255);
					bits += 4;
				}
			}
			break;
	}
}


void
ColorSlider::_DrawColorLineY(uint8* bits, int width, int r, int g, int b)
{
	for (int x = 0; x <= width; x++) {
		bits[0] = b;
		bits[1] = g;
		bits[2] = r;
		bits[3] = 255;
		bits += 4;
	}
}


void
ColorSlider::_DrawColorLineX(uint8* bits, int height, int bpr,
	int r, int g, int b)
{
	for (int y = 0; y <= height; y++) {
		bits[0] = b;
		bits[1] = g;
		bits[2] = r;
		bits[3] = 255;
		bits += bpr;
	}
}


void
ColorSlider::_DrawTriangle(BPoint point1, BPoint point2, BPoint point3)
{
	StrokeLine(point1, point2);
	StrokeLine(point3);
	StrokeLine(point1);
}


void
ColorSlider::_TrackMouse(BPoint where)
{
	BRect b(_BitmapRect());

	if (fOrientation == B_VERTICAL)
		SetValue((int)(((where.y - b.top) / b.Height()) * 255.0));
	else
		SetValue(255 - (int)(((where.x - b.left) / b.Width()) * 255.0));

	Invoke();
}
