/*
 * Copyright 2009, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 *		Jonas Sundström, jonas@kirilla.com
 */


#include "VolumeSlider.h"


VolumeSlider::VolumeSlider(BRect frame, const char* name, int32 minValue,
	int32 maxValue, BMessage* message, BHandler* target)
	:
	BSlider(frame, name, NULL, message, minValue, maxValue)
{
	SetTarget(target);
}


VolumeSlider::~VolumeSlider()
{
}


void
VolumeSlider::SetValue(int32 value)
{
	if (value == Value())
		return;

	BSlider::SetValue(value);
	Invoke();
}

