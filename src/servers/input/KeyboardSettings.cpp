/*
** Copyright 2004, the Haiku project. All rights reserved.
** Distributed under the terms of the Haiku License.
**
** Author : Jérôme Duval
** Original authors: mccall@digitalparadise.co.uk
*/
 
#include <Application.h>
#include <FindDirectory.h>
#include <File.h>
#include <Path.h>
#include <String.h>
#include <stdio.h>

#include "KeyboardSettings.h"

KeyboardSettings::KeyboardSettings()
{
	fSettings.key_repeat_delay=200;
	fSettings.key_repeat_rate=250000;
	
	BPath path;
	
	if (find_directory(B_USER_SETTINGS_DIRECTORY,&path) == B_OK) {
		path.Append(kb_settings_file);
		BFile file(path.Path(), B_READ_ONLY);
		if (file.InitCheck() == B_OK) {
			// Now read in the data
			if (file.Read(&fSettings, sizeof(kb_settings)) != sizeof(kb_settings)) {
				fSettings.key_repeat_delay=200;
				fSettings.key_repeat_rate=250000;
			}
		}
	}	
}

KeyboardSettings::~KeyboardSettings()
{	
	BPath path;

	if (find_directory(B_USER_SETTINGS_DIRECTORY,&path) < B_OK)
		return;

	path.Append(kb_settings_file);

	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE);
	if (file.InitCheck() == B_OK) {
		file.Write(&fSettings, sizeof(kb_settings));
	}		
}


void
KeyboardSettings::SetKeyboardRepeatRate(int32 rate)
{
	fSettings.key_repeat_rate = rate;
}


void
KeyboardSettings::SetKeyboardRepeatDelay(int32 rate)
{
	fSettings.key_repeat_delay = rate;
}
