/*
 * Copyright 2003-2009, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Jérôme Duval
 *		François Revol
 *		Axel Dörfler, axeld@pinc-software.de.
 */
#ifndef VOLUME_SLIDER_H
#define VOLUME_SLIDER_H


#include <Slider.h>

class MixerControl;


class VolumeControl : public BSlider {
public:
							VolumeControl(int32 volumeWhich, bool beep,
								BMessage* message);
							VolumeControl(BMessage* archive);
	virtual					~VolumeControl();

	static	VolumeControl*	Instantiate(BMessage* archive);
	virtual	status_t		Archive(BMessage* archive, bool deep = true) const;

protected:
	virtual	void			AttachedToWindow();
	virtual	void			MouseDown(BPoint where);
	virtual	void			MessageReceived(BMessage* message);
	virtual status_t		Invoke(BMessage* message = NULL);

	virtual void			DrawBar();
	virtual const char*		UpdateText() const;

private:
			void			_InitVolume(int32 volumeWhich);
			bool			_IsReplicant() const;

			mutable	char	fText[64];
			MixerControl*	fMixerControl;
			int32			fOriginalValue;
			bool			fBeep;
};

#endif	// VOLUME_SLIDER_H
