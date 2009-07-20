/*
 * Copyright 2003-2009, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Phipps
 *		Jérôme Duval, jerome.duval@free.fr
 */

/*!	This is the class that wraps the screensaver settings, as well as the
	settings of the screensaver preference application.
*/


#include "ScreenSaverSettings.h"

#include <string.h>

#include <Debug.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <StorageDefs.h>
#include <String.h>


ScreenSaverSettings::ScreenSaverSettings()
{
	BPath path;
  	find_directory(B_USER_SETTINGS_DIRECTORY, &path);

	fSettingsPath = path;
	fSettingsPath.Append("ScreenSaver_settings", true);

	Defaults();
}


//! Load the flattened settings BMessage from disk and parse it.
bool
ScreenSaverSettings::Load()
{
	BFile file(fSettingsPath.Path(), B_READ_ONLY);
	if (file.InitCheck() != B_OK)
		return false;

	// File exists. Unflatten the message and call the settings parser.
	if (fSettings.Unflatten(&file) != B_OK)
		return false;

	PRINT_OBJECT(fSettings);

	BRect rect;
	if (fSettings.FindRect("windowframe", &rect) == B_OK)
		fWindowFrame = rect;
	int32 value;
	if (fSettings.FindInt32("windowtab", &value) == B_OK)
		fWindowTab = value;
	if (fSettings.FindInt32("timeflags", &value) == B_OK)
		fTimeFlags = value;

	if (fSettings.FindInt32("timefade", &value) == B_OK)
		fBlankTime = value * 1000000LL;
	if (fSettings.FindInt32("timestandby", &value) == B_OK)
		fStandByTime = value * 1000000LL;
	if (fSettings.FindInt32("timesuspend", &value) == B_OK)
		fSuspendTime = value * 1000000LL;
	if (fSettings.FindInt32("timeoff", &value) == B_OK)
		fOffTime = value * 1000000LL;

	if (fSettings.FindInt32("cornernow", &value) == B_OK)
		fBlankCorner = (screen_corner)value;
	if (fSettings.FindInt32("cornernever", &value) == B_OK)
		fNeverBlankCorner = (screen_corner)value;

	bool lockEnabled;
	if (fSettings.FindBool("lockenable", &lockEnabled) == B_OK)
		fLockEnabled = lockEnabled;
	if (fSettings.FindInt32("lockdelay", &value) == B_OK)
		fPasswordTime = value * 1000000LL;
	const char* string;
	if (fSettings.FindString("lockpassword", &string) == B_OK)
		fPassword = string;
	if (fSettings.FindString("lockmethod", &string) == B_OK)
		fLockMethod = string;

	if (fSettings.FindString("modulename", &string) == B_OK)
		fModuleName = string;

	if (IsNetworkPassword()) {
		// TODO: this does not work yet
	}

	return true;
}


void
ScreenSaverSettings::Defaults()
{
	fWindowFrame = BRect(96.5, 77.0, 542.5, 402);
	fWindowTab = 0;

	// Enable blanker + turning off the screen
	fTimeFlags = ENABLE_SAVER | ENABLE_DPMS_STAND_BY | ENABLE_DPMS_SUSPEND
		| ENABLE_DPMS_OFF;

	// Times are in microseconds
	fBlankTime = 900 * 1000000LL;	// 15 minutes

	// All these times are relative to fBlankTime; standby will start 5 minutes
	// after the screen saver.
	fStandByTime = 300 * 1000000LL;	// 5 minutes
	fSuspendTime = 300 * 1000000LL;
	fOffTime = 300 * 1000000LL;

	fBlankCorner = NO_CORNER;
	fNeverBlankCorner = NO_CORNER;

	fLockEnabled = false;
	// This time is NOT referenced to when the screen saver starts, but to when
	// idle time starts, like fBlankTime. By default it is the same as
	// fBlankTime.
	fPasswordTime = 900 * 1000000LL;
	fPassword = "";
	fLockMethod = "custom";

	fModuleName = "";
}


BMessage&
ScreenSaverSettings::Message()
{
	// We can't just empty the message, because the module states are stored
	// in this message as well.

	if (fSettings.ReplaceRect("windowframe", fWindowFrame) != B_OK)
		fSettings.AddRect("windowframe", fWindowFrame);
	if (fSettings.ReplaceInt32("windowtab", fWindowTab) != B_OK)
		fSettings.AddInt32("windowtab", fWindowTab);
	if (fSettings.ReplaceInt32("timeflags", fTimeFlags) != B_OK)
		fSettings.AddInt32("timeflags", fTimeFlags);
	if (fSettings.ReplaceInt32("timefade", fBlankTime / 1000000LL) != B_OK)
		fSettings.AddInt32("timefade", fBlankTime / 1000000LL);
	if (fSettings.ReplaceInt32("timestandby", fStandByTime / 1000000LL) != B_OK)
		fSettings.AddInt32("timestandby", fStandByTime / 1000000LL);
	if (fSettings.ReplaceInt32("timesuspend", fSuspendTime / 1000000LL) != B_OK)
		fSettings.AddInt32("timesuspend", fSuspendTime / 1000000LL);
	if (fSettings.ReplaceInt32("timeoff", fOffTime / 1000000LL) != B_OK)
		fSettings.AddInt32("timeoff", fOffTime / 1000000LL);
	if (fSettings.ReplaceInt32("cornernow", fBlankCorner) != B_OK)
		fSettings.AddInt32("cornernow", fBlankCorner);
	if (fSettings.ReplaceInt32("cornernever", fNeverBlankCorner) != B_OK)
		fSettings.AddInt32("cornernever", fNeverBlankCorner);
	if (fSettings.ReplaceBool("lockenable", fLockEnabled) != B_OK)
		fSettings.AddBool("lockenable", fLockEnabled);
	if (fSettings.ReplaceInt32("lockdelay", fPasswordTime / 1000000LL) != B_OK)
		fSettings.AddInt32("lockdelay", fPasswordTime / 1000000LL);
	if (fSettings.ReplaceString("lockmethod", fLockMethod) != B_OK)
		fSettings.AddString("lockmethod", fLockMethod);

	const char* password = IsNetworkPassword() ? "" : fPassword.String();
	if (fSettings.ReplaceString("lockpassword", password) != B_OK)
		fSettings.AddString("lockpassword", password);

	if (fSettings.ReplaceString("modulename", fModuleName) != B_OK)
		fSettings.AddString("modulename", fModuleName);

	return fSettings;
}


status_t
ScreenSaverSettings::GetModuleState(const char* name, BMessage* stateMsg)
{
	BString stateName("modulesettings_");
	stateName += name;
	return fSettings.FindMessage(stateName.String(), stateMsg);
}


void
ScreenSaverSettings::SetModuleState(const char* name, BMessage* stateMsg)
{
	BString stateName("modulesettings_");
	stateName += name;
	fSettings.RemoveName(stateName.String());
	fSettings.AddMessage(stateName.String(), stateMsg);
}


void
ScreenSaverSettings::Save()
{
  	BMessage &settings = Message();
	PRINT_OBJECT(settings);
	BFile file(fSettingsPath.Path(),
		B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	if (file.InitCheck() != B_OK || settings.Flatten(&file) != B_OK)
		fprintf(stderr, "Problem while saving screensaver preferences file!\n");
}

