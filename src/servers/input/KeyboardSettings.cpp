/*
** Copyright 2004-2006, the Haiku project. All rights reserved.
** Distributed under the terms of the MIT License.
**
** Authors in chronological order:
**  mccall@digitalparadise.co.uk
**  Jérôme Duval
**  Marcus Overhagen
*/
 
#include <FindDirectory.h>
#include <File.h>
#include <Path.h>
#include "KeyboardSettings.h"

KeyboardSettings::KeyboardSettings()
{
	BPath path;
	BFile file;
	
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) < B_OK)
		goto err;
	if (path.Append(kb_settings_file) < B_OK)
		goto err;
	if (file.SetTo(path.Path(), B_READ_ONLY) < B_OK)
		goto err;
	if (file.Read(&fSettings, sizeof(kb_settings)) != sizeof(kb_settings))
		goto err;
		
	return;
err:
	fSettings.key_repeat_delay = kb_default_key_repeat_delay;
	fSettings.key_repeat_rate  = kb_default_key_repeat_rate;
}


KeyboardSettings::~KeyboardSettings()
{	
}


void
KeyboardSettings::SetKeyboardRepeatRate(int32 rate)
{
	fSettings.key_repeat_rate = rate;
	Save();
}


void
KeyboardSettings::SetKeyboardRepeatDelay(bigtime_t delay)
{
	fSettings.key_repeat_delay = delay;
	Save();
}


void
KeyboardSettings::Save()
{
	BPath path;
	BFile file;
	
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) < B_OK)
		return;
	if (path.Append(kb_settings_file) < B_OK)
		return;
	if (file.SetTo(path.Path(), B_WRITE_ONLY | B_CREATE_FILE) < B_OK)
		return;
	
	file.Write(&fSettings, sizeof(kb_settings));
}
