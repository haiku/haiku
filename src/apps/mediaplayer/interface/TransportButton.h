/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

// NOTE: this file originates from the Be Sample Code. See .cpp.

#ifndef TRANSPORT_BUTTON_H
#define TRANSPORT_BUTTON_H

#include <Control.h>

class BMessage;
class BBitmap;
class PeriodicMessageSender;
class BitmapStash;

// TransportButton must be installed into a window with B_ASYNCHRONOUS_CONTROLS on
// currently no button focus drawing

class TransportButton : public BControl {
 public:

								TransportButton(BRect frame,
												const char* name,
												const uchar* normalBits,
												const uchar* pressedBits,
												const uchar* disabledBits,
												BMessage* invokeMessage,
													// done pressing over button
												BMessage* startPressingMessage = 0,
													// just clicked button
												BMessage* pressingMessage = 0,
													// periodical still pressing
												BMessage* donePressing = 0,
													// tracked out of button
													// (didn't invoke)
												bigtime_t period = 0,
													// pressing message period
												uint32 key = 0,
													// optional shortcut key
												uint32 modifiers = 0,
													// optional shortcut key modifier
												uint32 resizeFlags =
													B_FOLLOW_LEFT | B_FOLLOW_TOP);
	
	virtual						~TransportButton();

			void				SetStartPressingMessage(BMessage* message);
			void				SetPressingMessage(BMessage* message);
			void				SetDonePressingMessage(BMessage* message);
			void				SetPressingPeriod(bigtime_t);

	virtual	void				SetEnabled(bool enabled);

 protected:		

	enum {
		kDisabledMask = 0x1,
		kPressedMask = 0x2
	};
	
	virtual	void				AttachedToWindow();
	virtual	void				DetachedFromWindow();
	virtual	void				Draw(BRect updateRect);
	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
										   const BMessage* dragMessage);
	virtual	void				MouseUp(BPoint where);
	virtual	void				WindowActivated(bool active);

	virtual	BBitmap*			MakeBitmap(uint32);
		// lazy bitmap builder
	
	virtual	uint32				ModeMask() const;
		// mode mask corresponding to the current button state
		// - determines which bitmap will be used

	virtual	const uchar*		BitsForMask(uint32 mask) const;
		// pick the right bits based on a mode mask

		// overriding class can add swapping between two pairs of bitmaps, etc.
	virtual void				StartPressing();
	virtual void				MouseCancelPressing();
	virtual void				DonePressing();

private:
			void				ShortcutKeyDown();
			void				ShortcutKeyUp();
	
			void				MouseStartPressing();
			void				MouseDonePressing();

			BitmapStash*		bitmaps;
		// using BitmapStash* here instead of a direct member so that
		// the class can be private in the .cpp file

		// bitmap bits used to build bitmaps for the different states
			const uchar*		normalBits;
			const uchar*		pressedBits;
			const uchar*		disabledBits;
	
			BMessage*			startPressingMessage;
			BMessage*			pressingMessage;
			BMessage*			donePressingMessage;
			bigtime_t			pressingPeriod;
			
			bool				mouseDown;
			bool				keyDown;
			PeriodicMessageSender*	messageSender;
			BMessageFilter*		keyPressFilter;

	typedef BControl _inherited;
	
	friend class SkipButtonKeypressFilter;
	friend class BitmapStash;
};

class PlayPauseButton : public TransportButton {
	// Knows about playing and paused states, blinks
	// the pause LED during paused state
 public:
								PlayPauseButton(BRect frame,
												const char *name,
												const uchar* normalBits,
												const uchar* pressedBits,
												const uchar* disabledBits,
												const uchar* normalPlayingBits,
												const uchar* pressedPlayingBits,
												const uchar* normalPausedBits,
												const uchar* pressedPausedBits,
												BMessage* invokeMessage,
													// done pressing over button
												uint32 key = 0,
													// optional shortcut key
												uint32 modifiers = 0,
													// optional shortcut key modifier
												uint32 resizeFlags =
													B_FOLLOW_LEFT | B_FOLLOW_TOP);
	
	// These need get called periodically to update the button state
	// OK to call them over and over - once the state is correct, the call
	// is very low overhead
			void				SetStopped();
			void				SetPlaying();
			void				SetPaused();

protected:
	
	virtual uint32				ModeMask() const;
	virtual const uchar*		BitsForMask(uint32) const;

	virtual void				StartPressing();
	virtual void				MouseCancelPressing();
	virtual void				DonePressing();

private:
			const uchar*		normalPlayingBits;
			const uchar*		pressedPlayingBits;
			const uchar*		normalPausedBits;
			const uchar*		pressedPausedBits;
	
	enum PlayState {
		kStopped,
		kAboutToPlay,
		kPlaying,
		kAboutToPause,
		kPausedLedOn,
		kPausedLedOff
	};
	
	enum {
		kPlayingMask = 0x4,
		kPausedMask = 0x8
	};
	
			PlayState			state;
			bigtime_t			lastPauseBlinkTime;
			uint32				lastModeMask;
	
	typedef TransportButton _inherited;
};

#endif // TRANSPORT_BUTTON_H
