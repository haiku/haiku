/*
** Copyright 2004, the Haiku project. All rights reserved.
** Distributed under the terms of the Haiku License.
**
** Author : mccall@digitalparadise.co.uk, Jérôme Duval
*/
 
#include <Application.h>
#include <FindDirectory.h>
#include <File.h>
#include <Path.h>
#include <String.h>
#include <stdio.h>

#include "KeyboardSettings.h"
#include "KeyboardMessages.h"

const char KeyboardSettings::kKeyboardSettingsFile[] = "Keyboard_settings";

KeyboardSettings::KeyboardSettings()
{
	if (get_key_repeat_rate(&fSettings.key_repeat_rate)!=B_OK)
		fprintf(stderr, "error while get_key_repeat_rate!\n");
	if (get_key_repeat_delay(&fSettings.key_repeat_delay)!=B_OK)
		fprintf(stderr, "error while get_key_repeat_delay!\n");
		
	fCorner.x = 50;
	fCorner.y = 50;
	
	BPath path;
	
	if (find_directory(B_USER_SETTINGS_DIRECTORY,&path) == B_OK) {
		path.Append(kKeyboardSettingsFile);
		BFile file(path.Path(), B_READ_ONLY);
		if (file.InitCheck() == B_OK) {
			// Now read in the data

			file.ReadAt(sizeof(kb_settings), &fCorner, sizeof(BPoint));
		}
	} else
		be_app->PostMessage(ERROR_DETECTED);
		
	fOriginalSettings = fSettings;
}


KeyboardSettings::~KeyboardSettings()
{	
	BPath path;

	if (find_directory(B_USER_SETTINGS_DIRECTORY,&path) < B_OK)
		return;

	path.Append(kKeyboardSettingsFile);

	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE);
	if (file.InitCheck() == B_OK) {
		file.WriteAt(sizeof(kb_settings), &fCorner, sizeof(BPoint));
	}
	
		
}


void
KeyboardSettings::SetWindowCorner(BPoint corner)
{
	fCorner = corner;
}


void
KeyboardSettings::SetKeyboardRepeatRate(int32 rate)
{
	if (set_key_repeat_rate(rate)!=B_OK)
		fprintf(stderr, "error while set_key_repeat_rate!\n");
	fSettings.key_repeat_rate = rate;
}


void
KeyboardSettings::SetKeyboardRepeatDelay(bigtime_t delay)
{
	if (set_key_repeat_delay(delay)!=B_OK)
		fprintf(stderr, "error while set_key_repeat_delay!\n");
	fSettings.key_repeat_delay = delay;
}


void
KeyboardSettings::Dump()
{
	printf("repeat rate: %ld\n", fSettings.key_repeat_rate);
	printf("repeat delay: %Ld\n", fSettings.key_repeat_delay);
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
	SetKeyboardRepeatDelay(250000);
	SetKeyboardRepeatRate(200);
}
