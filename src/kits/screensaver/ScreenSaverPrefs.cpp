/*
 * Copyright 2003, Michael Phipps. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <StorageDefs.h>
#include <FindDirectory.h>
#include <File.h>
#include <Path.h>
#include <string.h> // Posix string functions
#include <String.h> // BString def
#include <stdio.h>

#include "ScreenSaverPrefs.h"

ScreenSaverPrefs::ScreenSaverPrefs() 
{
  	find_directory(B_USER_SETTINGS_DIRECTORY,&ssPath);
  	find_directory(B_USER_SETTINGS_DIRECTORY,&networkPath);
  	ssPath.Append("ScreenSaver_settings",true);
  	networkPath.Append("network",true);

	Defaults();
}


// Load the flattened settings BMessage from disk and parse it.
bool 
ScreenSaverPrefs::LoadSettings() 
{
	BFile ssSettings(ssPath.Path(),B_READ_ONLY);
	if (B_OK==ssSettings.InitCheck()) { // File exists. Unflatten the message and call the settings parser.
		BMessage settings;
		if (settings.Unflatten(&ssSettings)!=B_OK)
			return false;

		settings.PrintToStream();
		
		settings.FindRect("windowframe", &fWindowFrame);
		settings.FindInt32("windowtab", &fWindowTab);
		settings.FindInt32("timeflags", &fTimeFlags);

		int32 blank_time, standByTime, suspendTime, offTime, passwordTime;
		if (settings.FindInt32("timefade", &blank_time) == B_OK)
			fBlankTime = blank_time * 1000000;
		if (settings.FindInt32("timestandby", &standByTime) == B_OK)
			fStandByTime = standByTime * 1000000;
		if (settings.FindInt32("timesuspend", &suspendTime) == B_OK)
			fSuspendTime = suspendTime * 1000000;
		if (settings.FindInt32("timeoff", &offTime) == B_OK)
			fOffTime = offTime * 1000000;
		settings.FindInt32("cornernow", (int32*)&fBlankCorner);
		settings.FindInt32("cornernever", (int32*)&fNeverBlankCorner);
		settings.FindBool("lockenable", &fLockEnabled);
		if (settings.FindInt32("lockdelay", &passwordTime) == B_OK)
			fPasswordTime = passwordTime * 1000000;
		settings.FindString("lockpassword", &fPassword);
		settings.FindString("lockmethod", &fLockMethod);
		settings.FindString("modulename", &fModuleName);
		
		if (IsNetworkPassword()) {
			FILE *networkFile=NULL;
			char buffer[512],*start;
			// This ugly piece opens the networking file and reads the password, if it exists.
			if ((networkFile=fopen(networkPath.Path(),"r")))
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


BMessage *
ScreenSaverPrefs::GetSettings()
{
	BMessage *msg = new BMessage();
	msg->AddRect("windowframe", fWindowFrame);
	msg->AddInt32("windowtab", fWindowTab);
	msg->AddInt32("timeflags", fTimeFlags);
	msg->AddInt32("timefade", fBlankTime/1000000);
	msg->AddInt32("timestandby", fStandByTime/1000000);
	msg->AddInt32("timesuspend", fSuspendTime/1000000);
	msg->AddInt32("timeoff", fOffTime/1000000);
	msg->AddInt32("cornernow", fBlankCorner);
	msg->AddInt32("cornernever", fNeverBlankCorner);
	msg->AddBool("lockenable", fLockEnabled);
	msg->AddInt32("lockdelay", fPasswordTime/1000000);
	msg->AddString("lockpassword", fPassword);
	msg->AddString("lockmethod", fLockMethod);
	msg->AddString("modulename", fModuleName);
	
	return msg;
}


BMessage *
ScreenSaverPrefs::GetState(const char *name) 
{
	BString stateMsgName("modulesettings_");
	stateMsgName+=name;
	currentMessage.FindMessage(stateMsgName.String(),&stateMsg);	
	return &stateMsg;
}


void
ScreenSaverPrefs::SetState(const char *name,BMessage *msg) 
{
	BString stateMsgName("modulesettings_");
	stateMsgName+=name;
	if (B_NAME_NOT_FOUND == currentMessage.ReplaceMessage(stateMsgName.String(),msg))
		currentMessage.AddMessage(stateMsgName.String(),msg);
}


void 
ScreenSaverPrefs::SaveSettings() 
{
  	BMessage *settings = GetSettings();
	settings->PrintToStream();
	BFile file(ssPath.Path(), B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	if ((file.InitCheck()!=B_OK) || (settings->Flatten(&file)!=B_OK))
		fprintf(stderr, "Problem while saving screensaver preferences file!\n");
	delete settings;
}
