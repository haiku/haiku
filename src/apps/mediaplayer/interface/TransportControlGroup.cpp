/*
 * Copyright 2006-2010 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */


// NOTE: Based on my code in the BeOS interface for the VLC media player
// that I did during the VLC 0.4.3 - 0.4.6 times. Code not written by me
// removed. -Stephan Aßmus


#include "TransportControlGroup.h"

#include <stdio.h>
#include <string.h>

#include <Shape.h>
#include <SpaceLayoutItem.h>
#include <String.h>
#include <ToolTipManager.h>
#include <Window.h>

#include "DurationView.h"
#include "PeakView.h"
#include "PlaybackState.h"
#include "PlayPauseButton.h"
#include "PositionToolTip.h"
#include "SeekSlider.h"
#include "SymbolButton.h"
#include "VolumeSlider.h"

enum {
	MSG_SEEK				= 'seek',
	MSG_PLAY				= 'play',
	MSG_STOP				= 'stop',
	MSG_REWIND				= 'rwnd',
	MSG_FORWARD				= 'frwd',
	MSG_SKIP_BACKWARDS		= 'skpb',
	MSG_SKIP_FORWARD		= 'skpf',
	MSG_SET_VOLUME			= 'stvl',
	MSG_SET_MUTE			= 'stmt',
	MSG_DURATION_TOOLTIP	= 'msdt'
};

// the range of the volume sliders (in dB)
#define kVolumeDbMax	6.0
#define kVolumeDbMin	-60.0
// a power function for non linear sliders
#define kVolumeDbExpPositive 1.4	// for dB values > 0
#define kVolumeDbExpNegative 1.9	// for dB values < 0

#define kVolumeFactor	100
#define kPositionFactor	3000


TransportControlGroup::TransportControlGroup(BRect frame, bool useSkipButtons,
		bool usePeakView, bool useWindButtons)
	:
	BGroupView(B_VERTICAL, 0),
	fSeekSlider(NULL),
	fDurationView(NULL),
	fPositionToolTip(NULL),
	fPeakView(NULL),
	fVolumeSlider(NULL),
	fSkipBack(NULL),
	fSkipForward(NULL),
	fRewind(NULL),
	fForward(NULL),
	fPlayPause(NULL),
	fStop(NULL),
	fMute(NULL),
	fSymbolScale(1.0f),
	fLastEnabledButtons(0)
{
	// Pick a symbol size based on the current system font size, but make
	// sure the size is uneven, so the pointy shapes have their middle on
	// a pixel instead of between two pixels.
	float symbolHeight = int(be_plain_font->Size() / 1.33) | 1;

	BGroupView* seekGroup = new BGroupView(B_HORIZONTAL, 0);
	fSeekLayout = seekGroup->GroupLayout();
	GroupLayout()->AddView(seekGroup);

	// Seek slider
	fSeekSlider = new SeekSlider("seek slider", new BMessage(MSG_SEEK),
		0, kPositionFactor);
	fSeekLayout->AddView(fSeekSlider);

	fPositionToolTip = new PositionToolTip();
	fSeekSlider->SetToolTip(fPositionToolTip);

	// Duration view
	fDurationView = new DurationView("duration view");
	fSeekLayout->AddView(fDurationView);

	// Buttons

	uint32 topBottomBorder = BControlLook::B_TOP_BORDER
		| BControlLook::B_BOTTOM_BORDER;

	if (useSkipButtons) {
		// Skip Back
		fSkipBack = new SymbolButton(B_EMPTY_STRING,
			_CreateSkipBackwardsShape(symbolHeight),
			new BMessage(MSG_SKIP_BACKWARDS),
			BControlLook::B_LEFT_BORDER | topBottomBorder);
		// Skip Foward
		fSkipForward = new SymbolButton(B_EMPTY_STRING,
			_CreateSkipForwardShape(symbolHeight),
			new BMessage(MSG_SKIP_FORWARD),
			BControlLook::B_RIGHT_BORDER | topBottomBorder);
	}

	if (useWindButtons) {
		// Rewind
		fRewind = new SymbolButton(B_EMPTY_STRING,
			_CreateRewindShape(symbolHeight), new BMessage(MSG_REWIND),
			useSkipButtons ? topBottomBorder
				: BControlLook::B_LEFT_BORDER | topBottomBorder);
		// Forward
		fForward = new SymbolButton(B_EMPTY_STRING,
			_CreateForwardShape(symbolHeight), new BMessage(MSG_FORWARD),
			useSkipButtons ? topBottomBorder
				: BControlLook::B_RIGHT_BORDER | topBottomBorder);
	}

	// Play Pause
	fPlayPause = new PlayPauseButton(B_EMPTY_STRING,
		_CreatePlayShape(symbolHeight), _CreatePauseShape(symbolHeight),
		new BMessage(MSG_PLAY), useWindButtons || useSkipButtons
			? topBottomBorder
			: topBottomBorder | BControlLook::B_LEFT_BORDER);

	// Stop
	fStop = new SymbolButton(B_EMPTY_STRING,
		_CreateStopShape(symbolHeight), new BMessage(MSG_STOP),
		useWindButtons || useSkipButtons ? topBottomBorder
			: topBottomBorder | BControlLook::B_RIGHT_BORDER);

	// Mute
	fMute = new SymbolButton(B_EMPTY_STRING,
		_CreateSpeakerShape(floorf(symbolHeight * 0.9)),
		new BMessage(MSG_SET_MUTE), 0);

	// Volume Slider
	fVolumeSlider = new VolumeSlider("volume slider",
		_DbToGain(_ExponentialToLinear(kVolumeDbMin)) * kVolumeFactor,
		_DbToGain(_ExponentialToLinear(kVolumeDbMax)) * kVolumeFactor,
		kVolumeFactor, new BMessage(MSG_SET_VOLUME));
	fVolumeSlider->SetValue(_DbToGain(_ExponentialToLinear(0.0))
		* kVolumeFactor);

	// Peak view
	if (usePeakView)
		fPeakView = new PeakView("peak view", false, false);

	// Layout the controls

	BGroupView* buttonGroup = new BGroupView(B_HORIZONTAL, 0);
	BGroupLayout* buttonLayout = buttonGroup->GroupLayout();

	if (fSkipBack != NULL)
		buttonLayout->AddView(fSkipBack);
	if (fRewind != NULL)
		buttonLayout->AddView(fRewind);
	buttonLayout->AddView(fPlayPause);
	buttonLayout->AddView(fStop);
	if (fForward != NULL)
		buttonLayout->AddView(fForward);
	if (fSkipForward != NULL)
		buttonLayout->AddView(fSkipForward);

	BGroupView* controlGroup = new BGroupView(B_HORIZONTAL, 0);
	GroupLayout()->AddView(controlGroup);
	fControlLayout = controlGroup->GroupLayout();
	fControlLayout->AddView(buttonGroup, 0.6f);
	fControlLayout->AddItem(BSpaceLayoutItem::CreateHorizontalStrut(5));
	fControlLayout->AddView(fMute);
	fControlLayout->AddView(fVolumeSlider);
	if (fPeakView != NULL)
		fControlLayout->AddView(fPeakView, 0.6f);

	// Figure out the visual insets of the slider bounds towards the slider
	// bar, and use that as insets for the rest of the layout.
	float inset = fSeekSlider->BarFrame().left;
	float hInset = inset - fSeekSlider->BarFrame().top;
	if (hInset < 0.0f)
		hInset = 0.0f;

	fSeekLayout->SetInsets(0, hInset, 5, 0);
	fControlLayout->SetInsets(inset, hInset, inset, inset);

	BSize size = fControlLayout->MinSize();
	size.width *= 3;
	size.height = B_SIZE_UNSET;
	fControlLayout->SetExplicitMaxSize(size);
	fControlLayout->SetExplicitAlignment(BAlignment(B_ALIGN_CENTER,
		B_ALIGN_TOP));
}


TransportControlGroup::~TransportControlGroup()
{
	if (!fSeekSlider->IsEnabled())
		fPositionToolTip->ReleaseReference();
}


void
TransportControlGroup::AttachedToWindow()
{
	SetEnabled(EnabledButtons());

	// we are now a valid BHandler
	fSeekSlider->SetTarget(this);
	fVolumeSlider->SetTarget(this);
	if (fSkipBack)
		fSkipBack->SetTarget(this);
	if (fSkipForward)
		fSkipForward->SetTarget(this);
	if (fRewind)
		fRewind->SetTarget(this);
	if (fForward)
		fForward->SetTarget(this);
	fPlayPause->SetTarget(this);
	fStop->SetTarget(this);
	fMute->SetTarget(this);
}


void
TransportControlGroup::GetPreferredSize(float* _width, float* _height)
{
	BSize size = GroupLayout()->MinSize();
	if (_width != NULL)
		*_width = size.width;
	if (_height != NULL)
		*_height = size.height;
}


void
TransportControlGroup::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_PLAY:
			_TogglePlaying();
			break;
		case MSG_STOP:
			_Stop();
			break;

		case MSG_REWIND:
			_Rewind();
			break;
		case MSG_FORWARD:
			_Forward();
			break;

		case MSG_SKIP_BACKWARDS:
			_SkipBackward();
			break;
		case MSG_SKIP_FORWARD:
			_SkipForward();
			break;

		case MSG_SET_VOLUME:
			_UpdateVolume();
			break;
		case MSG_SET_MUTE:
			_ToggleMute();
			break;

		case MSG_SEEK:
			_UpdatePosition();
			break;

		case MSG_DURATION_TOOLTIP:
		{
			BToolTipManager* manager = BToolTipManager::Manager();
			BPoint tipPoint;
			GetMouse(&tipPoint, NULL, false);
			manager->ShowTip(fPositionToolTip, tipPoint, this);
			break;
		}

		default:
			BView::MessageReceived(message);
			break;
	}
}


// #pragma mark - default implementation for the virtuals


uint32
TransportControlGroup::EnabledButtons()
{
	return fLastEnabledButtons;
}


void TransportControlGroup::TogglePlaying() {}
void TransportControlGroup::Stop() {}
void TransportControlGroup::Rewind() {}
void TransportControlGroup::Forward() {}
void TransportControlGroup::SkipBackward() {}
void TransportControlGroup::SkipForward() {}
void TransportControlGroup::VolumeChanged(float value) {}
void TransportControlGroup::ToggleMute() {}
void TransportControlGroup::PositionChanged(float value) {}


// #pragma mark -


float
TransportControlGroup::_LinearToExponential(float dbIn)
{
	float db = dbIn;
	if (db >= 0) {
		db = db * (pow(fabs(kVolumeDbMax), (1.0 / kVolumeDbExpPositive))
			/ fabs(kVolumeDbMax));
		db = pow(db, kVolumeDbExpPositive);
	} else {
		db = -db;
		db = db * (pow(fabs(kVolumeDbMin), (1.0 / kVolumeDbExpNegative))
			/ fabs(kVolumeDbMin));
		db = pow(db, kVolumeDbExpNegative);
		db = -db;
	}
	return db;
}


float
TransportControlGroup::_ExponentialToLinear(float dbIn)
{
	float db = dbIn;
	if (db >= 0) {
		db = pow(db, (1.0 / kVolumeDbExpPositive));
		db = db * (fabs(kVolumeDbMax) / pow(fabs(kVolumeDbMax),
			(1.0 / kVolumeDbExpPositive)));
	} else {
		db = -db;
		db = pow(db, (1.0 / kVolumeDbExpNegative));
		db = db * (fabs(kVolumeDbMin) / pow(fabs(kVolumeDbMin),
			(1.0 / kVolumeDbExpNegative)));
		db = -db;
	}
	return db;
}


float
TransportControlGroup::_DbToGain(float db)
{
	return pow(10.0, db / 20.0);
}


float
TransportControlGroup::_GainToDb(float gain)
{
	return 20.0 * log10(gain);
}


// #pragma mark -


void
TransportControlGroup::SetEnabled(uint32 buttons)
{
	if (!LockLooper())
		return;

	fLastEnabledButtons = buttons;

	fSeekSlider->SetEnabled(buttons & SEEK_ENABLED);
	fSeekSlider->SetToolTip((buttons & SEEK_ENABLED) != 0
		? fPositionToolTip : NULL);

	fVolumeSlider->SetEnabled(buttons & VOLUME_ENABLED);
	fMute->SetEnabled(buttons & VOLUME_ENABLED);

	if (fSkipBack)
		fSkipBack->SetEnabled(buttons & SKIP_BACK_ENABLED);
	if (fSkipForward)
		fSkipForward->SetEnabled(buttons & SKIP_FORWARD_ENABLED);
	if (fRewind)
		fRewind->SetEnabled(buttons & SEEK_BACK_ENABLED);
	if (fForward)
		fForward->SetEnabled(buttons & SEEK_FORWARD_ENABLED);

	fPlayPause->SetEnabled(buttons & PLAYBACK_ENABLED);
	fStop->SetEnabled(buttons & PLAYBACK_ENABLED);

	UnlockLooper();
}


// #pragma mark -


void
TransportControlGroup::SetPlaybackState(uint32 state)
{
	if (!LockLooper())
		return;

	switch (state) {
		case PLAYBACK_STATE_PLAYING:
			fPlayPause->SetPlaying();
			break;
		case PLAYBACK_STATE_PAUSED:
			fPlayPause->SetPaused();
			break;
		case PLAYBACK_STATE_STOPPED:
			fPlayPause->SetStopped();
			break;
	}

	UnlockLooper();
}


void
TransportControlGroup::SetSkippable(bool backward, bool forward)
{
	if (!LockLooper())
		return;

	if (fSkipBack)
		fSkipBack->SetEnabled(backward);
	if (fSkipForward)
		fSkipForward->SetEnabled(forward);

	UnlockLooper();
}


// #pragma mark -


void
TransportControlGroup::SetAudioEnabled(bool enabled)
{
	if (!LockLooper())
		return;

	fMute->SetEnabled(enabled);
	fVolumeSlider->SetEnabled(enabled);

	UnlockLooper();
}


void
TransportControlGroup::SetMuted(bool mute)
{
	if (!LockLooper())
		return;

	fVolumeSlider->SetMuted(mute);

	UnlockLooper();
}


void
TransportControlGroup::SetVolume(float value)
{
	float db = _GainToDb(value);
	float exponential = _LinearToExponential(db);
	float gain = _DbToGain(exponential);
	int32 pos = (int32)(floorf(gain * kVolumeFactor + 0.5));

	fVolumeSlider->SetValue(pos);
}


void
TransportControlGroup::SetAudioChannelCount(int32 count)
{
	fPeakView->SetChannelCount(count);
}


void
TransportControlGroup::SetPosition(float value, bigtime_t position,
	bigtime_t duration)
{
	fPositionToolTip->Update(position, duration);
	fDurationView->Update(position, duration);

	if (fSeekSlider->IsTracking())
		return;

	fSeekSlider->SetPosition(value);
}


float
TransportControlGroup::Position() const
{
	return fSeekSlider->Position();
}


void
TransportControlGroup::SetDisabledString(const char* string)
{
	fSeekSlider->SetDisabledString(string);
}


void
TransportControlGroup::SetSymbolScale(float scale)
{
	if (scale == fSymbolScale)
		return;

	fSymbolScale = scale;

	if (fSeekSlider != NULL)
		fSeekSlider->SetSymbolScale(scale);
	if (fVolumeSlider != NULL) {
		fVolumeSlider->SetBarThickness(fVolumeSlider->PreferredBarThickness()
			* scale);
	}
	if (fDurationView != NULL)
		fDurationView->SetSymbolScale(scale);

	float symbolHeight = int(scale * be_plain_font->Size() / 1.33) | 1;

	if (fSkipBack != NULL)
		fSkipBack->SetSymbol(_CreateSkipBackwardsShape(symbolHeight));
	if (fSkipForward != NULL)
		fSkipForward->SetSymbol(_CreateSkipForwardShape(symbolHeight));
	if (fRewind != NULL)
		fRewind->SetSymbol(_CreateRewindShape(symbolHeight));
	if (fForward != NULL)
		fForward->SetSymbol(_CreateForwardShape(symbolHeight));
	if (fPlayPause != NULL) {
		fPlayPause->SetSymbols(_CreatePlayShape(symbolHeight),
			_CreatePauseShape(symbolHeight));
	}
	if (fStop != NULL)
		fStop->SetSymbol(_CreateStopShape(symbolHeight));
	if (fMute != NULL)
		fMute->SetSymbol(_CreateSpeakerShape(floorf(symbolHeight * 0.9)));

	// Figure out the visual insets of the slider bounds towards the slider
	// bar, and use that as insets for the rest of the layout.
	float barInset = fSeekSlider->BarFrame().left;
	float inset = barInset * scale;
	float hInset = inset - fSeekSlider->BarFrame().top;
	if (hInset < 0.0f)
		hInset = 0.0f;

	fSeekLayout->SetInsets(inset - barInset, hInset, inset, 0);
	fSeekLayout->SetSpacing(inset - barInset);
	fControlLayout->SetInsets(inset, hInset, inset, inset);
	fControlLayout->SetSpacing(inset - barInset);

	ResizeTo(Bounds().Width(), GroupLayout()->MinSize().height);
}

// #pragma mark -


void
TransportControlGroup::_TogglePlaying()
{
	TogglePlaying();
}


void
TransportControlGroup::_Stop()
{
	fPlayPause->SetStopped();
	Stop();
}


void
TransportControlGroup::_Rewind()
{
	Rewind();
}


void
TransportControlGroup::_Forward()
{
	Forward();
}


void
TransportControlGroup::_SkipBackward()
{
	SkipBackward();
}


void
TransportControlGroup::_SkipForward()
{
	SkipForward();
}


void
TransportControlGroup::_UpdateVolume()
{
	float pos = fVolumeSlider->Value() / (float)kVolumeFactor;
	float db = _ExponentialToLinear(_GainToDb(pos));
	float gain = _DbToGain(db);
	VolumeChanged(gain);
}


void
TransportControlGroup::_ToggleMute()
{
	fVolumeSlider->SetMuted(!fVolumeSlider->IsMuted());
	ToggleMute();
}


void
TransportControlGroup::_UpdatePosition()
{
	PositionChanged(fSeekSlider->Value() / (float)kPositionFactor);

	BMessage msg(MSG_DURATION_TOOLTIP);
	Window()->PostMessage(&msg, this);
}


// #pragma mark -


BShape*
TransportControlGroup::_CreateSkipBackwardsShape(float height) const
{
	BShape* shape = new BShape();

	float stopWidth = ceilf(height / 6);

	shape->MoveTo(BPoint(-stopWidth, height));
	shape->LineTo(BPoint(0, height));
	shape->LineTo(BPoint(0, 0));
	shape->LineTo(BPoint(-stopWidth, 0));
	shape->Close();

	shape->MoveTo(BPoint(0, height / 2));
	shape->LineTo(BPoint(height, height));
	shape->LineTo(BPoint(height, 0));
	shape->Close();

	shape->MoveTo(BPoint(height, height / 2));
	shape->LineTo(BPoint(height * 2, height));
	shape->LineTo(BPoint(height * 2, 0));
	shape->Close();

	return shape;
}


BShape*
TransportControlGroup::_CreateSkipForwardShape(float height) const
{
	BShape* shape = new BShape();

	shape->MoveTo(BPoint(height, height / 2));
	shape->LineTo(BPoint(0, height));
	shape->LineTo(BPoint(0, 0));
	shape->Close();

	shape->MoveTo(BPoint(height * 2, height / 2));
	shape->LineTo(BPoint(height, height));
	shape->LineTo(BPoint(height, 0));
	shape->Close();

	float stopWidth = ceilf(height / 6);

	shape->MoveTo(BPoint(height * 2, height));
	shape->LineTo(BPoint(height * 2 + stopWidth, height));
	shape->LineTo(BPoint(height * 2 + stopWidth, 0));
	shape->LineTo(BPoint(height * 2, 0));
	shape->Close();

	return shape;
}


BShape*
TransportControlGroup::_CreateRewindShape(float height) const
{
	BShape* shape = new BShape();

	shape->MoveTo(BPoint(0, height / 2));
	shape->LineTo(BPoint(height, height));
	shape->LineTo(BPoint(height, 0));
	shape->Close();

	shape->MoveTo(BPoint(height, height / 2));
	shape->LineTo(BPoint(height * 2, height));
	shape->LineTo(BPoint(height * 2, 0));
	shape->Close();

	return shape;
}


BShape*
TransportControlGroup::_CreateForwardShape(float height) const
{
	BShape* shape = new BShape();

	shape->MoveTo(BPoint(height, height / 2));
	shape->LineTo(BPoint(0, height));
	shape->LineTo(BPoint(0, 0));
	shape->Close();

	shape->MoveTo(BPoint(height * 2, height / 2));
	shape->LineTo(BPoint(height, height));
	shape->LineTo(BPoint(height, 0));
	shape->Close();

	return shape;
}


BShape*
TransportControlGroup::_CreatePlayShape(float height) const
{
	BShape* shape = new BShape();

	float step = floorf(height / 8);

	shape->MoveTo(BPoint(height + step, height / 2));
	shape->LineTo(BPoint(-step, height + step));
	shape->LineTo(BPoint(-step, 0 - step));
	shape->Close();

	return shape;
}


BShape*
TransportControlGroup::_CreatePauseShape(float height) const
{
	BShape* shape = new BShape();

	float stemWidth = floorf(height / 3);

	shape->MoveTo(BPoint(0, height));
	shape->LineTo(BPoint(stemWidth, height));
	shape->LineTo(BPoint(stemWidth, 0));
	shape->LineTo(BPoint(0, 0));
	shape->Close();

	shape->MoveTo(BPoint(height - stemWidth, height));
	shape->LineTo(BPoint(height, height));
	shape->LineTo(BPoint(height, 0));
	shape->LineTo(BPoint(height - stemWidth, 0));
	shape->Close();

	return shape;
}


BShape*
TransportControlGroup::_CreateStopShape(float height) const
{
	BShape* shape = new BShape();

	shape->MoveTo(BPoint(0, height));
	shape->LineTo(BPoint(height, height));
	shape->LineTo(BPoint(height, 0));
	shape->LineTo(BPoint(0, 0));
	shape->Close();

	return shape;
}


static void
add_bow(BShape* shape, float offset, float size, float height, float step)
{
	float width = floorf(size * 2 / 3);
	float outerControlHeight = size * 2 / 3;
	float outerControlWidth = size / 4;
	float innerControlHeight = size / 2;
	float innerControlWidth = size / 5;
	// left/bottom
	shape->MoveTo(BPoint(offset, height / 2 + size));
	// outer bow, to middle
	shape->BezierTo(
		BPoint(offset + outerControlWidth, height / 2 + size),
		BPoint(offset + width, height / 2 + outerControlHeight),
		BPoint(offset + width, height / 2)
	);
	// outer bow, to left/top
	shape->BezierTo(
		BPoint(offset + width, height / 2 - outerControlHeight),
		BPoint(offset + outerControlWidth, height / 2 - size),
		BPoint(offset, height / 2 - size)
	);
	// inner bow, to middle
	shape->BezierTo(
		BPoint(offset + innerControlWidth, height / 2 - size),
		BPoint(offset + width - step, height / 2 - innerControlHeight),
		BPoint(offset + width - step, height / 2)
	);
	// inner bow, back to left/bottom
	shape->BezierTo(
		BPoint(offset + width - step, height / 2 + innerControlHeight),
		BPoint(offset + innerControlWidth, height / 2 + size),
		BPoint(offset, height / 2 + size)
	);
	shape->Close();
}


BShape*
TransportControlGroup::_CreateSpeakerShape(float height) const
{
	BShape* shape = new BShape();

	float step = floorf(height / 8);
	float magnetWidth = floorf(height / 5);
	float chassieWidth = floorf(height / 1.5);
	float chassieHeight = floorf(height / 4);

	shape->MoveTo(BPoint(0, height - step));
	shape->LineTo(BPoint(magnetWidth, height - step));
	shape->LineTo(BPoint(magnetWidth, height / 2 + chassieHeight));
	shape->LineTo(BPoint(magnetWidth + chassieWidth - step, height + step));
	shape->LineTo(BPoint(magnetWidth + chassieWidth, height + step));
	shape->LineTo(BPoint(magnetWidth + chassieWidth, -step));
	shape->LineTo(BPoint(magnetWidth + chassieWidth - step, -step));
	shape->LineTo(BPoint(magnetWidth, height / 2 - chassieHeight));
	shape->LineTo(BPoint(magnetWidth, step));
	shape->LineTo(BPoint(0, step));
	shape->Close();

	float offset = magnetWidth + chassieWidth + step * 2;
	add_bow(shape, offset, 3 * step, height, step * 2);
	offset += step * 2;
	add_bow(shape, offset, 5 * step, height, step * 2);
	offset += step * 2;
	add_bow(shape, offset, 7 * step, height, step * 2);

	return shape;
}
