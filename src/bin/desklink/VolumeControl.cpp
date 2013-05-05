/*
 * Copyright 2003-2010, Haiku, Inc.
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
#include <Catalog.h>
#include <ControlLook.h>
#include <Dragger.h>
#include <MessageRunner.h>
#include <Roster.h>

#include <AppMisc.h>

#include "desklink.h"
#include "MixerControl.h"
#include "VolumeWindow.h"




#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "VolumeControl"


static const char* kMediaServerSignature = "application/x-vnd.Be.media-server";
static const char* kAddOnServerSignature = "application/x-vnd.Be.addon-host";

static const uint32 kMsgReconnectVolume = 'rcms';


VolumeControl::VolumeControl(int32 volumeWhich, bool beep, BMessage* message)
	:
	BSlider("VolumeControl", B_TRANSLATE("Volume"),
		message, 0, 1, B_HORIZONTAL),
	fMixerControl(new MixerControl(volumeWhich)),
	fBeep(beep),
	fSnapping(false),
	fConnectRetries(0)
{
	font_height fontHeight;
	GetFontHeight(&fontHeight);
	SetBarThickness(ceilf((fontHeight.ascent + fontHeight.descent) * 0.7));

	BRect rect(Bounds());
	rect.top = rect.bottom - 7;
	rect.left = rect.right - 7;
	BDragger* dragger = new BDragger(rect, this,
		B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	AddChild(dragger);
}


VolumeControl::VolumeControl(BMessage* archive)
	:
	BSlider(archive),
	fMixerControl(NULL),
	fSnapping(false),
	fConnectRetries(0)
{
	if (archive->FindBool("beep", &fBeep) != B_OK)
		fBeep = false;

	int32 volumeWhich;
	if (archive->FindInt32("volume which", &volumeWhich) != B_OK)
		volumeWhich = VOLUME_USE_MIXER;

	fMixerControl = new MixerControl(volumeWhich);

	archive->SendReply(new BMessage(B_QUIT_REQUESTED));
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

	if (_IsReplicant())
		SetEventMask(0, 0);
	else
		SetEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);

	be_roster->StartWatching(this, B_REQUEST_LAUNCHED | B_REQUEST_QUIT);

	_ConnectVolume();

	if (!fMixerControl->Connected()) {
		// Wait a bit, and try again - the media server might not have been
		// ready yet
		BMessage reconnect(kMsgReconnectVolume);
		BMessageRunner::StartSending(this, &reconnect, 1000000LL, 1);
		fConnectRetries = 3;
	}
}


void
VolumeControl::DetachedFromWindow()
{
	_DisconnectVolume();

	be_roster->StopWatching(this);
}


/*!	Since we have set a mouse event mask, we don't want to forward all
	mouse downs to the slider - instead, we only invoke it, which causes a
	message to our target. Within the VolumeWindow, this will actually
	cause the window to close.
	Also, we need to mask out the dragger in this case, or else dragging
	us will also cause a volume update.
*/
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
VolumeControl::MouseUp(BPoint where)
{
	fSnapping = false;
	BSlider::MouseUp(where);
}


/*!	Override the BSlider functionality to be able to grab the knob when
	it's over 0 dB for some pixels.
*/
void
VolumeControl::MouseMoved(BPoint where, uint32 transit,
	const BMessage* dragMessage)
{
	if (!IsTracking()) {
		BSlider::MouseMoved(where, transit, dragMessage);
		return;
	}

	float cursorPosition = Orientation() == B_HORIZONTAL ? where.x : where.y;

	if (fSnapping && cursorPosition >= fMinSnap && cursorPosition <= fMaxSnap) {
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
	if ((oldValue < 0 && newValue >= 0) || (oldValue > 0 && newValue <= 0)) {
		SetValue(0);
		if (ModificationMessage() != NULL)
			Messenger().SendMessage(ModificationMessage());

		float snapPoint = _PointForValue(0);
		const float kMinSnapOffset = 6;

		if (oldValue > newValue) {
			// movement from right to left
			fMinSnap = _PointForValue(-4);
			if (fabs(snapPoint - fMinSnap) < kMinSnapOffset)
				fMinSnap = snapPoint - kMinSnapOffset;

			fMaxSnap = _PointForValue(1);
		} else {
			// movement from left to right
			fMinSnap = _PointForValue(-1);
			fMaxSnap = _PointForValue(4);
			if (fabs(snapPoint - fMaxSnap) < kMinSnapOffset)
				fMaxSnap = snapPoint + kMinSnapOffset;
		}

		fSnapping = true;
		return;
	}

	BSlider::MouseMoved(where, transit, dragMessage);
}


void
VolumeControl::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case B_MOUSE_WHEEL_CHANGED:
		{
			if (!fMixerControl->Connected())
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
				InvokeNotify(ModificationMessage(), B_CONTROL_MODIFIED);
			}
			break;
		}

		case B_MEDIA_NEW_PARAMETER_VALUE:
			if (IsTracking())
				break;

			SetValue((int32)fMixerControl->Volume());
			break;

		case B_SOME_APP_LAUNCHED:
		case B_SOME_APP_QUIT:
		{
			const char* signature;
			if (msg->FindString("be:signature", &signature) != B_OK)
				break;

            bool isMediaServer = !strcmp(signature, kMediaServerSignature);
            bool isAddOnServer = !strcmp(signature, kAddOnServerSignature);
            if (isMediaServer)
                fMediaServerRunning = msg->what == B_SOME_APP_LAUNCHED;
            if (isAddOnServer)
                fAddOnServerRunning = msg->what == B_SOME_APP_LAUNCHED;

           if (isMediaServer || isAddOnServer) {
                if (!fMediaServerRunning && !fAddOnServerRunning) {
					// No media server around
					SetLabel(B_TRANSLATE("No media server running"));
					SetEnabled(false);
                } else if (fMediaServerRunning && fAddOnServerRunning) {
                    // HACK!
                    // quit our now invalid instance of the media roster
                    // so that before new nodes are created,
                    // we get a new roster
                    BMediaRoster* roster = BMediaRoster::CurrentRoster();
                    if (roster != NULL) {
                        roster->Lock();
                        roster->Quit();
                    }

					BMessage reconnect(kMsgReconnectVolume);
					BMessageRunner::StartSending(this, &reconnect, 1000000LL, 1);
					fConnectRetries = 3;
                }
			}
			break;
		}

		case B_QUIT_REQUESTED:
			Window()->MessageReceived(msg);
			break;

		case kMsgReconnectVolume:
			_ConnectVolume();
			if (!fMixerControl->Connected() && --fConnectRetries > 1) {
				BMessage reconnect(kMsgReconnectVolume);
				BMessageRunner::StartSending(this, &reconnect,
					6000000LL / fConnectRetries, 1);
			}
			break;

		default:
			return BView::MessageReceived(msg);
	}
}


status_t
VolumeControl::Invoke(BMessage* message)
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
		float position = (float)min / (min - max);

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

	fText.SetToFormat(B_TRANSLATE("%ld dB"), Value());
	return fText.String();
}


void
VolumeControl::_DisconnectVolume()
{
	BMediaRoster* roster = BMediaRoster::CurrentRoster();
	if (roster != NULL && fMixerControl->GainNode() != media_node::null) {
		roster->StopWatching(this, fMixerControl->GainNode(),
			B_MEDIA_NEW_PARAMETER_VALUE);
	}
}


void
VolumeControl::_ConnectVolume()
{
	_DisconnectVolume();

	const char* errorString = NULL;
	float volume = 0.0;
	fMixerControl->Connect(fMixerControl->VolumeWhich(), &volume, &errorString);

	if (errorString != NULL) {
		SetLabel(errorString);
		SetLimits(-60, 18);
	} else {
		SetLabel(B_TRANSLATE("Volume"));
		SetLimits((int32)floorf(fMixerControl->Minimum()),
			(int32)ceilf(fMixerControl->Maximum()));

		BMediaRoster* roster = BMediaRoster::CurrentRoster();
		if (roster != NULL && fMixerControl->GainNode() != media_node::null) {
			roster->StartWatching(this, fMixerControl->GainNode(),
				B_MEDIA_NEW_PARAMETER_VALUE);
		}
	}

	SetEnabled(errorString == NULL);

	fOriginalValue = (int32)volume;
	SetValue((int32)volume);
}


float
VolumeControl::_PointForValue(int32 value) const
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


bool
VolumeControl::_IsReplicant() const
{
	return dynamic_cast<VolumeWindow*>(Window()) == NULL;
}
