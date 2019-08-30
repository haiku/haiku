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

#include <FindDirectory.h>
#include <File.h>
#include <Path.h>
#include <stdio.h>


KeyboardSettings::KeyboardSettings()
{
	if (get_key_repeat_rate(&fSettings.key_repeat_rate) != B_OK)
		fSettings.key_repeat_rate = kb_default_key_repeat_rate;

	if (get_key_repeat_delay(&fSettings.key_repeat_delay) != B_OK)
		fSettings.key_repeat_delay = kb_default_key_repeat_delay;

	fOriginalSettings = fSettings;

	BPath path;
	BFile file;

	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status == B_OK) {
		status = path.Append(kb_settings_file);
		if (status == B_OK) {
			status = file.SetTo(path.Path(), B_READ_ONLY);
				}
			}
}


KeyboardSettings::~KeyboardSettings()
{
	BPath path;
	BFile file;

	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) < B_OK)
		return;

	if (path.Append(kb_settings_file) < B_OK)
		return;

	// be careful: don't create the file if it doesn't already exist
	if (file.SetTo(path.Path(), B_WRITE_ONLY) < B_OK)
		return;
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
KeyboardSettings::IsDefaultable()
{
	return fSettings.key_repeat_delay != kb_default_key_repeat_delay
		|| fSettings.key_repeat_rate != kb_default_key_repeat_rate;
}
