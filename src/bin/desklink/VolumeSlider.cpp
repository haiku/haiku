/*
 * Copyright 2003-2009, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Jérôme Duval
 *		François Revol
 */


#include "VolumeSlider.h"

#include <string.h>
#include <stdio.h>

#include <Beep.h>
#include <Box.h>
#include <ControlLook.h>
#include <Debug.h>
#include <GroupLayout.h>
#include <Screen.h>
#include <Slider.h>

#include "MixerControl.h"

#define VOLUME_CHANGED 'vlcg'
#define VOLUME_UPDATED 'vlud'


class SliderView : public BSlider {
public:
							SliderView(const char* name, const char* label,
								BMessage *message, int32 minValue,
								int32 maxValue);
	virtual					~SliderView();

	virtual	void			MouseDown(BPoint where);
	virtual	void			MessageReceived(BMessage* msg);
	virtual void			DrawBar();
	virtual const char*		UpdateText() const;

private:
			mutable	char	fText[64];
};



SliderView::SliderView(const char* name, const char* label, BMessage *message,
		int32 minValue, int32 maxValue)
	: BSlider("slider", label, message, minValue, maxValue, B_HORIZONTAL)
{
	font_height fontHeight;
	GetFontHeight(&fontHeight);
	SetBarThickness(ceilf((fontHeight.ascent + fontHeight.descent) * 0.7));

	SetEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
}


SliderView::~SliderView()
{
}


void
SliderView::MouseDown(BPoint where)
{
	if (!IsEnabled() || !Bounds().Contains(where))
		Invoke();

	BSlider::MouseDown(where);
}


void
SliderView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case B_MOUSE_WHEEL_CHANGED:
		{
			if (Value() == -1)
				return;

			// Even though the volume bar is horizontal, we use the more common
			// vertical mouse wheel change
			float deltaY = 0.0f;

			msg->FindFloat("be:wheel_delta_y", &deltaY);

			if (deltaY == 0.0f)
				return;

			int32 currentValue = Value();
			// deltaY is generally 1 or -1, so this should increase or decrease
			// the 0-100 volume value by 5 each time. Also -5 is used because
			// mousewheel up is negative but should increase the volume.
			int32 newValue = MAX(MIN(currentValue + static_cast<int32>(deltaY * -5.0), 100), 0);

			if (newValue != currentValue) {
				SetValue(newValue);
				Draw(Bounds());
				Flush();

				if (Window())
					Window()->PostMessage(VOLUME_UPDATED);
			}

			break;
		}

		default:
			return BView::MessageReceived(msg);
	}
}


void
SliderView::DrawBar()
{
	BRect frame = BarFrame();
	BView* view = OffscreenView();

	if (be_control_look != NULL) {
		uint32 flags = be_control_look->Flags(this);
		rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
		rgb_color rightFillColor = (rgb_color){255, 109, 38, 255};
		rgb_color leftFillColor = (rgb_color){116, 224, 0, 255};

		int32 min, max;
		GetLimits(&min, &max);
		float position = 1.0f * (max - min - max) / (max - min);

		be_control_look->DrawSliderBar(view, frame, frame, base, leftFillColor,
			rightFillColor, position, flags, Orientation());
		return;
	}

	BSlider::DrawBar();
}


const char*
SliderView::UpdateText() const
{
	snprintf(fText, sizeof(fText), "%ld dB", Value());
	return fText;
}


//	#pragma mark -


VolumeSlider::VolumeSlider(BRect frame, bool dontBeep, int32 volumeWhich)
	: BWindow(frame, "VolumeSlider", B_BORDERED_WINDOW_LOOK,
		B_FLOATING_ALL_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_WILL_ACCEPT_FIRST_CLICK, 0)
{
	fDontBeep = dontBeep;

	const char *errorString = NULL;
	float value = 0.0;
	fMixerControl = new MixerControl(volumeWhich, &value, &errorString);

	SetLayout(new BGroupLayout(B_HORIZONTAL));

	BGroupLayout* layout = new BGroupLayout(B_HORIZONTAL);
	layout->SetInsets(10, 0, 10, 0);

	BBox* box = new BBox("sliderbox");
	box->SetLayout(layout);
	box->SetBorder(B_PLAIN_BORDER);
	AddChild(box);

	fHasChanged = false;
		// make sure we don't beep if we don't change anything

	fSlider = new SliderView("volume", errorString ? errorString : "Volume",
		new BMessage(VOLUME_CHANGED), fMixerControl->Minimum(),
		fMixerControl->Maximum());
	fSlider->SetValue((int32)value);
	fSlider->SetModificationMessage(new BMessage(VOLUME_UPDATED));
	if (errorString != NULL)
		fSlider->SetEnabled(false);
	box->AddChild(fSlider);

	fSlider->SetTarget(this);
	SetPulseRate(100);
	ResizeTo(300, 50);

	// Make sure it's not outside the screen.
	const int32 kMargin = 3;
	BRect windowRect = Frame();
	BRect screenFrame(BScreen(B_MAIN_SCREEN_ID).Frame());
	if (screenFrame.right < windowRect.right + kMargin)
		MoveBy(- kMargin - windowRect.right + screenFrame.right, 0);
	if (screenFrame.bottom < windowRect.bottom + kMargin)
		MoveBy(0, - kMargin - windowRect.bottom + screenFrame.bottom);
	if (screenFrame.left > windowRect.left - kMargin)
		MoveBy(kMargin + screenFrame.left - windowRect.left, 0);
	if (screenFrame.top > windowRect.top - kMargin)
		MoveBy(0, kMargin + screenFrame.top - windowRect.top);
}


VolumeSlider::~VolumeSlider()
{
	delete fMixerControl;
}


void
VolumeSlider::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case VOLUME_UPDATED:
puts("updated");
			PRINT(("VOLUME_UPDATED\n"));
			fMixerControl->SetVolume(fSlider->Value());
			fHasChanged = true;
			break;

		case VOLUME_CHANGED:
puts("changed");
			if (fHasChanged) {
				PRINT(("VOLUME_CHANGED\n"));
				fMixerControl->SetVolume(fSlider->Value());
				if (!fDontBeep)
					beep();
			}
			Quit();
			break;

		default:
			BWindow::MessageReceived(msg);
			break;
	}
}

