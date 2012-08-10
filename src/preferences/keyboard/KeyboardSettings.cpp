/*
 * Copyright 2004-2006, the Haiku project. All rights reserved.
 * Distributed under the terms of the Haiku License.
 *
 * Authors in chronological order:
 *  mccall@digitalparadise.co.uk
 *  Jérôme Duval
 *  Marcus Overhagen
*/
#include <FindDirectory.h>
#include <File.h>
#include <Path.h>
#include <stdio.h>
#include "KeyboardSettings.h"

// Keyboard setting file layout is like this:
// struct {
//    struct kb_settings; // managed by input server
//    BPoint corner;      // used by pref app
// }

KeyboardSettings::KeyboardSettings()
{
	if (get_key_repeat_rate(&fSettings.key_repeat_rate) != B_OK)
		fSettings.key_repeat_rate = kb_default_key_repeat_rate;

	if (get_key_repeat_delay(&fSettings.key_repeat_delay) != B_OK)
		fSettings.key_repeat_delay = kb_default_key_repeat_delay;

	fOriginalSettings = fSettings;
		
	BPath path;
	BFile file;
	
	fCorner.x = 150;
	fCorner.y = 100;
	
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status == B_OK) {
		status = path.Append(kb_settings_file);
		if (status == B_OK) {
			status = file.SetTo(path.Path(), B_READ_ONLY);
			if (status == B_OK) {
				if (file.ReadAt(sizeof(kb_settings), &fCorner, sizeof(fCorner)) != sizeof(fCorner)) {
					fCorner.x = 150;
					fCorner.y = 100;
				}
			}
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
	
	file.WriteAt(sizeof(kb_settings), &fCorner, sizeof(fCorner));
}


void
KeyboardSettings::SetWindowCorner(BPoint corner)
{
	fCorner = corner;
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
KeyboardSettings::Dump()
{
	printf("repeat rate: %" B_PRId32 "\n", fSettings.key_repeat_rate);
	printf("repeat delay: %" B_PRId64 "\n", fSettings.key_repeat_delay);
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
