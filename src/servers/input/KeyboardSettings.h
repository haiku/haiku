/*
** Copyright 2004, the Haiku project. All rights reserved.
** Distributed under the terms of the Haiku License.
**
** Author : Jérôme Duval
** Original authors: mccall@digitalparadise.co.uk
*/

#ifndef KEYBOARD_SETTINGS_H_
#define KEYBOARD_SETTINGS_H_

#include <SupportDefs.h>
#include <kb_mouse_settings.h>

class KeyboardSettings {
public :
	KeyboardSettings();
	~KeyboardSettings();
	
	int32 KeyboardRepeatRate() const { return fSettings.key_repeat_rate; }
	void SetKeyboardRepeatRate(int32 rate);
	int32 KeyboardRepeatDelay() const { return fSettings.key_repeat_delay; }
	void SetKeyboardRepeatDelay(int32 rate);
	
private:
	static const char 	kKeyboardSettingsFile[];
	BPoint				fCorner;
	kb_settings			fSettings;
};

#endif
