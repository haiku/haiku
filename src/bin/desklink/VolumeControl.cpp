/*
 * Copyright 2003-2009, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Jérôme Duval
 *		François Revol
 *		Axel Dörfler, axeld@pinc-software.de.
 */


#include "VolumeControl.h"

#include <string.h>
#include <stdio.h>

#include <Application.h>
#include <Beep.h>
#include <ControlLook.h>
#include <Dragger.h>
#include <Roster.h>

#include <AppMisc.h>

#include "desklink.h"
#include "MixerControl.h"
#include "VolumeWindow.h"


VolumeControl::VolumeControl(int32 volumeWhich, bool beep, BMessage* message)
	: BSlider("VolumeControl", "Volume", message, 0, 1, B_HORIZONTAL),
	fBeep(beep)
{
	font_height fontHeight;
	GetFontHeight(&fontHeight);
	SetBarThickness(ceilf((fontHeight.ascent + fontHeight.descent) * 0.7));

	_InitVolume(volumeWhich);

	BRect rect(Bounds());
	rect.top = rect.bottom - 7;
	rect.left = rect.right - 7;
	BDragger* dragger = new BDragger(rect, this,
		B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	AddChild(dragger);
}


VolumeControl::VolumeControl(BMessage* archive)
	: BSlider(archive)
{
	if (archive->FindBool("beep", &fBeep) != B_OK)
		fBeep = false;

	int32 volumeWhich;
	if (archive->FindInt32("volume which", &volumeWhich) != B_OK)
		volumeWhich = VOLUME_USE_MIXER;

	_InitVolume(volumeWhich);
}


VolumeControl::~VolumeControl()
{
	delete fMixerControl;
}


status_t
VolumeControl::Archive(BMessage* into, bool deep) const
{
	status_t status;

	status = BView::Archive(into, deep);
	if (status < B_OK)
		return status;

	status = into->AddString("add_on", kAppSignature);
	if (status < B_OK)
		return status;

	status = into->AddBool("beep", fBeep);
	if (status != B_OK)
		return status;

	return into->AddInt32("volume which", fMixerControl->VolumeWhich());
}


VolumeControl*
VolumeControl::Instantiate(BMessage* archive)
{
	if (!validate_instantiation(archive, "VolumeControl"))
		return NULL;

	return new VolumeControl(archive);
}


void
VolumeControl::AttachedToWindow()
{
	BSlider::AttachedToWindow();

	SetEventMask(_IsReplicant() ? 0 : B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
}


void
VolumeControl::MouseDown(BPoint where)
{
	// Ignore clicks on the dragger
	int32 viewToken;
	if (Bounds().Contains(where) && Looper()->CurrentMessage() != NULL
		&& Looper()->CurrentMessage()->FindInt32("_view_token",
				&viewToken) == B_OK
		&& viewToken != _get_object_token_(this))
		return;

	// TODO: investigate why this does not work as expected (the dragger
	// frame seems to be off)
#if 0
	if (BView* dragger = ChildAt(0)) {
		if (!dragger->IsHidden() && dragger->Frame().Contains(where))
			return;
	}
#endif

	if (!IsEnabled() || !Bounds().Contains(where)) {
		Invoke();
		return;
	}

	BSlider::MouseDown(where);
}


void
VolumeControl::MessageReceived(BMessage* msg)
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
			int32 newValue = currentValue - int32(deltaY) * 3;

			if (newValue != currentValue) {
				SetValue(newValue);
				if (ModificationMessage() != NULL)
					Messenger().SendMessage(ModificationMessage());
			}
			break;
		}

		default:
			return BView::MessageReceived(msg);
	}
}


status_t
VolumeControl::Invoke(BMessage* message = NULL)
{
	if (fBeep && fOriginalValue != Value() && message == NULL) {
		beep();
		fOriginalValue = Value();
	}

	fMixerControl->SetVolume(Value());

	return BSlider::Invoke(message);
}


void
VolumeControl::DrawBar()
{
	BRect frame = BarFrame();
	BView* view = OffscreenView();

	if (be_control_look != NULL) {
		uint32 flags = be_control_look->Flags(this);
		rgb_color base = LowColor();
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
VolumeControl::UpdateText() const
{
	if (!IsEnabled())
		return NULL;

	snprintf(fText, sizeof(fText), "%ld dB", Value());
	return fText;
}


void
VolumeControl::_InitVolume(int32 volumeWhich)
{
	const char* errorString = NULL;
	float volume = 0.0;
	fMixerControl = new MixerControl(volumeWhich, &volume, &errorString);

	if (errorString != NULL) {
		SetLabel(errorString);
		SetLimits(-60, 18);
	} else {
		SetLabel("Volume");
		SetLimits((int32)floorf(fMixerControl->Minimum()),
			(int32)ceilf(fMixerControl->Maximum()));
	}

	SetEnabled(errorString == NULL);

	fOriginalValue = (int32)volume;
	SetValue((int32)volume);
}


bool
VolumeControl::_IsReplicant() const
{
	return dynamic_cast<VolumeWindow*>(Window()) == NULL;
}
