/*
 * KeyboardSettings.cpp
 * Keyboard mccall@digitalparadise.co.uk
 *
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
	BPath path;
	
	if (find_directory(B_USER_SETTINGS_DIRECTORY,&path) == B_OK) {
		path.Append(kKeyboardSettingsFile);
		BFile file(path.Path(), B_READ_ONLY);
		if (file.InitCheck() == B_OK) {
			// Now read in the data
			if (file.Read(&fSettings, sizeof(kb_settings)) != sizeof(kb_settings)) {
				fSettings.key_repeat_delay=200;
        		fSettings.key_repeat_rate=250000;
			}

			if (file.Read(&fCorner, sizeof(BPoint)) != sizeof(BPoint)) {
					fCorner.x=50;
					fCorner.y=50;
				};
		}
		else {
			fCorner.x=50;
			fCorner.y=50;
			fSettings.key_repeat_delay=200;
        	fSettings.key_repeat_rate=250000;
		}
	}
	else
		be_app->PostMessage(ERROR_DETECTED);	
}

KeyboardSettings::~KeyboardSettings()
{	
	BPath path;

	if (find_directory(B_USER_SETTINGS_DIRECTORY,&path) < B_OK)
		return;

	path.Append(kKeyboardSettingsFile);

	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE);
	if (file.InitCheck() == B_OK) {
		file.Write(&fSettings, sizeof(kb_settings));
		file.Write(&fCorner, sizeof(BPoint));
	}
	
		
}

void
KeyboardSettings::SetWindowCorner(BPoint corner)
{
	fCorner=corner;
}

void
KeyboardSettings::SetKeyboardRepeatRate(int32 rate)
{
	fSettings.key_repeat_rate=rate;
}

void
KeyboardSettings::SetKeyboardRepeatDelay(int32 rate)
{
	fSettings.key_repeat_delay=rate;
}