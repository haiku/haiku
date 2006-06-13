/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */

#include "SliderView.h"

#include <math.h>
#include <stdio.h>

#include <Message.h>
#include <Window.h>

#include "PopupSlider.h"

// constructor
SliderView::SliderView(PopupSlider* target,
					   int32 min, int32 max, int32 value,
					   const char* formatString)
	: PopupView("slider"),
	  fTarget(target),
	  fFormatString(formatString),
	  fMin(min),
	  fMax(max),
	  fValue(value),
	  fButtonRect(0.0, 0.0, -1.0, -1.0),
	  fDragOffset(0.0)
{
	SetViewColor(B_TRANSPARENT_32_BIT);
	if (Max() < Min())
		SetMax(Min());
	BFont font;
	GetFont(&font);
	float buttonWidth, buttonHeight;
	GetSliderButtonDimensions(Max(), FormatString(), &font,
							  buttonWidth, buttonHeight);

	fButtonRect.right = fButtonRect.left + buttonWidth;
	fButtonRect.bottom = fButtonRect.top + buttonHeight;
	float size = Max() - Min();
	if (size > 200)
		size = 200;
	ResizeTo(6.0 + fButtonRect.Width() + size + 6.0,
			 6.0 + fButtonRect.Height() + 6.0);
}

// destructor
SliderView::~SliderView()
{
}

// minimax
minimax	
SliderView::layoutprefs()
{
	mpm.mini.x = mpm.maxi.x = Bounds().Width() + 1.0;
	mpm.mini.y = mpm.maxi.y = Bounds().Height() + 1.0;

	mpm.weight = 1.0;

	return mpm;
}

// layout
BRect
SliderView::layout(BRect frame)
{
	MoveTo(frame.LeftTop());
	ResizeTo(frame.Width(), frame.Height());
	return Frame();
}

// Draw
void
SliderView::Draw(BRect updateRect)
{
	fButtonRect.OffsetTo(ButtonOffset(), 6.0);

	rgb_color background = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color light = tint_color(background, B_LIGHTEN_MAX_TINT);
	rgb_color lightShadow = tint_color(background, B_DARKEN_1_TINT);
	rgb_color shadow = tint_color(background, B_DARKEN_2_TINT);
	rgb_color darkShadow = tint_color(background, B_DARKEN_4_TINT);

	BRect r(Bounds());
	BeginLineArray(24);
		// outer dark line
		AddLine(BPoint(r.left, r.bottom),
				BPoint(r.left, r.top), lightShadow);
		AddLine(BPoint(r.left + 1.0, r.top),
				BPoint(r.right, r.top), lightShadow);
		AddLine(BPoint(r.right, r.top + 1.0),
				BPoint(r.right, r.bottom), darkShadow);
		AddLine(BPoint(r.right - 1.0, r.bottom),
				BPoint(r.left + 1.0, r.bottom), darkShadow);
		// second line (raised)
		r.InsetBy(1.0, 1.0);
		AddLine(BPoint(r.left, r.bottom),
				BPoint(r.left, r.top), light);
		AddLine(BPoint(r.left + 1.0, r.top),
				BPoint(r.right, r.top), light);
		AddLine(BPoint(r.right, r.top + 1.0),
				BPoint(r.right, r.bottom), shadow);
		AddLine(BPoint(r.right - 1.0, r.bottom),
				BPoint(r.left + 1.0, r.bottom), shadow);
		// third line (normal)
		r.InsetBy(1.0, 1.0);
		AddLine(BPoint(r.left, r.bottom),
				BPoint(r.left, r.top), background);
		AddLine(BPoint(r.left + 1.0, r.top),
				BPoint(r.right, r.top), background);
		AddLine(BPoint(r.right, r.top + 1.0),
				BPoint(r.right, r.bottom), background);
		AddLine(BPoint(r.right - 1.0, r.bottom),
				BPoint(r.left + 1.0, r.bottom), background);
		// fourth line (normal)
		r.InsetBy(1.0, 1.0);
		AddLine(BPoint(r.left, r.bottom),
				BPoint(r.left, r.top), background);
		AddLine(BPoint(r.left + 1.0, r.top),
				BPoint(r.right, r.top), background);
		AddLine(BPoint(r.right, r.top + 1.0),
				BPoint(r.right, r.bottom), background);
		AddLine(BPoint(r.right - 1.0, r.bottom),
				BPoint(r.left + 1.0, r.bottom), background);
		// fifth line (depressed)
		r.InsetBy(1.0, 1.0);
		AddLine(BPoint(r.left, r.bottom),
				BPoint(r.left, r.top), lightShadow);
		AddLine(BPoint(r.left + 1.0, r.top),
				BPoint(r.right, r.top), lightShadow);
		AddLine(BPoint(r.right, r.top + 1.0),
				BPoint(r.right, r.bottom), light);
		AddLine(BPoint(r.right - 1.0, r.bottom),
				BPoint(r.left + 1.0, r.bottom), light);
		// fifth line (strongly depressed)
		r.InsetBy(1.0, 1.0);
		AddLine(BPoint(r.left, r.bottom),
				BPoint(r.left, r.top), darkShadow);
		AddLine(BPoint(r.left + 1.0, r.top),
				BPoint(r.right, r.top), darkShadow);
		AddLine(BPoint(r.right, r.top + 1.0),
				BPoint(r.right, r.bottom), shadow);
		AddLine(BPoint(r.right - 1.0, r.bottom),
				BPoint(r.left + 1.0, r.bottom), shadow);
	EndLineArray();

	r.InsetBy(1.0, 1.0);
	SetLowColor(lightShadow);
	BRect leftOfButton(r.left + 1.0, r.top + 1.0, fButtonRect.left - 2.0, r.bottom);
	if (leftOfButton.IsValid())
		FillRect(leftOfButton, B_SOLID_LOW);
	BRect rightOfButton(fButtonRect.right + 2.0, r.top + 1.0,
						r.right, r.bottom);
	if (rightOfButton.IsValid())
		FillRect(rightOfButton, B_SOLID_LOW);

	// inner shadow and knob out lines
	BeginLineArray(5);
		// shadow
		AddLine(BPoint(r.left, r.bottom),
				BPoint(r.left, r.top), shadow);
		AddLine(BPoint(r.left + 1.0, r.top),
				BPoint(r.right, r.top), shadow);
		// at knob
		if (fButtonRect.left == 6.0)
			AddLine(BPoint(fButtonRect.left - 1.0, r.top),
					BPoint(fButtonRect.left - 1.0, r.bottom), darkShadow);
		else
			AddLine(BPoint(fButtonRect.left - 1.0, r.top),
					BPoint(fButtonRect.left - 1.0, r.bottom), shadow);
		AddLine(BPoint(fButtonRect.left, fButtonRect.bottom + 1.0),
				BPoint(fButtonRect.right + 1.0, fButtonRect.bottom + 1.0), darkShadow);
		AddLine(BPoint(fButtonRect.right + 1.0, fButtonRect.bottom),
				BPoint(fButtonRect.right + 1.0, fButtonRect.top), darkShadow);
	EndLineArray();


	DrawSliderButton(this, fButtonRect, fValue, fFormatString.String(), fTarget->IsEnabled());
}

// MouseUp
void
SliderView::MouseUp(BPoint where)
{
	PopupDone(false);
	fTarget->TriggerValueChanged(fTarget->Message());
}

// MouseMoved
void
SliderView::MouseMoved(BPoint where, uint32 transit, const BMessage* message)
{
	uint32 buttons = 0;
	if (BMessage* message = Window()->CurrentMessage()) {
		if (message->FindInt32("buttons", (int32*)&buttons) < B_OK)
			buttons = 0;
	}

	if (buttons == 0) {
		MouseUp(where);
		return;
	}
	
	SetValue(_ValueAt(where.x - fDragOffset - 6.0));
}

// MessageReceived
void
SliderView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		default:
			PopupView::MessageReceived(message);
			break;
	}
}

// SetValue
void
SliderView::SetValue(int32 value)
{
	if (value < fMin)
		value = fMin;
	if (value > fMax)
		value = fMax;
	if (value != fValue) {
		fValue = value;
		fTarget->SetValue(value);
		Invalidate();
	}
}

// Value
int32
SliderView::Value() const
{
	return fValue;
}

// SetMin
void	
SliderView::SetMin(int32 min)
{
	if (min != fMax) {
		fMin = min;
		if (fValue < fMin)
			SetValue(fMin);
		Invalidate();
	}
}

// Min
int32
SliderView::Min() const
{
	return fMin;
}

// SetMax
void	
SliderView::SetMax(int32 max)
{
	if (max != fMax) {
		fMax = max;
		if (fValue > fMax)
			SetValue(fMax);
		Invalidate();
	}
}

// Max
int32	
SliderView::Max() const
{
	return fMax;
}

// SetFormatString
void
SliderView::SetFormatString(const char* formatString)
{
	// TODO: check if formatString contains "%ld"
	fFormatString.SetTo(formatString);
}

// FormatString
const char*
SliderView::FormatString() const
{
	return fFormatString.String();
}

// SetDragOffset
void
SliderView::SetDragOffset(float offset)
{
	fDragOffset = offset;
}

// ButtonOffset
float
SliderView::ButtonOffset()
{
//	float range = Bounds().Width() - 12.0 - fButtonRect.Width() + 1.0;
//	return 6.0 + range / (float)(fMax - fMin + 1) * (float)(fValue - fMin);
	float ratio = fTarget->DeScale((float)(fValue - fMin) / (float)(fMax - fMin));
	return 6.0 + ratio * (Bounds().Width() - 12.0 - fButtonRect.Width());
}

// GetSliderButtonDimensions
void
SliderView::GetSliderButtonDimensions(int32 max, const char* formatString,
									  BFont* font,
									  float& width, float& height)
{
	if (font) {
		char label[256];
		sprintf(label, formatString, max);
		font_height fh;
		font->GetHeight(&fh);
		// 4 pixels room on each side,
		// 1 pixel room at top and bottom
		width = 5.0 + ceilf(font->StringWidth(label)) + 5.0;
		height = 2.0 + ceilf(fh.ascent + fh.descent) + 2.0;
	}
}

// DrawSliderButton
void
SliderView::DrawSliderButton(BView* v, BRect r, int32 value,
							 const char* formatString, bool enabled)
{
	rgb_color background = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color light;
	rgb_color shadow;
	rgb_color button;
	rgb_color black;
	if (enabled) {
		light = tint_color(background, B_LIGHTEN_MAX_TINT);
		shadow = tint_color(background, B_DARKEN_1_TINT);
		button = tint_color(background, B_LIGHTEN_1_TINT);
		black = tint_color(background, B_DARKEN_MAX_TINT);
	} else {
		light = tint_color(background, B_LIGHTEN_1_TINT);
		shadow = tint_color(background, 1.1);
		button = tint_color(background, 0.8);
		black = tint_color(background, B_DISABLED_LABEL_TINT);
	}
	// border
	v->BeginLineArray(4);
		v->AddLine(BPoint(r.left, r.bottom),
				   BPoint(r.left, r.top), light);
		v->AddLine(BPoint(r.left + 1.0, r.top),
				   BPoint(r.right, r.top), light);
		v->AddLine(BPoint(r.right, r.top + 1.0),
				   BPoint(r.right, r.bottom), shadow);
		v->AddLine(BPoint(r.right - 1.0, r.bottom),
				   BPoint(r.left + 1.0, r.bottom), shadow);
	v->EndLineArray();
	// background & label
	r.InsetBy(1.0, 1.0);
	char label[256];
	sprintf(label, formatString, value);
	float width = v->StringWidth(label);
	font_height fh;
	v->GetFontHeight(&fh);
	BPoint textPoint((r.left + r.right) / 2.0 - width / 2.0,
					 (r.top + r.bottom) / 2.0 + fh.ascent / 2.0);
	v->SetHighColor(black);
	v->SetLowColor(button);
	v->FillRect(r, B_SOLID_LOW);
	v->DrawString(label, textPoint);
}

// _ValueAt
int32
SliderView::_ValueAt(float h)
{
/*	return fMin + (int32)(((float)(fMax - fMin + 1) * h) /
		   (Bounds().Width() - fButtonRect.Width() - 12.0));*/
	float ratio = h / (Bounds().Width() - fButtonRect.Width() - 12.0);
	if (ratio < 0.0)
		ratio = 0.0;
	if (ratio > 1.0)
		ratio = 1.0;
	ratio = fTarget->Scale(ratio);
	return fMin + (int32)((fMax - fMin + 1) * ratio);
}




