/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "InputSlider.h"

#include <stdio.h>

#include <Message.h>
#include <MessageFilter.h>

#include "NummericalTextView.h"


// MouseDownFilter

class NumericInputFilter : public BMessageFilter {
 public:
								NumericInputFilter(InputSlider* slider);

	virtual	filter_result		Filter(BMessage*, BHandler** target);

 private:
			InputSlider*		fSlider;
};

// constructor
NumericInputFilter::NumericInputFilter(InputSlider* slider)
	: BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE),
	  fSlider(slider)
{
}

// Filter
filter_result
NumericInputFilter::Filter(BMessage* msg, BHandler** target)
{
	filter_result result = B_DISPATCH_MESSAGE;
	switch (msg->what)
	{
		case B_KEY_DOWN:
		case B_KEY_UP:
		{
msg->PrintToStream();
			const char *string;
			if (msg->FindString("bytes", &string) == B_OK) {
				while (*string != 0) {
					if (*string < '0' || *string > '9') {
						
						if (*string != '-') {
							result = B_SKIP_MESSAGE;
						}
					}
					string++;
				}
			}
			break;
		}
		default:
			break;
	}
	/*
	if (fWindow) {
		if (BView* view = dynamic_cast<BView*>(*target)) {
			BPoint point;
			if (message->FindPoint("where", &point) == B_OK) {
				if (!fWindow->Frame().Contains(view->ConvertToScreen(point)))
					*target = fWindow;
			}
		}
	}*/
	return result;
}

// constructor
InputSlider::InputSlider(const char* name, const char* label,
						 BMessage* model, BHandler* target,
						 int32 min, int32 max, int32 value,
						 const char* formatString)
	: PopupSlider(name, label, model, target, min, max, value, formatString),
	  fTextView(new NummericalTextView(BRect(0, 0 , 20, 20),
	  								   "input",
	  								   BRect(5, 5, 15, 15),
	  								   B_FOLLOW_NONE,
	  								   B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE))
	  //fTextViewFilter(dynamic_cast<BMessageFilter*>(new NumericInputFilter(this)))
{
	// prepare fTextView
	fTextView->SetWordWrap(false);
	fTextView->SetViewColor(255,255,255,0);
	fTextView->SetValue(Value());
	//fTextView->AddFilter(fTextViewFilter);
	AddChild(fTextView);
}

// destructor
InputSlider::~InputSlider()
{
	//delete fTextViewFilter;
}

// layout
BRect
InputSlider::layout(BRect frame)
{
	PopupSlider::layout(frame);

	frame = SliderFrame();

	frame.right -= 10.0;
	frame.InsetBy(2, 2);

	fTextView->MoveTo(frame.LeftTop());
	fTextView->ResizeTo(frame.Width(), frame.Height());

	BRect textRect(fTextView->Bounds());
	textRect.InsetBy(1, 1);
	fTextView->SetTextRect(textRect);

	fTextView->SetAlignment(B_ALIGN_CENTER);

	return Frame();
}

// MouseDown
void
InputSlider::MouseDown(BPoint where)
{
	if (fTextView->Frame().Contains(where))
		return;

	fTextView->MakeFocus(true);

	if (SliderFrame().Contains(where)) {
		SetValue(fTextView->IntValue());
		PopupSlider::MouseDown(where);
	}
}

// SetEnabled
void
InputSlider::SetEnabled(bool enable)
{
	PopupSlider::SetEnabled(enable);

//	fTextView->SetEnabled(enable);
}

// ValueChanged
void
InputSlider::ValueChanged(int32 newValue)
{
	PopupSlider::ValueChanged(newValue);

	// change fTextView's value
	if (LockLooper()) {
		fTextView->SetValue(Value());
		UnlockLooper();
	}
}

// DrawSlider
void
InputSlider::DrawSlider(BRect frame, bool enabled)
{
	rgb_color background = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color lightShadow;
	rgb_color midShadow;
	rgb_color darkShadow;
	rgb_color light;
	if (enabled) {
		lightShadow = tint_color(background, B_DARKEN_1_TINT);
		midShadow = tint_color(background, B_DARKEN_2_TINT);
		darkShadow = tint_color(background, B_DARKEN_4_TINT);
		light = tint_color(background, B_LIGHTEN_MAX_TINT);
	} else {
		lightShadow = background;
		midShadow = tint_color(background, B_DARKEN_1_TINT);
		darkShadow = tint_color(background, B_DARKEN_2_TINT);
		light = tint_color(background, B_LIGHTEN_1_TINT);
	}

	// frame around text view
	BRect r(frame);
	BeginLineArray(16);
		AddLine(BPoint(r.left, r.bottom),
				BPoint(r.left, r.top), lightShadow);
		AddLine(BPoint(r.left + 1.0, r.top),
				BPoint(r.right, r.top), lightShadow);
		AddLine(BPoint(r.right, r.top + 1.0),
				BPoint(r.right, r.bottom), light);
		AddLine(BPoint(r.right - 1.0, r.bottom),
				BPoint(r.left + 1.0, r.bottom), light);

		r = fTextView->Frame().InsetByCopy(-1, -1);

		AddLine(BPoint(r.left, r.bottom),
				BPoint(r.left, r.top), darkShadow);
		AddLine(BPoint(r.left + 1.0, r.top),
				BPoint(r.right, r.top), darkShadow);
		AddLine(BPoint(r.right, r.top + 1.0),
				BPoint(r.right, r.bottom), background);
		AddLine(BPoint(r.right - 1.0, r.bottom),
				BPoint(r.left + 1.0, r.bottom), background);

		r.left = r.right + 1;
		r.right = frame.right - 1;

		AddLine(BPoint(r.left, r.bottom),
				BPoint(r.left, r.top + 1.0), midShadow);
		AddLine(BPoint(r.left, r.top),
				BPoint(r.right, r.top), darkShadow);
		AddLine(BPoint(r.right, r.top + 1.0),
				BPoint(r.right, r.bottom), midShadow);
		AddLine(BPoint(r.right - 1.0, r.bottom),
				BPoint(r.left + 1.0, r.bottom), midShadow);

		r.InsetBy(1, 1);

		AddLine(BPoint(r.left, r.bottom),
				BPoint(r.left, r.top), light);
		AddLine(BPoint(r.left + 1.0, r.top),
				BPoint(r.right, r.top), light);
		AddLine(BPoint(r.right, r.top + 1.0),
				BPoint(r.right, r.bottom), lightShadow);
		AddLine(BPoint(r.right - 1.0, r.bottom),
				BPoint(r.left + 1.0, r.bottom), lightShadow);
	EndLineArray();

	r.InsetBy(1, 1);
	SetLowColor(background);
	FillRect(r, B_SOLID_LOW);
}

