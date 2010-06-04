/*
 * Copyright 2005, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Inspired by SoundCapture from Be newsletter (Media Kit Basics: Consumers 
 * and Producers)
 */

#include <Bitmap.h>
#include <Debug.h>
#include <MessageFilter.h>
#include <Window.h>

#include <map>

#include "TransportButton.h"
#include "DrawingTidbits.h"

using std::map;

class BitmapStash {
// Bitmap stash is a simple class to hold all the lazily-allocated
// bitmaps that the TransportButton needs when rendering itself.
// signature is a combination of the different enabled, pressed, playing, etc.
// flavors of a bitmap. If the stash does not have a particular bitmap,
// it turns around to ask the button to create one and stores it for next time.
public:
	BitmapStash(TransportButton *);
	~BitmapStash();
	BBitmap *GetBitmap(uint32 signature);
	
private:
	TransportButton *owner;
	map<uint32, BBitmap *> stash;
};


BitmapStash::BitmapStash(TransportButton *owner)
	:	owner(owner)
{
}


BBitmap *
BitmapStash::GetBitmap(uint32 signature)
{
	if (stash.find(signature) == stash.end()) {
		BBitmap *newBits = owner->MakeBitmap(signature);
		ASSERT(newBits);
		stash[signature] = newBits;
	}
	
	return stash[signature];
}


BitmapStash::~BitmapStash()
{
	// delete all the bitmaps
	for (map<uint32, BBitmap *>::iterator i = stash.begin(); 
		i != stash.end(); i++)
		delete (*i).second;
}


class PeriodicMessageSender {
	// used to send a specified message repeatedly when holding down a button
public:
	static PeriodicMessageSender *Launch(BMessenger target,
		const BMessage *message, bigtime_t period);
	void Quit();

private:
	PeriodicMessageSender(BMessenger target, const BMessage *message,
		bigtime_t period);
	~PeriodicMessageSender() {}
		// use quit

	static status_t TrackBinder(void *);
	void Run();
	
	BMessenger target;
	BMessage message;

	bigtime_t period;
	
	bool requestToQuit;
};


PeriodicMessageSender::PeriodicMessageSender(BMessenger target,
	const BMessage *message, bigtime_t period)
	:	target(target),
		message(*message),
		period(period),
		requestToQuit(false)
{
}


PeriodicMessageSender *
PeriodicMessageSender::Launch(BMessenger target, const BMessage *message,
	bigtime_t period)
{
	PeriodicMessageSender *result = new PeriodicMessageSender(target, 
		message, period);
	thread_id thread = spawn_thread(&PeriodicMessageSender::TrackBinder,
		"ButtonRepeatingThread", B_NORMAL_PRIORITY, result);
	
	if (thread <= 0 || resume_thread(thread) != B_OK) {
		// didn't start, don't leak self
		delete result;
		result = 0;
	}

	return result;
}


void 
PeriodicMessageSender::Quit()
{
	requestToQuit = true;
}


status_t 
PeriodicMessageSender::TrackBinder(void *castToThis)
{
	((PeriodicMessageSender *)castToThis)->Run();
	return 0;
}


void 
PeriodicMessageSender::Run()
{
	for (;;) {
		snooze(period);
		if (requestToQuit)
			break;
		target.SendMessage(&message);
	}
	delete this;
}


class SkipButtonKeypressFilter : public BMessageFilter {
public:
	SkipButtonKeypressFilter(uint32 shortcutKey, uint32 shortcutModifier,
		TransportButton *target);

protected:
	filter_result Filter(BMessage *message, BHandler **handler);

private:
	uint32 shortcutKey;
	uint32 shortcutModifier;
	TransportButton *target;
};


SkipButtonKeypressFilter::SkipButtonKeypressFilter(uint32 shortcutKey,
	uint32 shortcutModifier, TransportButton *target)
	:	BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE),
		shortcutKey(shortcutKey),
		shortcutModifier(shortcutModifier),
		target(target)
{
}


filter_result 
SkipButtonKeypressFilter::Filter(BMessage *message, BHandler **handler)
{
	if (target->IsEnabled()
		&& (message->what == B_KEY_DOWN || message->what == B_KEY_UP)) {
		uint32 modifiers;
		uint32 rawKeyChar = 0;
		uint8 byte = 0;
		int32 key = 0;
		
		if (message->FindInt32("modifiers", (int32 *)&modifiers) != B_OK
			|| message->FindInt32("raw_char", (int32 *)&rawKeyChar) != B_OK
			|| message->FindInt8("byte", (int8 *)&byte) != B_OK
			|| message->FindInt32("key", &key) != B_OK)
			return B_DISPATCH_MESSAGE;

		modifiers &= B_SHIFT_KEY | B_COMMAND_KEY | B_CONTROL_KEY
			| B_OPTION_KEY | B_MENU_KEY;
			// strip caps lock, etc.

		if (modifiers == shortcutModifier && rawKeyChar == shortcutKey) {
			if (message->what == B_KEY_DOWN)
				target->ShortcutKeyDown();
			else
				target->ShortcutKeyUp();
			
			return B_SKIP_MESSAGE;
		}
	}

	// let others deal with this
	return B_DISPATCH_MESSAGE;
}


TransportButton::TransportButton(BRect frame, const char *name,
	const unsigned char *normalBits,
	const unsigned char *pressedBits,
	const unsigned char *disabledBits,
	BMessage *invokeMessage, BMessage *startPressingMessage,
	BMessage *pressingMessage, BMessage *donePressingMessage, bigtime_t period,
	uint32 key, uint32 modifiers, uint32 resizeFlags)
	:	BControl(frame, name, "", invokeMessage, resizeFlags, 
			B_WILL_DRAW | B_NAVIGABLE),
		bitmaps(new BitmapStash(this)),
		normalBits(normalBits),
		pressedBits(pressedBits),
		disabledBits(disabledBits),
		startPressingMessage(startPressingMessage),
		pressingMessage(pressingMessage),
		donePressingMessage(donePressingMessage),
		pressingPeriod(period),
		mouseDown(false),
		keyDown(false),
		messageSender(0),
		keyPressFilter(0)
{
	if (key)
		keyPressFilter = new SkipButtonKeypressFilter(key, modifiers, this);
}


void 
TransportButton::AttachedToWindow()
{
	_inherited::AttachedToWindow();
	if (keyPressFilter)
		Window()->AddCommonFilter(keyPressFilter);
	
	// transparent to reduce flicker
	SetViewColor(B_TRANSPARENT_COLOR);
}


void 
TransportButton::DetachedFromWindow()
{
	if (keyPressFilter) {
		Window()->RemoveCommonFilter(keyPressFilter);
		delete keyPressFilter;
	}
	_inherited::DetachedFromWindow();
}


TransportButton::~TransportButton()
{
	delete startPressingMessage;
	delete pressingMessage;
	delete donePressingMessage;
	delete bitmaps;
}


void 
TransportButton::WindowActivated(bool state)
{
	if (!state)
		ShortcutKeyUp();
	
	_inherited::WindowActivated(state);
}


void 
TransportButton::SetEnabled(bool on)
{
	_inherited::SetEnabled(on);
	if (!on)
		ShortcutKeyUp();	
}


const unsigned char *
TransportButton::BitsForMask(uint32 mask) const
{
	switch (mask) {
		case 0:
			return normalBits;
		case kDisabledMask:
			return disabledBits;
		case kPressedMask:
			return pressedBits;
		default:
			break;
	}	
	TRESPASS();
	return 0;
}


BBitmap *
TransportButton::MakeBitmap(uint32 mask)
{
	BBitmap *result = new BBitmap(Bounds(), B_CMAP8);
	result->SetBits(BitsForMask(mask), (Bounds().Width() + 1) 
		* (Bounds().Height() + 1), 0, B_CMAP8);

	ReplaceTransparentColor(result, Parent()->ViewColor());
	
	return result;
}


uint32 
TransportButton::ModeMask() const
{
	return (IsEnabled() ? 0 : kDisabledMask)
		| (Value() ? kPressedMask : 0);
}


void 
TransportButton::Draw(BRect)
{
	DrawBitmapAsync(bitmaps->GetBitmap(ModeMask()));
}


void 
TransportButton::StartPressing()
{
	SetValue(1);
	if (startPressingMessage)
		Invoke(startPressingMessage);
	
	if (pressingMessage) {
		ASSERT(pressingMessage);
		messageSender = PeriodicMessageSender::Launch(Messenger(),
			pressingMessage, pressingPeriod);
	}
}


void 
TransportButton::MouseCancelPressing()
{
	if (!mouseDown || keyDown)
		return;

	mouseDown = false;

	if (pressingMessage) {
		ASSERT(messageSender);
		PeriodicMessageSender *sender = messageSender;
		messageSender = 0;
		sender->Quit();
	}

	if (donePressingMessage)
		Invoke(donePressingMessage);
	SetValue(0);
}


void 
TransportButton::DonePressing()
{	
	if (pressingMessage) {
		ASSERT(messageSender);
		PeriodicMessageSender *sender = messageSender;
		messageSender = 0;
		sender->Quit();
	}

	Invoke();
	SetValue(0);
}


void 
TransportButton::MouseStartPressing()
{
	if (mouseDown)
		return;
	
	mouseDown = true;
	if (!keyDown)
		StartPressing();
}


void 
TransportButton::MouseDonePressing()
{
	if (!mouseDown)
		return;
	
	mouseDown = false;
	if (!keyDown)
		DonePressing();
}


void 
TransportButton::ShortcutKeyDown()
{
	if (!IsEnabled())
		return;

	if (keyDown)
		return;
	
	keyDown = true;
	if (!mouseDown)
		StartPressing();
}


void 
TransportButton::ShortcutKeyUp()
{
	if (!keyDown)
		return;
	
	keyDown = false;
	if (!mouseDown)
		DonePressing();
}


void 
TransportButton::MouseDown(BPoint)
{
	if (!IsEnabled())
		return;

	ASSERT(Window()->Flags() & B_ASYNCHRONOUS_CONTROLS);
	SetTracking(true);
	SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
	MouseStartPressing();
}

void 
TransportButton::MouseMoved(BPoint point, uint32 code, const BMessage *)
{
	if (IsTracking() && Bounds().Contains(point) != Value()) {
		if (!Value())
			MouseStartPressing();
		else
			MouseCancelPressing();
	}
}

void 
TransportButton::MouseUp(BPoint point)
{
	if (IsTracking()) {
		if (Bounds().Contains(point))
			MouseDonePressing();
		else
			MouseCancelPressing();
		SetTracking(false);
	}
}

void 
TransportButton::SetStartPressingMessage(BMessage *message)
{
	delete startPressingMessage;
	startPressingMessage = message;
}

void 
TransportButton::SetPressingMessage(BMessage *message)
{
	delete pressingMessage;
	pressingMessage = message;
}

void 
TransportButton::SetDonePressingMessage(BMessage *message)
{
	delete donePressingMessage;
	donePressingMessage = message;
}

void 
TransportButton::SetPressingPeriod(bigtime_t newTime)
{
	pressingPeriod = newTime;
}


PlayPauseButton::PlayPauseButton(BRect frame, const char *name,
	BMessage *invokeMessage, BMessage *blinkMessage,
	uint32 key, uint32 modifiers, uint32 resizeFlags)
	:	TransportButton(frame, name, kPlayButtonBitmapBits, 
			kPressedPlayButtonBitmapBits,
			kDisabledPlayButtonBitmapBits, invokeMessage, NULL,
			NULL, NULL, 0, key, modifiers, resizeFlags),
		fState(PlayPauseButton::kStopped),
		fLastModeMask(0),
		fRunner(NULL),
		fBlinkMessage(blinkMessage)
{
}

void 
PlayPauseButton::SetStopped()
{
	if (fState == kStopped || fState == kAboutToPlay)
		return;
	
	fState = kStopped;
	delete fRunner;
	fRunner = NULL;
	Invalidate();
}

void 
PlayPauseButton::SetPlaying()
{
	if (fState == kAboutToPause)
		return;
	
	// in playing state blink the LED on and off
	if (fState == kPlayingLedOn)
		fState = kPlayingLedOff;
	else
		fState = kPlayingLedOn;
	
	Invalidate();
}

const bigtime_t kPlayingBlinkPeriod = 600000;

void 
PlayPauseButton::SetPaused()
{
	if (fState == kAboutToPlay)
		return;

	// in paused state blink the LED on and off
	if (fState == kPausedLedOn)
		fState = kPausedLedOff;
	else
		fState = kPausedLedOn;
	
	Invalidate();
}

uint32 
PlayPauseButton::ModeMask() const
{
	if (!IsEnabled())
		return kDisabledMask;
	
	uint32 result = 0;

	if (Value())
		result = kPressedMask;

	if (fState == kPlayingLedOn || fState == kAboutToPlay)
		result |= kPlayingMask;
	else if (fState == kAboutToPause || fState == kPausedLedOn)	
		result |= kPausedMask;
	
	return result;
}

const unsigned char *
PlayPauseButton::BitsForMask(uint32 mask) const
{
	switch (mask) {
		case kPlayingMask:
			return kPlayingPlayButtonBitmapBits;
		case kPlayingMask | kPressedMask:
			return kPressedPlayingPlayButtonBitmapBits;
		case kPausedMask:
			return kPausedPlayButtonBitmapBits;
		case kPausedMask | kPressedMask:
			return kPressedPausedPlayButtonBitmapBits;
		default:
			return _inherited::BitsForMask(mask);
	}	
	TRESPASS();
	return 0;
}


void 
PlayPauseButton::StartPressing()
{
	if (fState == kPlayingLedOn || fState == kPlayingLedOff)
		fState = kAboutToPause;
	else
	 	fState = kAboutToPlay;
	
	_inherited::StartPressing();
}

void 
PlayPauseButton::MouseCancelPressing()
{
	if (fState == kAboutToPause)
	 	fState = kPlayingLedOn;
	else
		fState = kStopped;
	
	_inherited::MouseCancelPressing();
}

void 
PlayPauseButton::DonePressing()
{
	if (fState == kAboutToPause) {
	 	fState = kPausedLedOn;
	} else if (fState == kAboutToPlay) {
		fState = kPlayingLedOn;
		if (!fRunner && fBlinkMessage)
			fRunner = new BMessageRunner(Messenger(), fBlinkMessage, 
				kPlayingBlinkPeriod);
	}
	
	_inherited::DonePressing();
}


RecordButton::RecordButton(BRect frame, const char *name,
	BMessage *invokeMessage, BMessage *blinkMessage,
	uint32 key, uint32 modifiers, uint32 resizeFlags)
	:	TransportButton(frame, name, kRecordButtonBitmapBits, 
			kPressedRecordButtonBitmapBits,
			kDisabledRecordButtonBitmapBits, invokeMessage, NULL, NULL,
			NULL, 0, key, modifiers, resizeFlags),
		fState(RecordButton::kStopped),
		fLastModeMask(0),
		fRunner(NULL),
		fBlinkMessage(blinkMessage)
{
}

void 
RecordButton::SetStopped()
{
	if (fState == kStopped || fState == kAboutToRecord)
		return;
	
	fState = kStopped;
	delete fRunner;
	fRunner = NULL;
	Invalidate();
}

const bigtime_t kRecordingBlinkPeriod = 600000;

void 
RecordButton::SetRecording()
{
	if (fState == kAboutToStop)
		return;

	if (fState == kRecordingLedOff)
		fState = kRecordingLedOn;
	else
		fState = kRecordingLedOff;
	
	Invalidate();
}

uint32 
RecordButton::ModeMask() const
{
	if (!IsEnabled())
		return kDisabledMask;
	
	uint32 result = 0;

	if (Value())
		result = kPressedMask;

	if (fState == kAboutToStop || fState == kRecordingLedOn)
		result |= kRecordingMask;
	
	return result;
}

const unsigned char *
RecordButton::BitsForMask(uint32 mask) const
{
	switch (mask) {
		case kRecordingMask:
			return kRecordingRecordButtonBitmapBits;
		case kRecordingMask | kPressedMask:
			return kPressedRecordingRecordButtonBitmapBits;
		default:
			return _inherited::BitsForMask(mask);
	}	
	TRESPASS();
	return 0;
}


void 
RecordButton::StartPressing()
{
	if (fState == kRecordingLedOn || fState == kRecordingLedOff)
		fState = kAboutToStop;
	else
	 	fState = kAboutToRecord;
	
	_inherited::StartPressing();
}

void 
RecordButton::MouseCancelPressing()
{
	if (fState == kAboutToStop)
	 	fState = kRecordingLedOn;
	else
		fState = kStopped;
	
	_inherited::MouseCancelPressing();
}

void 
RecordButton::DonePressing()
{
	if (fState == kAboutToStop) {
	 	fState = kStopped;
	 	delete fRunner;
	 	fRunner = NULL;
	} else if (fState == kAboutToRecord) {
		fState = kRecordingLedOn;
		if (!fRunner && fBlinkMessage)
			fRunner = new BMessageRunner(Messenger(), fBlinkMessage, 
				kRecordingBlinkPeriod);
	}
	
	_inherited::DonePressing();
}
