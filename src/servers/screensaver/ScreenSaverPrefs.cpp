#include "ScreenSaverPrefs.h"
#include "StorageDefs.h"
#include "FindDirectory.h"
#include "File.h"
#include "Path.h"
#include "string.h" // Posix string functions
#include "String.h" // BString def
#include <stdio.h>

ScreenSaverPrefs::ScreenSaverPrefs(void)  {
}

// Load the flattened settings BMessage from disk and parse it.
bool ScreenSaverPrefs::LoadSettings(void) {
	bool ok;
	char pathAndFile[B_PATH_NAME_LENGTH]; 
	BPath path;

	status_t found=find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	if (ok=(found==B_OK)) {
		strncpy(pathAndFile,path.Path(),B_PATH_NAME_LENGTH-1);
		strncat(pathAndFile,"/ScreenSaver_settings",B_PATH_NAME_LENGTH-1);
		BFile ssSettings(pathAndFile,B_READ_ONLY);
		if (B_OK==ssSettings.InitCheck())
			{ // File exists. Unflatten the message and call the settings parser.
			BMessage settings;
			settings.Unflatten(&ssSettings);
			ok=parseSettings (&settings);
			}
		}
	return ok;		
}

void setOnValue(BMessage *msg, char *name, int &result) {
	int32 value;
	if (B_OK == msg->FindInt32(name,&value)) // If screen saving is even enabled
		result=value;
	// printf ("Read parameter %s, setting it to:%d\n",name,result);
}

bool ScreenSaverPrefs::parseSettings (BMessage *msg) {
	int temp;
	const char *strPtr,*passwordPointer;
	char pathAndFile[B_PATH_NAME_LENGTH]; // Yes, this is very long...
	BPath path;

	passwordPointer=password;
	setOnValue(msg,"timeflags",temp);
	if (temp) // If screen saver is enabled, set blanktime. 
		setOnValue(msg,"timefade",blankTime);
	else
		blankTime=-1;
	passwordTime=-1; // This is pessimistic - assume that password is off OR that a settings load will fail.
	setOnValue(msg,"lockenable",temp);
	if (temp && (B_OK == msg->FindString("lockmethod",&strPtr)))
		{ // Get the password. Either from the settings file (that's secure) or the networking file (also secure).
		if (strcmp(strPtr,"network")) // Not network, therefore from the settings file
			if (B_OK == msg->FindString("lockpassword",&passwordPointer))
				setOnValue(msg,"lockdelay",passwordTime);
		else // Get from the network file
			{
			status_t found=find_directory(B_USER_SETTINGS_DIRECTORY,&path);
			if (found==B_OK) {
				FILE *networkFile=NULL;
				char buffer[512],*start;
				strncpy(pathAndFile,path.Path(),B_PATH_NAME_LENGTH-1);
				strncat(pathAndFile,"/network",B_PATH_NAME_LENGTH-1);
				// This ugly piece opens the networking file and reads the password, if it exists.
				if ((networkFile=fopen(pathAndFile,"r")))
					while (buffer==fgets(buffer,512,networkFile))
						if ((start=strstr(buffer,"PASSWORD =")))
							strncpy(password, start+10,strlen(start-11));
				}
			}
		setOnValue(msg,"lockdelay",passwordTime);
		}
	if (B_OK != msg->FindString("modulename",&strPtr)) 
		blankTime=-1; // If the module doesn't exist, never blank.
	strcpy(moduleName,strPtr);
	BString stateMsgName("modulesettings_");
	stateMsgName+=moduleName;
	msg->FindMessage(stateMsgName.String(),&stateMsg); // Doesn't matter if it fails - stateMsg would just continue to be empty
	return true;
}
