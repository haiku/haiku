/*
 * Copyright 2010, Stephan Aßmus <superstippi@gmx.de>.
 * Distributed under the terms of the MIT License.
 */


#include "VolumeSlider.h"

#include <GradientLinear.h>

#include <stdio.h>
#include <string.h>


#define KNOB_EMBEDDED 0
#define ROUND_KNOB 0

static const rgb_color kGreen = (rgb_color){ 116, 224, 0, 255 };


// constructor
VolumeSlider::VolumeSlider(const char* name, int32 minValue, int32 maxValue,
		int32 snapValue, BMessage* message)
	:
	BSlider(name, NULL, NULL, minValue, maxValue, B_HORIZONTAL,
		B_BLOCK_THUMB),
	fMuted(false),
	fSnapValue(snapValue),
	fSnapping(false)
{
	SetModificationMessage(message);
	UseFillColor(true, &kGreen);
	SetBarThickness(PreferredBarThickness());
}


VolumeSlider::~VolumeSlider()
{
}


void
VolumeSlider::MouseMoved(BPoint where, uint32 transit,
	const BMessage* dragMessage)
{
	if (!IsTracking()) {
		BSlider::MouseMoved(where, transit, dragMessage);
		return;
	}

	float cursorPosition = Orientation() == B_HORIZONTAL ? where.x : where.y;

	if (fSnapping
		&& cursorPosition >= fMinSnap && cursorPosition <= fMaxSnap) {
		// Don't move the slider, keep the current value for a few
		// more pixels
		return;
	}

	fSnapping = false;

	int32 oldValue = Value();
	int32 newValue = ValueForPoint(where);
	if (oldValue == newValue) {
		BSlider::MouseMoved(where, transit, dragMessage);
		return;
	}

	// Check if there is a 0 dB transition at all
	if ((oldValue < fSnapValue && newValue >= fSnapValue)
		|| (oldValue > fSnapValue && newValue <= fSnapValue)) {
		SetValue(fSnapValue);
		if (ModificationMessage() != NULL)
			Messenger().SendMessage(ModificationMessage());

		float snapPoint = _PointForValue(fSnapValue);
		const float kMaxSnapOffset = 6;
		if (oldValue > newValue) {
			// movement from right to left
			fMinSnap = snapPoint - kMaxSnapOffset;
			fMaxSnap = snapPoint + 1;
		} else {
			// movement from left to right
			fMinSnap = snapPoint - 1;
			fMaxSnap = snapPoint + kMaxSnapOffset;
		}

		fSnapping = true;
		return;
	}

	BSlider::MouseMoved(where, transit, dragMessage);
}


BRect
VolumeSlider::ThumbFrame() const
{
#if !ROUND_KNOB
	BRect rect = BSlider::ThumbFrame();
	rect.InsetBy(2, 2);
	rect.bottom += 1;
#else
	BRect rect(BarFrame());
#	if KNOB_EMBEDDED
	// Knob embedded in bar frame
	rect.InsetBy(0, 1);
#	else
	// Knob extends outside the bar frame
	rect.InsetBy(0, -1);
#	endif
	rect.InsetBy(rect.Height() / 2, 0);
	rect.left = rect.left + rect.Width() * Position() - rect.Height() / 2;
	rect.right = rect.left + rect.Height();
#endif

	return rect;
}


void
VolumeSlider::DrawThumb()
{
#if ROUND_KNOB
	// Draw a round thumb
	BRect rect(ThumbFrame());

	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color frameLightColor;
	rgb_color frameShadowColor;
	rgb_color shadowColor = (rgb_color){ 0, 0, 0, 60 };

	float topTint = 0.49;
	float middleTint1 = 0.62;
	float middleTint2 = 0.76;
	float bottomTint = 0.90;

	if (!IsEnabled()) {
		topTint = (topTint + B_NO_TINT) / 2;
		middleTint1 = (middleTint1 + B_NO_TINT) / 2;
		middleTint2 = (middleTint2 + B_NO_TINT) / 2;
		bottomTint = (bottomTint + B_NO_TINT) / 2;
		shadowColor = (rgb_color){ 0, 0, 0, 30 };
	}

	// Draw shadow
#if !KNOB_EMBEDDED
	rect.left++;
	rect.top++;
	SetDrawingMode(B_OP_ALPHA);
	SetHighColor(shadowColor);
	FillEllipse(rect);

	// Draw thumb shape
	rect.OffsetBy(-1, -1);
#endif

	if (IsFocus()) {
		// focused
		frameLightColor = ui_color(B_KEYBOARD_NAVIGATION_COLOR);
		frameShadowColor = frameLightColor;
	} else {
		// figure out the tints to be used
		float frameLightTint;
		float frameShadowTint;

		if (!IsEnabled()) {
			frameLightTint = 1.30;
			frameShadowTint = 1.35;
			shadowColor.alpha = 30;
		} else {
			frameLightTint = 1.6;
			frameShadowTint = 1.65;
		}

		frameLightColor = tint_color(base, frameLightTint);
		frameShadowColor = tint_color(base, frameShadowTint);
	}

	BGradientLinear frameGradient;
	frameGradient.AddColor(frameShadowColor, 0);
	frameGradient.AddColor(frameLightColor, 255);
	frameGradient.SetStart(rect.LeftTop());
	frameGradient.SetEnd(rect.RightBottom());

	FillEllipse(rect, frameGradient);
	rect.InsetBy(1, 1);

//	frameGradient.MakeEmpty();
//	frameGradient.AddColor(borderColor, 0);
//	frameGradient.AddColor(tint_color(borderColor, 0.8), 255);
//	view->FillEllipse(rect, frameGradient);
//	rect.InsetBy(1, 1);

	BGradientLinear gradient;
	if (!IsEnabled()) {
		gradient.AddColor(tint_color(base, topTint), 0);
		gradient.AddColor(tint_color(base, bottomTint), 255);
	} else {
		gradient.AddColor(tint_color(base, topTint), 0);
		gradient.AddColor(tint_color(base, middleTint1), 132);
		gradient.AddColor(tint_color(base, middleTint2), 136);
		gradient.AddColor(tint_color(base, bottomTint), 255);
	}
	gradient.SetStart(rect.LeftTop());
	gradient.SetEnd(rect.LeftBottom());
	FillEllipse(rect, gradient);
#else
	BSlider::DrawThumb();
#endif
}


BSize
VolumeSlider::MinSize()
{
	BSize size = BSlider::MinSize();
	size.width *= 2;
	return size;
}


void
VolumeSlider::SetMuted(bool mute)
{
	if (mute == fMuted)
		return;

	fMuted = mute;

	rgb_color fillColor = kGreen;
	if (fMuted) {
		fillColor = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
			B_DARKEN_2_TINT);
	}

	UseFillColor(true, &fillColor);

	Invalidate();
}


float
VolumeSlider::PreferredBarThickness() const
{
#if KNOB_EMBEDDED
	return 10.0f;
#else
	return 8.0f;
#endif
}


float
VolumeSlider::_PointForValue(int32 value) const
{
	int32 min, max;
	GetLimits(&min, &max);

	if (Orientation() == B_HORIZONTAL) {
		return ceilf(1.0f * (value - min) / (max - min)
			* (BarFrame().Width() - 2) + BarFrame().left + 1);
	}

	return ceilf(BarFrame().top - 1.0f * (value - min) / (max - min)
		* BarFrame().Height());
}


