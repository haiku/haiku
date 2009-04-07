/*
 * Copyright 2003-2009, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Jérôme Duval
 *		François Revol
 */
#ifndef VOLUME_SLIDER_H
#define VOLUME_SLIDER_H


#include <Window.h>


class BSlider;
class MixerControl;

class VolumeSlider : public BWindow {
public:
							VolumeSlider(BRect frame, bool dontBeep = false,
								int32 volumeWhich = 0);
							~VolumeSlider();

protected:
			void			MessageReceived(BMessage* message);

private:
			MixerControl*	fMixerControl;
			bool			fHasChanged;
			bool			fDontBeep;
			BSlider*		fSlider;
};

#endif	// VOLUME_SLIDER_H
