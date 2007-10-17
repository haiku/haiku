/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

// NOTE: Based on my code in the BeOS interface for the VLC media player
// that I did during the VLC 0.4.3 - 0.4.6 times. Code not done by me
// removed. -Stephan Aßmus

#include "VolumeSlider.h"

#include "ButtonBitmaps.h"
#include "DrawingTidbits.h"

#include <Bitmap.h>
#include <Screen.h>

#include <stdio.h>
#include <string.h>


// slider colors are hardcoded here, because that's just
// what they currently are within those bitmaps
const rgb_color kGreen = (rgb_color){ 152, 203, 152, 255 };
const rgb_color kGreenShadow = (rgb_color){ 102, 152, 102, 255 };
const rgb_color kBackground = (rgb_color){ 216, 216, 216, 255 };
const rgb_color kSeekGreen = (rgb_color){ 171, 221, 161, 255 };
const rgb_color kSeekGreenShadow = (rgb_color){ 144, 186, 136, 255 };
const rgb_color kSeekRed = (rgb_color){ 255, 0, 0, 255 };
const rgb_color kSeekRedLight = (rgb_color){ 255, 152, 152, 255 };
const rgb_color kSeekRedShadow = (rgb_color){ 178, 0, 0, 255 };

#define DIM_LEVEL 0.4

// constructor
VolumeSlider::VolumeSlider(BRect frame, const char* name,
						   int32 minValue, int32 maxValue,
						   BMessage* message, BHandler* target)
	: BControl(frame, name, NULL, message, B_FOLLOW_LEFT | B_FOLLOW_TOP,
			   B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
	  fLeftSideBits(NULL),
	  fRightSideBits(NULL),
	  fKnobBits(NULL),
	  fTracking(false),
	  fMuted(false),
	  fMinValue(minValue),
	  fMaxValue(maxValue)
{
	SetTarget(target);

	// create bitmaps
	BRect r(0.0, 0.0,
			kVolumeSliderBitmapWidth - 1, kVolumeSliderBitmapHeight - 1);
	fLeftSideBits = new BBitmap(r, B_CMAP8);
	fRightSideBits = new BBitmap(r, B_CMAP8);
	r.Set(0.0, 0.0,
		  kVolumeSliderKnobWidth - 1, kVolumeSliderKnobHeight - 1);
	fKnobBits = new BBitmap(r, B_CMAP8);

	_MakeBitmaps();
}

// destructor
VolumeSlider::~VolumeSlider()
{
	delete fLeftSideBits;
	delete fRightSideBits;
	delete fKnobBits;
}

// AttachedToWindow
void
VolumeSlider::AttachedToWindow()
{
	BControl::AttachedToWindow();
	SetViewColor(B_TRANSPARENT_COLOR);
}

// SetValue
void
VolumeSlider::SetValue(int32 value)
{
	if (value == Value())
		return;

	BControl::SetValue(value);
	Invoke();
}

// SetValueNoInvoke
void
VolumeSlider::SetValueNoInvoke(int32 value)
{
	if (value == Value())
		return;

	BControl::SetValue(value);
}

// SetEnabled
void
VolumeSlider::SetEnabled(bool enable)
{
	if (enable == IsEnabled())
		return;

	BControl::SetEnabled(enable);

	_MakeBitmaps();
	Invalidate();
}

// Draw
void
VolumeSlider::Draw(BRect updateRect)
{
	if (!IsValid()) {
		fprintf(stderr, "VolumeSlider::Draw() - Error: no valid bitmaps!");
		SetHighColor(255, 0, 0);
		FillRect(updateRect);
		return;
	}

	BRect r(Bounds());
	float sliderSideWidth = kVolumeSliderBitmapWidth;
	float sliderStart = (r.left + sliderSideWidth);
	float sliderEnd = (r.right - sliderSideWidth);
	float knobPos = sliderStart
					+ (sliderEnd - sliderStart - 1.0) * (Value() - fMinValue)
					/ (fMaxValue - fMinValue);
	// draw both sides (the original from Be doesn't seem
	// to make a difference for enabled/disabled state)
	DrawBitmapAsync(fLeftSideBits, r.LeftTop());
	DrawBitmapAsync(fRightSideBits, BPoint(sliderEnd + 1.0, r.top));
	// colors for the slider area between the two bitmaps
	rgb_color background = kBackground;//ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color shadow = tint_color(background, B_DARKEN_2_TINT);
	rgb_color softShadow = tint_color(background, B_DARKEN_1_TINT);
	rgb_color darkShadow = tint_color(background, B_DARKEN_4_TINT);
	rgb_color midShadow = tint_color(background, B_DARKEN_3_TINT);
	rgb_color light = tint_color(background, B_LIGHTEN_MAX_TINT);
	rgb_color softLight = tint_color(background, B_LIGHTEN_1_TINT);
	rgb_color green = kGreen;
	rgb_color greenShadow = kGreenShadow;
	rgb_color black = kBlack;
	rgb_color dotGrey = midShadow;
	rgb_color dotGreen = greenShadow;
	// make dimmed version of colors if we're disabled
	if (!IsEnabled()) {
		shadow = (rgb_color){ 200, 200, 200, 255 };
		softShadow = dimmed_color_cmap8(softShadow, background, DIM_LEVEL);
		darkShadow = dimmed_color_cmap8(darkShadow, background, DIM_LEVEL);
		midShadow = shadow;
		light = dimmed_color_cmap8(light, background, DIM_LEVEL);
		softLight = dimmed_color_cmap8(softLight, background, DIM_LEVEL);
		green = dimmed_color_cmap8(green, background, DIM_LEVEL);
		greenShadow = dimmed_color_cmap8(greenShadow, background, DIM_LEVEL);
		black = dimmed_color_cmap8(black, background, DIM_LEVEL);
		dotGreen = dotGrey;
	} else if (fMuted) {
		green = tint_color(kBackground, B_DARKEN_3_TINT);
		greenShadow = tint_color(kBackground, B_DARKEN_4_TINT);
		dotGreen = greenShadow;
	}
	// draw slider edges between bitmaps
	BeginLineArray(7);
		AddLine(BPoint(sliderStart, r.top),
				BPoint(sliderEnd, r.top), softShadow);
		AddLine(BPoint(sliderStart, r.bottom),
				BPoint(sliderEnd, r.bottom), softLight);
		r.InsetBy(0.0, 1.0);
		AddLine(BPoint(sliderStart, r.top),
				BPoint(sliderEnd, r.top), black);
		AddLine(BPoint(sliderStart, r.bottom),
				BPoint(sliderEnd, r.bottom), light);
		r.top++;
		AddLine(BPoint(sliderStart, r.top),
				BPoint(knobPos, r.top), greenShadow);
		AddLine(BPoint(knobPos, r.top),
				BPoint(sliderEnd, r.top), midShadow);
		r.top++;
		AddLine(BPoint(sliderStart, r.top),
				BPoint(knobPos, r.top), greenShadow);
	EndLineArray();
	// fill rest inside of slider
	r.InsetBy(0.0, 1.0);
	r.left = sliderStart;
	r.right = knobPos;
	SetHighColor(green);
	FillRect(r, B_SOLID_HIGH);
	r.left = knobPos + 1.0;
	r.right = sliderEnd;
	r.top -= 1.0;
	SetHighColor(shadow);
	FillRect(r, B_SOLID_HIGH);
	// draw little dots inside
	int32 dotCount = (int32)((sliderEnd - sliderStart) / 5.0);
	BPoint dotPos;
	dotPos.y = r.top + 4.0;
	for (int32 i = 0; i < dotCount; i++) {
		dotPos.x = sliderStart + i * 5.0 + 4.0;
		SetHighColor(dotPos.x < knobPos ? dotGreen : dotGrey);
		StrokeLine(dotPos, BPoint(dotPos.x, dotPos.y + 1.0));
	}
	// draw knob
	r.top -= 1.0;
	SetDrawingMode(B_OP_OVER); // part of knob is transparent
	DrawBitmapAsync(fKnobBits, BPoint(knobPos - kVolumeSliderKnobWidth / 2, r.top));
}

// MouseDown
void
VolumeSlider::MouseDown(BPoint where)
{
	if (Bounds().Contains(where) && IsEnabled()) {
		fTracking = true;
		SetValue(_ValueFor(where.x));
		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
	}
}

// MouseMoved
void
VolumeSlider::MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage)
{
	if (fTracking)
		SetValue(_ValueFor(where.x));
}

// MouseUp
void
VolumeSlider::MouseUp(BPoint where)
{
	fTracking = false;
}

// IsValid
bool
VolumeSlider::IsValid() const
{
	return (fLeftSideBits && fLeftSideBits->IsValid()
			&& fRightSideBits && fRightSideBits->IsValid()
			&& fKnobBits && fKnobBits->IsValid());
}

// SetMuted
void
VolumeSlider::SetMuted(bool mute)
{
	if (mute == fMuted)
		return;

	fMuted = mute;
	_MakeBitmaps();
	Invalidate();
}

// _MakeBitmaps
void
VolumeSlider::_MakeBitmaps()
{
	if (!IsValid())
		return;

	// left side of slider
	memcpy(fLeftSideBits->Bits(), kVolumeSliderLeftBitmapBits,
		   fLeftSideBits->BitsLength());
	// right side of slider
	memcpy(fRightSideBits->Bits(), kVolumeSliderRightBits,
		   fRightSideBits->BitsLength());
	// slider knob
	int32 length = fKnobBits->BitsLength();
	memcpy(fKnobBits->Bits(), kVolumeSliderKnobBits, length);
	uint8* bits = (uint8*)fKnobBits->Bits();
	// black was used in the knob to represent transparency
	// use screen to get index for the "transarent" color used in the bitmap
	BScreen screen(B_MAIN_SCREEN_ID);
	uint8 blackIndex = screen.IndexForColor(kBlack);
	// replace black index with transparent index
	for (int32 i = 0; i < length; i++)
		if (bits[i] == blackIndex)
			bits[i] = B_TRANSPARENT_MAGIC_CMAP8;

	if (!IsEnabled()) {
		// make ghosted versions of the bitmaps
		dim_bitmap(fLeftSideBits, kBackground, DIM_LEVEL);
		dim_bitmap(fRightSideBits, kBackground, DIM_LEVEL);
		dim_bitmap(fKnobBits, kBackground, DIM_LEVEL);
	} else if (fMuted) {
		// replace green color (and shadow) in left slider side
		bits = (uint8*)fLeftSideBits->Bits();
		length = fLeftSideBits->BitsLength();
		uint8 greenIndex = screen.IndexForColor(kGreen);
		uint8 greenShadowIndex = screen.IndexForColor(kGreenShadow);
		rgb_color shadow = tint_color(kBackground, B_DARKEN_3_TINT);
		rgb_color midShadow = tint_color(kBackground, B_DARKEN_4_TINT);
		uint8 replaceIndex = screen.IndexForColor(shadow);
		uint8 replaceShadowIndex = screen.IndexForColor(midShadow);
		for (int32 i = 0; i < length; i++) {
			if (bits[i] == greenIndex)
				bits[i] = replaceIndex;
			else if (bits[i] == greenShadowIndex)
				bits[i] = replaceShadowIndex;
		}
	}
}

// _ValueFor
int32
VolumeSlider::_ValueFor(float xPos) const
{
	BRect r(Bounds());
	float sliderStart = (r.left + kVolumeSliderBitmapWidth);
	float sliderEnd = (r.right - kVolumeSliderBitmapWidth);
	int32 value = fMinValue + (int32)(((xPos - sliderStart)
				  * (fMaxValue - fMinValue))
				  / (sliderEnd - sliderStart - 1.0));
	if (value < fMinValue)
		value = fMinValue;
	if (value > fMaxValue)
		value = fMaxValue;
	return value;
}

