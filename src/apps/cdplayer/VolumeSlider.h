/*
 * Copyright 2009, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 *		Jonas Sundström, jonas@kirilla.com
 */
#ifndef VOLUME_SLIDER_H
#define VOLUME_SLIDER_H


#include <Slider.h>


class VolumeSlider : public BSlider {
public:
							VolumeSlider(BRect frame, const char* name,
								int32 minValue, int32 maxValue,
								BMessage* message = NULL,
								BHandler* target = NULL);

	virtual					~VolumeSlider();
	virtual void			SetValue(int32 value);

};

#endif	// VOLUME_SLIDER_H

