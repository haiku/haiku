/*
 * Copyright (c) 2003-2005, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Jérôme Duval
 *		François Revol
 */
#ifndef VOLUMESLIDER_H
#define VOLUMESLIDER_H

#include <Window.h>
#include <Control.h>
#include <Bitmap.h>
#include <ParameterWeb.h>

#define VOLUME_USE_MIXER 0 /* default */
#define VOLUME_USE_PHYS_OUTPUT 1


class SliderView : public BControl {
	public:
		SliderView(BRect rect, BMessage *msg, const char* title, uint32 resizeFlags,
			int32 value);
		~SliderView();

		virtual void Draw(BRect);
		virtual void MouseMoved(BPoint point, uint32 transit, const BMessage *message);
		virtual void MouseUp(BPoint point);
		virtual void Pulse();

	private:
		BBitmap fLeftBitmap, fRightBitmap, fButtonBitmap;
		const char* fTitle;
};

class VolumeSlider : public BWindow {
	public:
		VolumeSlider(BRect frame, bool dontBeep=false, int32 volumeWhich=0);
		~VolumeSlider();

		void MessageReceived(BMessage*);
		void WindowActivated(bool active);

	private:
		void UpdateVolume(BContinuousParameter* param);

		media_node *fAudioMixerNode;
		BParameterWeb* fParamWeb;
		BContinuousParameter* fMixerParam;
		float fMin, fMax, fStep;
		bool fHasChanged;
		bool fDontBeep;
		SliderView *fSlider;
};

#endif	// VOLUMESLIDER_H
