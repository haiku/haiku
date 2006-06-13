/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */

#include "PopupSlider.h"

#include <math.h>
#include <stdio.h>

#include <Message.h>

#include <MDividable.h>
#include <MWindow.h>

#include "SliderView.h"

// constructor
PopupSlider::PopupSlider(const char* name, const char* label,
						 BMessage* model, BHandler* target,
						 int32 min, int32 max, int32 value,
						 const char* formatString)
	: PopupControl(name, fSlider = new SliderView(this, min, max, value,
												  formatString)),
	  MDividable(),
	  fModel(model),
	  fPressModel(NULL),
	  fReleaseModel(NULL),
	  fTarget(target),
	  fLabel(label),
	  fSliderButtonRect(0.0, 0.0, -1.0, -1.0),
	  fEnabled(true),
	  fTracking(false)
{
	SetViewColor(B_TRANSPARENT_32_BIT);
}

// destructor
PopupSlider::~PopupSlider()
{
	delete fModel;
	if (BWindow* window = fSlider->Window()) {
		window->Lock();
		window->RemoveChild(fSlider);
		window->Unlock();
	}
	delete fSlider;
}

// layoutprefs
minimax
PopupSlider::layoutprefs()
{
	BFont font;
	GetFont(&font);
	font_height fh;
	font.GetHeight(&fh);
	float labelHeight = 2.0 + ceilf(fh.ascent + fh.descent) + 2.0;
	float sliderWidth, sliderHeight;
	SliderView::GetSliderButtonDimensions(Max(), FormatString(), &font,
										  sliderWidth, sliderHeight);

	float height = labelHeight > sliderHeight + 2.0 ?
						labelHeight : sliderHeight + 2.0;

	float minLabelWidth = LabelWidth();
	if (rolemodel)
		labelwidth = rolemodel->LabelWidth();
	labelwidth = minLabelWidth > labelwidth ? minLabelWidth : labelwidth;

	fSliderButtonRect.left = labelwidth;
	fSliderButtonRect.right = fSliderButtonRect.left + sliderWidth + 2.0;
	fSliderButtonRect.top = floorf(height / 2.0 - (sliderHeight + 2.0) / 2.0);
	fSliderButtonRect.bottom = fSliderButtonRect.top + sliderHeight + 2.0;

	fSliderButtonRect.OffsetTo(Bounds().right - fSliderButtonRect.Width(),
							   fSliderButtonRect.top);

	mpm.mini.x = labelwidth + fSliderButtonRect.Width() + 1.0;
	mpm.maxi.x = 10000.0;
	mpm.mini.y = mpm.maxi.y = height + 1.0;

	mpm.weight = 1.0;

	return mpm;
}

// layout
BRect
PopupSlider::layout(BRect frame)
{
	MoveTo(frame.LeftTop());
	ResizeTo(frame.Width(), frame.Height());

	fSliderButtonRect.OffsetTo(Bounds().right - fSliderButtonRect.Width(),
							   fSliderButtonRect.top);
	return Frame();
}

// MessageReceived
void
PopupSlider::MessageReceived(BMessage* message)
{
	switch (message->what) {
		default:
			PopupControl::MessageReceived(message);
			break;
	}
}

// AttachedToWindow
void
PopupSlider::AttachedToWindow()
{
	fSliderButtonRect.OffsetTo(Bounds().right - fSliderButtonRect.Width(),
							   fSliderButtonRect.top);
	PopupControl::AttachedToWindow();
}

// Draw
void
PopupSlider::Draw(BRect updateRect)
{
	bool enabled = IsEnabled();
	rgb_color background = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color black;
	if (enabled) {
		black = tint_color(background, B_DARKEN_MAX_TINT);
	} else {
		black = tint_color(background, B_DISABLED_LABEL_TINT);
	}
	// draw label
	BRect r(Bounds());
	r.right = fSliderButtonRect.left - 1.0;
	font_height fh;
	GetFontHeight(&fh);
	BPoint textPoint(0.0, (r.top + r.bottom) / 2.0 + fh.ascent / 2.0);
	SetLowColor(background);
	SetHighColor(black);
	FillRect(r, B_SOLID_LOW);
	DrawString(fLabel.String(), textPoint);
	// draw slider button
	DrawSlider(fSliderButtonRect, enabled);
}

// MouseDown
void
PopupSlider::MouseDown(BPoint where)
{
	if (fEnabled && fSliderButtonRect.Contains(where) &&
		!fSlider->LockLooper()) {
		
		SetPopupLocation(BPoint(fSliderButtonRect.left + 1.0
								- fSlider->ButtonOffset(),
								-5.0));
		where.x -= fSliderButtonRect.left + 1.0;
		fSlider->SetDragOffset(where.x);
		// just to be on the safe side (avoid a dead lock)
		fTracking = true;
		ShowPopup(&where);
//		fSlider->SetDragOffset(where.x);
	}
}

// PopupShown
void
PopupSlider::PopupShown()
{
	TriggerValueChanged(fPressModel);
	fTracking = true;
}

// PopupHidden
void
PopupSlider::PopupHidden(bool canceled)
{
	TriggerValueChanged(fReleaseModel);
	fTracking = false;
}

// SetValue
void		
PopupSlider::SetValue(int32 value)
{
	if (!fTracking) {
/*		if (fSlider->LockLooper()) {
			fSlider->SetValue(value);
			fSlider->UnlockLooper();
		} else*/
		if (value != Value()) {
			fSlider->SetValue(value);
			if (LockLooperWithTimeout(0) >= B_OK) {
				Invalidate();
				UnlockLooper();
			}
		}
	} else
		ValueChanged(value);
}

// Value
int32	
PopupSlider::Value() const
{
	int32 value = 0;
/*	if (fSlider->LockLooper()) {
		value = fSlider->Value();
		fSlider->UnlockLooper();
	} else*/
		value = fSlider->Value();
	return value;
}

// SetEnabled
void
PopupSlider::SetEnabled(bool enable)
{
	if (enable != fEnabled) {
		fEnabled = enable;
		if (LockLooper()) {
			Invalidate();
			UnlockLooper();
		}
	}
}

// SetEnabled
bool
PopupSlider::IsEnabled() const
{
	return fEnabled;
}

// TriggerValueChanged
void
PopupSlider::TriggerValueChanged(const BMessage* message) const
{
	if (message && fTarget) {
		BMessage msg(*message);
		msg.AddInt64("be:when", system_time());
		msg.AddInt32("be:value", Value());
		msg.AddPointer("be:source", (void*)this);
		if (BLooper* looper = fTarget->Looper())
			looper->PostMessage(&msg, fTarget);
	}
}

// IsTracking
bool
PopupSlider::IsTracking() const
{
	return fTracking;
}

// ValueChanged
void
PopupSlider::ValueChanged(int32 newValue)
{
	TriggerValueChanged(fModel);
}

// DrawSlider
void
PopupSlider::DrawSlider(BRect frame, bool enabled)
{
	rgb_color background = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color lightShadow;
	rgb_color darkShadow;
	if (enabled) {
		lightShadow = tint_color(background, B_DARKEN_2_TINT);
		darkShadow = tint_color(background, B_DARKEN_4_TINT);
	} else {
		lightShadow = tint_color(background, B_DARKEN_1_TINT);
		darkShadow = tint_color(background, B_DARKEN_2_TINT);
	}

	BeginLineArray(4);
		AddLine(BPoint(frame.left, frame.bottom),
				BPoint(frame.left, frame.top), lightShadow);
		AddLine(BPoint(frame.left + 1.0, frame.top),
				BPoint(frame.right, frame.top), lightShadow);
		AddLine(BPoint(frame.right, frame.top + 1.0),
				BPoint(frame.right, frame.bottom), darkShadow);
		AddLine(BPoint(frame.right - 1.0, frame.bottom),
				BPoint(frame.left + 1.0, frame.bottom), darkShadow);
	EndLineArray();

	frame.InsetBy(1.0, 1.0);
	SliderView::DrawSliderButton(this, frame, Value(), FormatString(), enabled);
}

// Scale
float
PopupSlider::Scale(float ratio) const
{
	return ratio;
}

// DeScale
float
PopupSlider::DeScale(float ratio) const
{
	return ratio;
}

// SetMessage
void
PopupSlider::SetMessage(BMessage* message)
{
	delete fModel;
	fModel = message;
}

// SetPressedMessage
void
PopupSlider::SetPressedMessage(BMessage* message)
{
	delete fPressModel;
	fPressModel = message;
}

// SetReleasedMessage
void
PopupSlider::SetReleasedMessage(BMessage* message)
{
	delete fReleaseModel;
	fReleaseModel = message;
}

// SetMin
void		
PopupSlider::SetMin(int32 min)
{
/*	if (fSlider->LockLooper()) {
		fSlider->SetMin(min);
		fSlider->UnlockLooper();
	} else*/
		fSlider->SetMin(min);
}

// Min
int32		
PopupSlider::Min() const
{
	int32 value = 0;
/*	if (fSlider->LockLooper()) {
		value = fSlider->Min();
		fSlider->UnlockLooper();
	} else*/
		value = fSlider->Min();
	return value;
}

// SetMax
void
PopupSlider::SetMax(int32 max)
{
/*	if (fSlider->LockLooper()) {
		fSlider->SetMax(max);
		fSlider->UnlockLooper();
	} else*/
		fSlider->SetMax(max);
}

// Max
int32
PopupSlider::Max() const
{
	int32 value = 0;
/*	if (fSlider->LockLooper()) {
		value = fSlider->Max();
		fSlider->UnlockLooper();
	} else*/
		value = fSlider->Max();
	return value;
}

// SetLabel
void	
PopupSlider::SetLabel(const char* label)
{
	fLabel.SetTo(label);
	Invalidate();
}

// Label
const char*	
PopupSlider::Label() const
{
	return fLabel.String();
}

// LabelWidth
float
PopupSlider::LabelWidth()
{
	return _MinLabelWidth();
}

// StringForValue
const char*
PopupSlider::StringForValue(int32 value)
{
	return NULL;
}

// MaxValueStringWidth
float
PopupSlider::MaxValueStringWidth()
{
	return 0.0;
}

// SetFormatString
void	
PopupSlider::SetFormatString(const char* formatString)
{
/*	if (fSlider->LockLooper()) {
		fSlider->SetFormatString(formatString);
		fSlider->UnlockLooper();
	} else*/
		fSlider->SetFormatString(formatString);
}

// FormatString
const char*	
PopupSlider::FormatString() const
{
	return fSlider->FormatString();
}

// _MinLabelWidth
float
PopupSlider::_MinLabelWidth() const
{
	return ceilf(StringWidth(fLabel.String())) + 5.0;
}

/*
// StringForValue
const char*
PercentSlider::StringForValue(int32 value)
{
	BString string;
	string << (value * 100) / Max() << "%";
	return 
}
*/


