/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

// NOTE: Based on my code in the BeOS interface for the VLC media player
// that I did during the VLC 0.4.3 - 0.4.6 times. Code not done by me
// removed. -Stephan Aßmus

#ifndef TRANSPORT_CONTROL_GROUP_H
#define TRANSPORT_CONTROL_GROUP_H

#include <View.h>

class PlayPauseButton;
class TransportButton;
class SeekSlider;
class VolumeSlider;

enum {
	PLAYBACK_STATE_PLAYING = 0,
	PLAYBACK_STATE_PAUSED,
	PLAYBACK_STATE_STOPPED,
};

enum {
	SKIP_BACK_ENABLED		= 1 << 0,
	SEEK_BACK_ENABLED		= 1 << 1,
	PLAYBACK_ENABLED		= 1 << 2,
	SEEK_FORWARD_ENABLED	= 1 << 3,
	SKIP_FORWARD_ENABLED	= 1 << 4,
	VOLUME_ENABLED			= 1 << 5,
	SEEK_ENABLED			= 1 << 6,
};

class TransportControlGroup : public BView {
 public:
								TransportControlGroup(BRect frame);
	virtual						~TransportControlGroup();

	// BView interface
	virtual	void				AttachedToWindow();
	virtual	void				FrameResized(float width, float height);
	virtual	void				GetPreferredSize(float* width, float* height);
	virtual	void				MessageReceived(BMessage* message);

	// TransportControlGroup
	virtual	uint32				EnabledButtons();
	virtual	void				TogglePlaying();
	virtual	void				Stop();
	virtual	void				Rewind();
	virtual	void				Forward();
	virtual	void				SkipBackward();
	virtual	void				SkipForward();
	virtual	void				VolumeChanged(float value);
	virtual	void				ToggleMute();
	virtual	void				PositionChanged(float value);

			void				SetEnabled(uint32 whichButtons);

			void				SetPlaybackState(uint32 state); 
			void				SetSkippable(bool backward,
											 bool forward);

			void				SetAudioEnabled(bool enable);
			void				SetMuted(bool mute);

			void				SetVolume(float value);
			void				SetPosition(float value);

 private:
			void				_LayoutControls(BRect frame) const;
			BRect				_MinFrame() const;
			void				_LayoutControl(BView* view,
											   BRect frame,
											   bool resizeWidth = false,
											   bool resizeHeight = false) const;

			void				_TogglePlaying();
			void				_Stop();
			void				_Rewind();
			void				_Forward();
			void				_SkipBackward();
			void				_SkipForward();
			void				_UpdateVolume();
			void				_ToggleMute();
			void				_UpdatePosition();

			float				_LinearToExponential(float db);
			float				_ExponentialToLinear(float db);
			float				_DbToGain(float db);
			float				_GainToDb(float gain);

			SeekSlider*			fSeekSlider;

			VolumeSlider*		fVolumeSlider;

			TransportButton*	fSkipBack;
			TransportButton*	fSkipForward;
			TransportButton*	fRewind;
			TransportButton*	fForward;
			PlayPauseButton*	fPlayPause;
			TransportButton*	fStop;
			TransportButton*	fMute;

			int					fCurrentStatus;
			float				fBottomControlHeight;
};

#endif	// TRANSPORT_CONTROL_GROUP_H
