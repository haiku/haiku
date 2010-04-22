/*
 * Copyright 2006-2008 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef TRANSPORT_CONTROL_GROUP_H
#define TRANSPORT_CONTROL_GROUP_H


// NOTE: Based on my code in the BeOS interface for the VLC media player
// that I did during the VLC 0.4.3 - 0.4.6 times. Code not written by me
// removed. -Stephan Aßmus


#include <View.h>


class PeakView;
class PlayPauseButton;
class PositionToolTip;
class TransportButton;
class SeekSlider;
class VolumeSlider;

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
								TransportControlGroup(BRect frame,
									bool useSkipButtons, bool usePeakView,
									bool useWindButtons);
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
			void				SetSkippable(bool backward, bool forward);

			void				SetAudioEnabled(bool enable);
			void				SetMuted(bool mute);

			void				SetVolume(float value);
			void				SetPosition(float value, bigtime_t position,
									bigtime_t duration);
			float				Position() const;

			PeakView*			GetPeakView() const
									{ return fPeakView; }

			void				SetDisabledString(const char* string);

private:
			void				_LayoutControls(BRect frame) const;
			BRect				_MinFrame() const;
			void				_LayoutControl(BView* view, BRect frame,
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
			PositionToolTip*	fPositionToolTip;
			PeakView*			fPeakView;
			VolumeSlider*		fVolumeSlider;

			TransportButton*	fSkipBack;
			TransportButton*	fSkipForward;
			TransportButton*	fRewind;
			TransportButton*	fForward;
			PlayPauseButton*	fPlayPause;
			TransportButton*	fStop;
			TransportButton*	fMute;

			float				fBottomControlHeight;
			float				fPeakViewMinWidth;
};

#endif	// TRANSPORT_CONTROL_GROUP_H
