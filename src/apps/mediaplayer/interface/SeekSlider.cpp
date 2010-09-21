/*
 * Copyright 2006-2010 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "SeekSlider.h"

#include <stdio.h>
#include <string.h>

#include <ControlLook.h>
#include <Region.h>
#include <Shape.h>


static const rgb_color kThumbRed = (rgb_color){ 255, 52, 52, 255 };


SeekSlider::SeekSlider(const char* name, BMessage* message, int32 minValue,
		int32 maxValue)
	:
	BSlider(name, NULL, NULL, minValue, maxValue, B_HORIZONTAL,
		B_TRIANGLE_THUMB),
	fTracking(false),
	fLastTrackTime(0),
	fDisabledString(""),
	fScale(0.0f)
{
	BFont font(be_plain_font);
	font.SetSize(9.0);
	SetFont(&font);
	SetSymbolScale(1.0);
	rgb_color fillColor = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
		B_DARKEN_3_TINT);
	UseFillColor(true, &fillColor);
	SetModificationMessage(message);
}


SeekSlider::~SeekSlider()
{
}


status_t
SeekSlider::Invoke(BMessage* message)
{
	fLastTrackTime = system_time();
	return BSlider::Invoke(message);
}


BRect
SeekSlider::ThumbFrame() const
{
	BRect frame = BSlider::ThumbFrame();

	float center = (frame.left + frame.right) / 2.0f;
	float height = ceilf(frame.Height() * fScale);
	float width = ceilf(frame.Width() * fScale);

	frame.left = floorf(center - width / 2) + 1;
	frame.right = frame.left + width;
	frame.bottom = frame.top + height;

	return frame;
}


void
SeekSlider::DrawBar()
{
	BSlider::DrawBar();
	if (IsEnabled())
		return;

	BRect r(BarFrame());
	font_height fh;
	GetFontHeight(&fh);
	float width = ceilf(StringWidth(fDisabledString.String()));
	BPoint textPos;
	textPos.x = r.left + (r.Width() - width) / 2.0;
	textPos.y = (r.top + r.bottom - ceilf(fh.ascent + fh.descent)) / 2.0
		+ ceilf(fh.ascent);

	SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
		B_DARKEN_3_TINT));
	SetDrawingMode(B_OP_OVER);
	DrawString(fDisabledString.String(), textPos);
}


void
SeekSlider::DrawThumb()
{
	if (!IsEnabled())
		return;

	BRect frame = ThumbFrame();
	be_control_look->DrawSliderTriangle(this, frame, frame, kThumbRed, 0,
		B_HORIZONTAL);
}


void
SeekSlider::MouseDown(BPoint where)
{
	if (IsEnabled())
		fTracking = true;
	BSlider::MouseDown(where);
}


void
SeekSlider::MouseUp(BPoint where)
{
	fTracking = false;
	BSlider::MouseUp(where);
}


void
SeekSlider::GetPreferredSize(float* _width, float* _height)
{
	BSlider::GetPreferredSize(_width, _height);
	if (_width != NULL) {
		float minWidth = 15.0 + StringWidth(fDisabledString.String()) + 15.0;
		*_width = max_c(*_width, minWidth);
	}
	if (_height != NULL) {
		BRect unscaledThumbFrame = BSlider::ThumbFrame();
		BRect scaledThumbFrame = ThumbFrame();
		*_height += scaledThumbFrame.Height() - unscaledThumbFrame.Height();
	}
}


BSize
SeekSlider::MinSize()
{
	BSize size = BSlider::MinSize();

	BRect unscaledThumbFrame = BSlider::ThumbFrame();
	BRect scaledThumbFrame = ThumbFrame();
	size.height += scaledThumbFrame.Height() - unscaledThumbFrame.Height();

	return size;
}


BSize
SeekSlider::MaxSize()
{
	BSize size = BSlider::MaxSize();

	BRect unscaledThumbFrame = BSlider::ThumbFrame();
	BRect scaledThumbFrame = ThumbFrame();
	size.height += scaledThumbFrame.Height() - unscaledThumbFrame.Height();

	return size;
}


bool
SeekSlider::IsTracking() const
{
	if (fTracking)
		return true;
	return system_time() - fLastTrackTime < 250000;
}


void
SeekSlider::SetDisabledString(const char* string)
{
	if (string == NULL)
		string = "";

	if (fDisabledString == string)
		return;

	fDisabledString = string;

	if (!IsEnabled())
		Invalidate();
}


void
SeekSlider::SetSymbolScale(float scale)
{
	if (scale == fScale)
		return;

	fScale = scale;
	SetBarThickness(fScale * 15.0);
	InvalidateLayout();
}


