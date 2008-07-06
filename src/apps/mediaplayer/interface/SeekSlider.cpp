/*
 * Copyright © 2006-2008 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */

// NOTE: Based on my code in the BeOS interface for the VLC media player
// that I did during the VLC 0.4.3 - 0.4.6 times. Code not written by me
// removed. -Stephan Aßmus

#include "SeekSlider.h"

#include <stdio.h>
#include <string.h>

#include "DrawingTidbits.h"


#define SEEK_SLIDER_KNOB_WIDTH 8.0

const rgb_color kSeekGreen = (rgb_color){ 171, 221, 161, 255 };
const rgb_color kSeekGreenShadow = (rgb_color){ 144, 186, 136, 255 };
const rgb_color kSeekRed = (rgb_color){ 255, 0, 0, 255 };
const rgb_color kSeekRedLight = (rgb_color){ 255, 152, 152, 255 };
const rgb_color kSeekRedShadow = (rgb_color){ 178, 0, 0, 255 };

const char* kDisabledSeekMessage = "Drop files to play";


SeekSlider::SeekSlider(BRect frame, const char* name, BMessage* message,
					   int32 minValue, int32 maxValue)
	: BControl(frame, name, NULL, message, B_FOLLOW_LEFT | B_FOLLOW_TOP,
			   B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_FRAME_EVENTS)
	, fTracking(false)
	, fLastTrackTime(0)
	, fKnobPos(_KnobPosFor(Bounds(), Value()))
	, fMinValue(minValue)
	, fMaxValue(maxValue)
{
	BFont font(be_plain_font);
	font.SetSize(9.0);
	SetFont(&font);
}


SeekSlider::~SeekSlider()
{
}


void
SeekSlider::AttachedToWindow()
{
	BControl::AttachedToWindow();
	SetViewColor(B_TRANSPARENT_32_BIT);
}


void
SeekSlider::SetValue(int32 value)
{
	if (value == Value())
		return;

#if __HAIKU__
	BControl::SetValueNoUpdate(value);
#else
	BControl::SetValue(value);
		// this will Invalidate() the whole view
#endif
	Invoke();

	_SetKnobPosition(_KnobPosFor(Bounds(), Value()));

	fLastTrackTime = system_time();
}


void
SeekSlider::Draw(BRect updateRect)
{
	BRect r(Bounds());	

	// draw both sides (the original from Be doesn't seem
	// to make a difference for enabled/disabled state)
//	DrawBitmapAsync(fLeftSideBits, r.LeftTop());
//	DrawBitmapAsync(fRightSideBits, BPoint(sliderEnd + 1.0, r.top));
	// colors for the slider area between the two bitmaps
	rgb_color background = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color shadow = tint_color(background, B_DARKEN_2_TINT);
	rgb_color softShadow = tint_color(background, B_DARKEN_1_TINT);
	rgb_color darkShadow = tint_color(background, B_DARKEN_4_TINT);
	rgb_color midShadow = tint_color(background, B_DARKEN_3_TINT);
	rgb_color light = tint_color(background, B_LIGHTEN_MAX_TINT);
	rgb_color softLight = tint_color(background, B_LIGHTEN_1_TINT);
	rgb_color green = kSeekGreen;
	rgb_color greenShadow = kSeekGreenShadow;
	rgb_color black = kBlack;
	rgb_color dotGrey = midShadow;
	rgb_color dotGreen = greenShadow;
	// draw frame
	_StrokeFrame(r, softShadow, softShadow, light, light);
	r.InsetBy(1.0, 1.0);
	_StrokeFrame(r, black, black, softShadow, softShadow);
	if (IsEnabled()) {
		// *** enabled look ***
		r.InsetBy(1.0, 1.0);
		// inner shadow
		_StrokeFrame(r, greenShadow, greenShadow, green, green);
		r.top++;
		r.left++;
		_StrokeFrame(r, greenShadow, greenShadow, green, green);
		// inside area
		r.InsetBy(1.0, 1.0);
		SetHighColor(green);
		FillRect(r);
		// dots
		int32 dotCount = (int32)(r.Width() / 6.0);
		BPoint dotPos;
		dotPos.y = r.top + 2.0;
		SetHighColor(dotGreen);

		float knobWidth2 = SEEK_SLIDER_KNOB_WIDTH / 2.0;
		float sliderStart = (r.left + knobWidth2);

		for (int32 i = 0; i < dotCount; i++) {
			dotPos.x = sliderStart + i * 6.0;
			StrokeLine(dotPos, BPoint(dotPos.x, dotPos.y + 6.0));
		}
		// slider handle
		r.top -= 4.0;
		r.bottom += 3.0;
		r.left = fKnobPos - knobWidth2;
		r.right = fKnobPos + knobWidth2;
		// black outline
		float handleBottomSize = 2.0;
		float handleArrowSize = 6.0;
		BeginLineArray(10);
			// upper handle
			AddLine(BPoint(r.left, r.top + handleBottomSize),
					BPoint(r.left, r.top), black);
			AddLine(BPoint(r.left + 1.0, r.top),
					BPoint(r.right, r.top), black);
			AddLine(BPoint(r.right, r.top + 1.0),
					BPoint(r.right, r.top + handleBottomSize), black);
			AddLine(BPoint(r.right - 1.0, r.top + handleBottomSize + 1.0),
					BPoint(fKnobPos, r.top + handleArrowSize), black);
			AddLine(BPoint(fKnobPos - 1.0, r.top + handleArrowSize - 1.0),
					BPoint(r.left + 1.0, r.top + handleBottomSize + 1.0), black);
			// lower handle
			AddLine(BPoint(r.left, r.bottom),
					BPoint(r.left, r.bottom - handleBottomSize), black);
			AddLine(BPoint(r.left + 1.0, r.bottom - handleBottomSize - 1.0),
					BPoint(fKnobPos, r.bottom - handleArrowSize), black);
			AddLine(BPoint(fKnobPos + 1.0, r.bottom - handleArrowSize + 1.0),
					BPoint(r.right, r.bottom - handleBottomSize), black);
			AddLine(BPoint(r.right, r.bottom - handleBottomSize + 1.0),
					BPoint(r.right, r.bottom), black);
			AddLine(BPoint(r.right - 1.0, r.bottom),
					BPoint(r.left + 1.0, r.bottom), black);
		EndLineArray();
		// inner red light and shadow lines
		r.InsetBy(1.0, 1.0);
		handleBottomSize--;
		handleArrowSize -= 2.0;
		BeginLineArray(10);
			// upper handle
			AddLine(BPoint(r.left, r.top + handleBottomSize),
					BPoint(r.left, r.top), kSeekRedLight);
			AddLine(BPoint(r.left + 1.0, r.top),
					BPoint(r.right, r.top), kSeekRedLight);
			AddLine(BPoint(r.right, r.top + 1.0),
					BPoint(r.right, r.top + handleBottomSize), kSeekRedShadow);
			AddLine(BPoint(r.right - 1.0, r.top + handleBottomSize + 1.0),
					BPoint(fKnobPos, r.top + handleArrowSize), kSeekRedShadow);
			AddLine(BPoint(fKnobPos - 1.0, r.top + handleArrowSize - 1.0),
					BPoint(r.left + 1.0, r.top + handleBottomSize + 1.0), kSeekRedLight);
			// lower handle
			AddLine(BPoint(r.left, r.bottom),
					BPoint(r.left, r.bottom - handleBottomSize), kSeekRedLight);
			AddLine(BPoint(r.left + 1.0, r.bottom - handleBottomSize - 1.0),
					BPoint(fKnobPos, r.bottom - handleArrowSize), kSeekRedLight);
			AddLine(BPoint(fKnobPos + 1.0, r.bottom - handleArrowSize + 1.0),
					BPoint(r.right, r.bottom - handleBottomSize), kSeekRedShadow);
			AddLine(BPoint(r.right, r.bottom - handleBottomSize + 1.0),
					BPoint(r.right, r.bottom), kSeekRedShadow);
			AddLine(BPoint(r.right - 1.0, r.bottom),
					BPoint(r.left + 1.0, r.bottom), kSeekRedShadow);
		EndLineArray();
		// fill rest of handles with red
		SetHighColor(kSeekRed);
		r.InsetBy(1.0, 1.0);
		handleArrowSize -= 2.0;
		BPoint arrow[3];
		// upper handle arrow
		arrow[0].x = r.left;
		arrow[0].y = r.top;
		arrow[1].x = r.right;
		arrow[1].y = r.top;
		arrow[2].x = fKnobPos;
		arrow[2].y = r.top + handleArrowSize;
		FillPolygon(arrow, 3);
		// lower handle arrow
		arrow[0].x = r.left;
		arrow[0].y = r.bottom;
		arrow[1].x = r.right;
		arrow[1].y = r.bottom;
		arrow[2].x = fKnobPos;
		arrow[2].y = r.bottom - handleArrowSize;
		FillPolygon(arrow, 3);
	} else {
		// *** disabled look ***
		r.InsetBy(1.0, 1.0);
		_StrokeFrame(r, darkShadow, darkShadow, darkShadow, darkShadow);
		r.InsetBy(1.0, 1.0);
		_StrokeFrame(r, darkShadow, darkShadow, darkShadow, darkShadow);
		r.InsetBy(1.0, 1.0);
		SetHighColor(darkShadow);
		SetLowColor(shadow);
		// stripes
		float width = floorf(StringWidth(kDisabledSeekMessage));
		float textPos = r.left + r.Width() / 2.0 - width / 2.0;
		pattern stripes = { { 0xc7, 0x8f, 0x1f, 0x3e, 0x7c, 0xf8, 0xf1, 0xe3 } };
		BRect stripesRect(r);
		stripesRect.right = textPos - 5.0;
		FillRect(stripesRect, stripes);
		stripesRect.left = textPos + width + 3.0;
		stripesRect.right = r.right;
		FillRect(stripesRect, stripes);
		// info text
		r.left = textPos - 4.0;
		r.right = textPos + width + 2.0;
		FillRect(r);
		SetHighColor(shadow);
		SetLowColor(darkShadow);
		font_height fh;
		GetFontHeight(&fh);
		DrawString(kDisabledSeekMessage, BPoint(textPos, r.top + ceilf(fh.ascent) - 1.0));
	}
}


void
SeekSlider::MouseDown(BPoint where)
{
	if (IsEnabled() && Bounds().Contains(where)) {
		SetValue(_ValueFor(where.x));
		fTracking = true;
		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
	}
}


void
SeekSlider::MouseMoved(BPoint where, uint32 code, const BMessage* dragMessage)
{
	if (fTracking) {
		SetValue(_ValueFor(where.x));
	}
}


void
SeekSlider::MouseUp(BPoint where)
{
	if (fTracking) {
		fTracking = false;
	}
}



void
SeekSlider::ResizeToPreferred()
{
	float width = 15.0 + StringWidth(kDisabledSeekMessage) + 15.0;
	ResizeTo(width, 17.0);
}


void
SeekSlider::FrameResized(float width, float height)
{
	_SetKnobPosition(_KnobPosFor(Bounds(), Value()));
}


void
SeekSlider::SetPosition(float position)
{
	int32 value = fMinValue + (int32)floorf((fMaxValue - fMinValue) * position + 0.5);
	if (value != Value()) {
		BControl::SetValue(value);
		_SetKnobPosition(_KnobPosFor(Bounds(), Value()));
	}
}


bool
SeekSlider::IsTracking() const
{
	if (fTracking)
		return true;
	return system_time() - fLastTrackTime < 250000;
}


// #pragma mark -


int32
SeekSlider::_ValueFor(float xPos) const
{
	BRect r(Bounds());
	float knobWidth2 = SEEK_SLIDER_KNOB_WIDTH / 2.0;
	float sliderStart = (r.left + knobWidth2);
	float sliderEnd = (r.right - knobWidth2);
	int32 value =  fMinValue + (int32)(((xPos - sliderStart) * (fMaxValue - fMinValue))
				  / (sliderEnd - sliderStart - 1.0));
	if (value < fMinValue)
		value = fMinValue;
	if (value > fMaxValue)
		value = fMaxValue;
	return value;
}


int32
SeekSlider::_KnobPosFor(BRect r, int32 value) const
{
	float knobWidth2 = SEEK_SLIDER_KNOB_WIDTH / 2.0;
	r.left += knobWidth2;
	r.right -= knobWidth2;
	float knobPos = r.left
					+ floorf((r.right - r.left - 1.0) * (Value() - fMinValue)
					/ (fMaxValue - fMinValue) + 0.5);
	return (int32)knobPos;
}


void
SeekSlider::_StrokeFrame(BRect r, rgb_color left, rgb_color top,
						 rgb_color right, rgb_color bottom)
{
	BeginLineArray(4);
		AddLine(BPoint(r.left, r.bottom), BPoint(r.left, r.top), left);
		AddLine(BPoint(r.left + 1.0, r.top), BPoint(r.right, r.top), top);
		AddLine(BPoint(r.right, r.top + 1.0), BPoint(r.right, r.bottom), right);
		AddLine(BPoint(r.right - 1.0, r.bottom), BPoint(r.left + 1.0, r.bottom), bottom);
	EndLineArray();
}


void
SeekSlider::_SetKnobPosition(int32 knobPos)
{
	if (fKnobPos == knobPos)
		return;

	float knobWidth2 = SEEK_SLIDER_KNOB_WIDTH / 2.0;
	BRect oldKnob(Bounds());
	BRect newKnob(oldKnob);
	oldKnob.left = fKnobPos - knobWidth2;
	oldKnob.right = fKnobPos + knobWidth2;

	fKnobPos = knobPos;

	newKnob.left = fKnobPos - knobWidth2;
	newKnob.right = fKnobPos + knobWidth2;
	Invalidate(oldKnob | newKnob);
}

