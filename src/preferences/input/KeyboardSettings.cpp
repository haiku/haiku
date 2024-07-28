/*
 * Copyright 2004-2006, the Haiku project. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors in chronological order:
 *  mccall@digitalparadise.co.uk
 *  Jérôme Duval
 *  Marcus Overhagen
 */


#include "KeyboardSettings.h"

#include <stdio.h>


KeyboardSettings::KeyboardSettings()
{
	if (get_key_repeat_rate(&fSettings.key_repeat_rate) != B_OK)
		fSettings.key_repeat_rate = kb_default_key_repeat_rate;

	if (get_key_repeat_delay(&fSettings.key_repeat_delay) != B_OK)
		fSettings.key_repeat_delay = kb_default_key_repeat_delay;

	fOriginalSettings = fSettings;
}


KeyboardSettings::~KeyboardSettings()
{
}


void
KeyboardSettings::SetKeyboardRepeatRate(int32 rate)
{
	if (set_key_repeat_rate(rate) != B_OK)
		fprintf(stderr, "error while set_key_repeat_rate!\n");
	fSettings.key_repeat_rate = rate;
}


void
KeyboardSettings::SetKeyboardRepeatDelay(bigtime_t delay)
{
	if (set_key_repeat_delay(delay) != B_OK)
		fprintf(stderr, "error while set_key_repeat_delay!\n");
	fSettings.key_repeat_delay = delay;
}


void
KeyboardSettings::Revert()
{
	SetKeyboardRepeatDelay(fOriginalSettings.key_repeat_delay);
	SetKeyboardRepeatRate(fOriginalSettings.key_repeat_rate);
}


void
KeyboardSettings::Defaults()
{
	SetKeyboardRepeatDelay(kb_default_key_repeat_delay);
	SetKeyboardRepeatRate(kb_default_key_repeat_rate);
}


bool
KeyboardSettings::IsDefaultable() const
{
	return fSettings.key_repeat_delay != kb_default_key_repeat_delay
		|| fSettings.key_repeat_rate != kb_default_key_repeat_rate;
}
