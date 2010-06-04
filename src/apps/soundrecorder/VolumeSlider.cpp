/*
 * Copyright 2005, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Inspired by SoundCapture from Be newsletter (Media Kit Basics: Consumers
 * and Producers)
 */

#include <stdio.h>

#include "VolumeSlider.h"
#include "icon_button.h"

#define VOLUME_CHANGED 'vlcg'
#define RATIO 2.0f

VolumeSlider::VolumeSlider(BRect rect, const char *title, uint32 resizeFlags)
	: BControl(rect, "slider", NULL, new BMessage(VOLUME_CHANGED),
		resizeFlags, B_WILL_DRAW),
	fLeftBitmap(BRect(0, 0, kLeftVolumeWidth - 1, kLeftVolumeHeight - 1),
		B_CMAP8),
	fRightBitmap(BRect(0, 0, kRightVolumeWidth - 1, kRightVolumeHeight - 1),
		B_CMAP8),
	fButtonBitmap(BRect(0, 0, kThumbWidth - 1, kThumbHeight - 1), B_CMAP8),
	fSoundPlayer(NULL)
{
	fLeftBitmap.SetBits(kLeftVolumeBits, kLeftVolumeWidth * kLeftVolumeHeight,
		0, B_CMAP8);
	fRightBitmap.SetBits(kRightVolumeBits, kRightVolumeWidth * kRightVolumeHeight,
		0, B_CMAP8);
	fButtonBitmap.SetBits(kThumbBits, kThumbWidth * kThumbHeight, 0, B_CMAP8);

	fRight = Bounds().right - 15;
}


VolumeSlider::~VolumeSlider()
{

}


void
VolumeSlider::Draw(BRect updateRect)
{
	SetHighColor(189, 186, 189);
	StrokeLine(BPoint(11, 1), BPoint(fRight, 1));
	SetHighColor(0, 0, 0);
	StrokeLine(BPoint(11, 2), BPoint(fRight, 2));
	SetHighColor(255, 255, 255);
	StrokeLine(BPoint(11, 14), BPoint(fRight, 14));
	SetHighColor(231, 227, 231);
	StrokeLine(BPoint(11, 15), BPoint(fRight, 15));

	SetLowColor(ViewColor());

	SetDrawingMode(B_OP_OVER);

	DrawBitmapAsync(&fLeftBitmap, BPoint(5, 1));
	DrawBitmapAsync(&fRightBitmap, BPoint(fRight + 1, 1));

	float position = 11 + (fRight - 11) * (fSoundPlayer
		? fSoundPlayer->Volume() / RATIO : 0);
	SetHighColor(102, 152, 102);
	FillRect(BRect(11, 3, position, 4));
	SetHighColor(152, 203, 152);
	FillRect(BRect(11, 5, position, 13));

	if (fSoundPlayer)
		SetHighColor(152, 152, 152);
	else
		SetHighColor(200, 200, 200);
	FillRect(BRect(position, 3, fRight, 13));

	SetHighColor(102, 152, 102);
	for (int i = 15; i <= fRight + 1; i += 5) {
		if (i > position)
			SetHighColor(128, 128, 128);
		StrokeLine(BPoint(i, 8), BPoint(i, 9));
	}

	DrawBitmapAsync(&fButtonBitmap, BPoint(position - 5, 3));

	Sync();

	SetDrawingMode(B_OP_COPY);
}


void
VolumeSlider::MouseMoved(BPoint point, uint32 transit, const BMessage *message)
{
	if (!IsTracking())
		return;

	uint32 mouseButtons;
	BPoint where;
	GetMouse(&where, &mouseButtons, true);

	// button not pressed, exit
	if (! (mouseButtons & B_PRIMARY_MOUSE_BUTTON)) {
		Invoke();
		SetTracking(false);
	}

	if (!fSoundPlayer || !Bounds().InsetBySelf(2, 2).Contains(point))
		return;

	_UpdateVolume(point);
}


void
VolumeSlider::MouseDown(BPoint point)
{
	if (!fSoundPlayer || !Bounds().InsetBySelf(2, 2).Contains(point))
		return;

	_UpdateVolume(point);
	SetTracking(true);
	SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
}


void
VolumeSlider::MouseUp(BPoint point)
{
	if (!IsTracking())
		return;
	if (fSoundPlayer && Bounds().InsetBySelf(2, 2).Contains(point)) {
		_UpdateVolume(point);
	}

	Invoke();
	SetTracking(false);
	Draw(Bounds());
	Flush();
}


void
VolumeSlider::_UpdateVolume(BPoint point)
{
	fVolume = MIN(MAX(point.x, 11), fRight);
	fVolume = (fVolume - 11) / (fRight - 11);
	fVolume = MAX(MIN(fVolume,1), 0);
	Draw(Bounds());
	Flush();
	if (fSoundPlayer)
		fSoundPlayer->SetVolume(fVolume * RATIO);
}

void
VolumeSlider::SetSoundPlayer(BSoundPlayer *player)
{
	fSoundPlayer = player;
	Invalidate();
}


SpeakerView::SpeakerView(BRect rect, uint32 resizeFlags)
	: BBox(rect, "speaker", resizeFlags, B_WILL_DRAW, B_NO_BORDER),
	fSpeakerBitmap(BRect(0, 0, kSpeakerIconBitmapWidth - 1,
		kSpeakerIconBitmapHeight - 1), B_CMAP8)
{
	fSpeakerBitmap.SetBits(kSpeakerIconBits, kSpeakerIconBitmapWidth
		* kSpeakerIconBitmapHeight, 0, B_CMAP8);
}


SpeakerView::~SpeakerView()
{

}


void
SpeakerView::Draw(BRect updateRect)
{
	SetDrawingMode(B_OP_OVER);

	DrawBitmap(&fSpeakerBitmap);

	SetDrawingMode(B_OP_COPY);
}
