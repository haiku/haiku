/*
 * Copyright 2004-2006, the Haiku project. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors in chronological order:
 *  mccall@digitalparadise.co.uk
 *  Jérôme Duval
 *  Marcus Overhagen
*/


#ifndef KEYBOARD_SETTINGS_H_
#define KEYBOARD_SETTINGS_H_

#include <SupportDefs.h>
#include "kb_mouse_settings.h"

class KeyboardSettings{
public :
	KeyboardSettings();
	~KeyboardSettings();

	void Revert();
	void Defaults();
	bool IsDefaultable();

	int32 KeyboardRepeatRate() const
		{ return fSettings.key_repeat_rate; }
	void SetKeyboardRepeatRate(int32 rate);

	bigtime_t KeyboardRepeatDelay() const
		{ return fSettings.key_repeat_delay; }
	void SetKeyboardRepeatDelay(bigtime_t delay);

private:
	kb_settings			fSettings;
	kb_settings			fOriginalSettings;
};

#endif
