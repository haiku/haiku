#include "ScreenSaverPrefs.h"
#include "StorageDefs.h"
#include "FindDirectory.h"
#include "File.h"
#include "Path.h"
#include "string.h" // Posix string functions
#include "String.h" // BString def
#include <stdio.h>

ScreenSaverPrefs::ScreenSaverPrefs(void)  
{
  	find_directory(B_USER_SETTINGS_DIRECTORY,&ssPath);
  	find_directory(B_USER_SETTINGS_DIRECTORY,&networkPath);
  	ssPath.Append("ScreenSaver_settings",true);
  	networkPath.Append("network",true);
}

// Load the flattened settings BMessage from disk and parse it.
bool 
ScreenSaverPrefs::LoadSettings(void) 
{
	BFile ssSettings(ssPath.Path(),B_READ_ONLY);
	if (B_OK==ssSettings.InitCheck()) { // File exists. Unflatten the message and call the settings parser.
		BMessage settings;
		settings.Unflatten(&ssSettings);
		return true;
		}
	return false;
}

const char *
ScreenSaverPrefs::Password(void) {
	if (IsNetworkPassword()) {
		FILE *networkFile=NULL;
		char buffer[512],*start;
		// This ugly piece opens the networking file and reads the password, if it exists.
		if ((networkFile=fopen(networkPath.Path(),"r")))
			while (buffer==fgets(buffer,512,networkFile))
				if ((start=strstr(buffer,"PASSWORD ="))) {
					strncpy(password, start+10,strlen(start-11));
					return password;
					}
	}
	else 
		return getString("lockpassword");
}

BMessage *
ScreenSaverPrefs::GetState(const char *name) {
	BString stateMsgName("modulesettings_");
	stateMsgName+=name;
	currentMessage.FindMessage(stateMsgName.String(),&stateMsg);	
	return &stateMsg;
}

void
ScreenSaverPrefs::SetState(const char *name,BMessage *msg) {
	BString stateMsgName("modulesettings_");
	stateMsgName+=name;
	if (B_NAME_NOT_FOUND == currentMessage.ReplaceMessage(stateMsgName.String(),msg))
		currentMessage.AddMessage(stateMsgName.String(),msg);
}

void 
ScreenSaverPrefs::SaveSettings (void) {
  	BFile file(ssPath.Path(),B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	if ((file.InitCheck()!=B_OK) || (currentMessage.Flatten(&file)!=B_OK))
		printf ("Problem with saving preferences file!\n");
}
