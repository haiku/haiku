/*
 * Copyright 2003-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Phipps
 *		Jérôme Duval, jerome.duval@free.fr
 */


#include "ScreenSaverPrefs.h"

#include <Debug.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <StorageDefs.h>
#include <String.h>

#include <string.h>


ScreenSaverPrefs::ScreenSaverPrefs() 
{
	BPath path;
  	find_directory(B_USER_SETTINGS_DIRECTORY, &path);

	fSettingsPath = path;
	fSettingsPath.Append("ScreenSaver_settings", true);
	fNetworkPath = path;
  	fNetworkPath.Append("network", true);

	Defaults();
}


//! Load the flattened settings BMessage from disk and parse it.
bool
ScreenSaverPrefs::LoadSettings() 
{
	BFile file(fSettingsPath.Path(), B_READ_ONLY);
	if (file.InitCheck() != B_OK)
		return false;

	// File exists. Unflatten the message and call the settings parser.
	if (fSettings.Unflatten(&file) != B_OK)
		return false;

	PRINT_OBJECT(fSettings);

	fSettings.FindRect("windowframe", &fWindowFrame);
	fSettings.FindInt32("windowtab", &fWindowTab);
	fSettings.FindInt32("timeflags", &fTimeFlags);

	int32 blank_time, standByTime, suspendTime, offTime, passwordTime;
	if (fSettings.FindInt32("timefade", &blank_time) == B_OK)
		fBlankTime = blank_time * 1000000LL;
	if (fSettings.FindInt32("timestandby", &standByTime) == B_OK)
		fStandByTime = standByTime * 1000000LL;
	if (fSettings.FindInt32("timesuspend", &suspendTime) == B_OK)
		fSuspendTime = suspendTime * 1000000LL;
	if (fSettings.FindInt32("timeoff", &offTime) == B_OK)
		fOffTime = offTime * 1000000LL;
	fSettings.FindInt32("cornernow", (int32*)&fBlankCorner);
	fSettings.FindInt32("cornernever", (int32*)&fNeverBlankCorner);
	fSettings.FindBool("lockenable", &fLockEnabled);
	if (fSettings.FindInt32("lockdelay", &passwordTime) == B_OK)
		fPasswordTime = passwordTime * 1000000LL;
	fSettings.FindString("lockpassword", &fPassword);
	fSettings.FindString("lockmethod", &fLockMethod);
	fSettings.FindString("modulename", &fModuleName);

	if (IsNetworkPassword()) {
		FILE *networkFile = NULL;
		char buffer[512], *start;
		// This ugly piece opens the networking file and reads the password, if it exists.
		if ((networkFile = fopen(fNetworkPath.Path(),"r")))
		while (fgets(buffer, 512, networkFile) != NULL) {
			if ((start = strstr(buffer,"PASSWORD = ")) != NULL) {
				char password[14];
				strncpy(password, start+11,strlen(start+11)-1);
				password[strlen(start+11)-1] = 0;
				fPassword = password;
				break;
			}
		}
	}

	return true;
}


void
ScreenSaverPrefs::Defaults()
{
	fWindowFrame = BRect(96.5, 77.0, 542.5, 402);
	fWindowTab = 0;
	fTimeFlags = SAVER_DISABLED;
	fBlankTime = 120*1000000LL;
	fStandByTime = 120*1000000LL;
	fSuspendTime = 120*1000000LL;
	fOffTime = 120*1000000LL;
	fBlankCorner = NONE;
	fNeverBlankCorner = NONE;
	fLockEnabled = false;
	fPasswordTime = 120*1000000LL;
	fPassword = "";
	fLockMethod = "custom";
	fModuleName = "";
}


BMessage &
ScreenSaverPrefs::GetSettings()
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
ScreenSaverPrefs::GetState(const char *name, BMessage *stateMsg) 
{
	BString stateName("modulesettings_");
	stateName += name;
	return fSettings.FindMessage(stateName.String(), stateMsg);	
}


void
ScreenSaverPrefs::SetState(const char *name, BMessage *stateMsg) 
{
	BString stateName("modulesettings_");
	stateName += name;
	fSettings.RemoveName(stateName.String());
	fSettings.AddMessage(stateName.String(), stateMsg);
}


void 
ScreenSaverPrefs::SaveSettings() 
{
  	BMessage &settings = GetSettings();
	PRINT_OBJECT(settings);
	BFile file(fSettingsPath.Path(), B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	if (file.InitCheck() != B_OK || settings.Flatten(&file) != B_OK)
		fprintf(stderr, "Problem while saving screensaver preferences file!\n");
}

