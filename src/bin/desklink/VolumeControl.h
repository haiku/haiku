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


#include <String.h>
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
	virtual	void			DetachedFromWindow();

	virtual	void			MouseDown(BPoint where);
	virtual	void			MouseUp(BPoint where);
	virtual	void			MouseMoved(BPoint where, uint32 transit,
								const BMessage* dragMessage);

	virtual	void			MessageReceived(BMessage* message);
	virtual status_t		Invoke(BMessage* message = NULL);

	virtual void			DrawBar();
	virtual const char*		UpdateText() const;

private:
			void			_DisconnectVolume();
			void			_ConnectVolume();
			bool			_IsReplicant() const;
			float			_PointForValue(int32 value) const;

			mutable BString	fText;
			MixerControl*	fMixerControl;
			int32			fOriginalValue;
			bool			fBeep;

			bool			fSnapping;
			float			fMinSnap;
			float			fMaxSnap;

			int32			fConnectRetries;
			bool			fMediaServerRunning;
			bool			fAddOnServerRunning;
};

#endif	// VOLUME_SLIDER_H
