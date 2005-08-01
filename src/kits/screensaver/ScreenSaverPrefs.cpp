/*
 * Copyright 2003, Michael Phipps. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <StorageDefs.h>
#include <String.h>
#include <stdio.h>
#include <string.h>

#include "ScreenSaverPrefs.h"

ScreenSaverPrefs::ScreenSaverPrefs() 
{
  	find_directory(B_USER_SETTINGS_DIRECTORY,&fSSPath);
  	find_directory(B_USER_SETTINGS_DIRECTORY,&fNetworkPath);
  	fSSPath.Append("ScreenSaver_settings",true);
  	fNetworkPath.Append("network",true);

	Defaults();
}


// Load the flattened settings BMessage from disk and parse it.
bool 
ScreenSaverPrefs::LoadSettings() 
{
	BFile ssSettings(fSSPath.Path(),B_READ_ONLY);
	if (B_OK==ssSettings.InitCheck()) { // File exists. Unflatten the message and call the settings parser.
		if (fSettings.Unflatten(&ssSettings)!=B_OK)
			return false;

		fSettings.PrintToStream();
		
		fSettings.FindRect("windowframe", &fWindowFrame);
		fSettings.FindInt32("windowtab", &fWindowTab);
		fSettings.FindInt32("timeflags", &fTimeFlags);

		int32 blank_time, standByTime, suspendTime, offTime, passwordTime;
		if (fSettings.FindInt32("timefade", &blank_time) == B_OK)
			fBlankTime = blank_time * 1000000;
		if (fSettings.FindInt32("timestandby", &standByTime) == B_OK)
			fStandByTime = standByTime * 1000000;
		if (fSettings.FindInt32("timesuspend", &suspendTime) == B_OK)
			fSuspendTime = suspendTime * 1000000;
		if (fSettings.FindInt32("timeoff", &offTime) == B_OK)
			fOffTime = offTime * 1000000;
		fSettings.FindInt32("cornernow", (int32*)&fBlankCorner);
		fSettings.FindInt32("cornernever", (int32*)&fNeverBlankCorner);
		fSettings.FindBool("lockenable", &fLockEnabled);
		if (fSettings.FindInt32("lockdelay", &passwordTime) == B_OK)
			fPasswordTime = passwordTime * 1000000;
		fSettings.FindString("lockpassword", &fPassword);
		fSettings.FindString("lockmethod", &fLockMethod);
		fSettings.FindString("modulename", &fModuleName);
		
		if (IsNetworkPassword()) {
			FILE *networkFile=NULL;
			char buffer[512],*start;
			// This ugly piece opens the networking file and reads the password, if it exists.
			if ((networkFile=fopen(fNetworkPath.Path(),"r")))
			while (buffer==fgets(buffer,512,networkFile))
				if ((start=strstr(buffer,"PASSWORD ="))) {
					char password[12];
					strncpy(password, start+10,strlen(start-11));
					fPassword = password;
                                }
        	}
	}
	return false;
}


void
ScreenSaverPrefs::Defaults()
{
	fWindowFrame = BRect(96.5, 77.0, 542.5, 402);
	fWindowTab = 0;
	fTimeFlags = 0;
	fBlankTime = 120;
	fStandByTime = 120;
	fSuspendTime = 120;
	fOffTime = 120;
	fBlankCorner = NONE;
	fNeverBlankCorner = NONE;
	fLockEnabled = false;
	fPasswordTime = 120;
	fPassword = "";
	fLockMethod = "custom";
	fModuleName = "";
}


BMessage &
ScreenSaverPrefs::GetSettings()
{
	fSettings.RemoveName("windowframe");
	fSettings.RemoveName("windowtab");
	fSettings.RemoveName("timeflags");
	fSettings.RemoveName("timefade");
	fSettings.RemoveName("timestandby");
	fSettings.RemoveName("timesuspend");
	fSettings.RemoveName("timeoff");
	fSettings.RemoveName("cornernow");
	fSettings.RemoveName("cornernever");
	fSettings.RemoveName("lockenable");
	fSettings.RemoveName("lockdelay");
	fSettings.RemoveName("lockpassword");
	fSettings.RemoveName("lockmethod");
	fSettings.RemoveName("modulename");
	fSettings.AddRect("windowframe", fWindowFrame);
	fSettings.AddInt32("windowtab", fWindowTab);
	fSettings.AddInt32("timeflags", fTimeFlags);
	fSettings.AddInt32("timefade", fBlankTime/1000000);
	fSettings.AddInt32("timestandby", fStandByTime/1000000);
	fSettings.AddInt32("timesuspend", fSuspendTime/1000000);
	fSettings.AddInt32("timeoff", fOffTime/1000000);
	fSettings.AddInt32("cornernow", fBlankCorner);
	fSettings.AddInt32("cornernever", fNeverBlankCorner);
	fSettings.AddBool("lockenable", fLockEnabled);
	fSettings.AddInt32("lockdelay", fPasswordTime/1000000);
	fSettings.AddString("lockpassword", fPassword);
	fSettings.AddString("lockmethod", fLockMethod);
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
	settings.PrintToStream();
	BFile file(fSSPath.Path(), B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	if ((file.InitCheck()!=B_OK) || (settings.Flatten(&file)!=B_OK))
		fprintf(stderr, "Problem while saving screensaver preferences file!\n");
}

