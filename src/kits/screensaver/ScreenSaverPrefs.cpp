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
}

// Load the flattened settings BMessage from disk and parse it.
bool 
ScreenSaverPrefs::LoadSettings(void) 
{
	bool ok;
	char pathAndFile[B_PATH_NAME_LENGTH]; 
	BPath path;

	status_t found=find_directory(B_USER_SETTINGS_DIRECTORY,&path);
	if (ok=(found==B_OK)) {
		strncpy(pathAndFile,path.Path(),B_PATH_NAME_LENGTH-1);
		strncat(pathAndFile,"/ScreenSaver_settings",B_PATH_NAME_LENGTH-1);
		BFile ssSettings(pathAndFile,B_READ_ONLY);
		if (B_OK==ssSettings.InitCheck()) { // File exists. Unflatten the message and call the settings parser.
			BMessage settings;
			settings.Unflatten(&ssSettings);
			ok=parseSettings (&settings);
			}
		}
	return ok;		
}


void 
setOnValue(BMessage *msg, char *name, int &result) 
{
	int32 value;
	if (B_OK == msg->FindInt32(name,&value)) // If screen saving is even enabled
		result=value;
}


bool 
ScreenSaverPrefs::parseSettings (BMessage *msg) 
{
	int temp;
	const char *strPtr;
	char pathAndFile[B_PATH_NAME_LENGTH]; 
	BPath path;

	// Set some defaults
	blankTime=passwordTime=-1;
	// Set the easy (integer) ones:
	setOnValue(msg,"windowtab",windowTab);
	setOnValue(msg,"timeflags",timeFlags);
	setOnValue(msg,"timefade",blankTime);
	setOnValue(msg,"timestandby",standbyTime);
	setOnValue(msg,"timesuspend",suspendTime);
	setOnValue(msg,"timeoff",offTime);
	setOnValue(msg,"cornernow",temp);
	blank=(arrowDirection)temp;
	setOnValue(msg,"cornernever",temp);
	neverBlank=(arrowDirection)temp;
	setOnValue(msg,"lockdelay",passwordTime);

	msg->FindRect("windowframe",&windowFrame);
	msg->FindBool("lockenable",&lockenable);
	msg->FindString("lockmethod",&strPtr);
	if (strPtr) {
		isNetworkPWD=(strcmp("custom",strPtr));
		 // Get the password. Either from the settings file or the networking file.
		if (isNetworkPWD) {
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
		} else {
			msg->FindString("lockpassword",&strPtr);
			if (strPtr)
				strncpy(password,strPtr,B_PATH_NAME_LENGTH-1);
		}
	} else 
		password[0]='\0';
	if (B_OK != msg->FindString("modulename",&strPtr)) 
		blankTime=-1; // If the module doesn't exist, never blank.
	if (strPtr)
		strncpy(moduleName,strPtr,B_PATH_NAME_LENGTH-1);
	else
		strcpy(moduleName,"Blackness");
	BString stateMsgName("modulesettings_");
	stateMsgName+=moduleName;
	msg->FindMessage(stateMsgName.String(),&stateMsg); // Doesn't matter if it fails - stateMsg would just continue to be empty
	return true;
}


BMessage *
ScreenSaverPrefs::GetSettings (void) 
{
	msg.MakeEmpty();
	msg.AddRect("windowframe",windowFrame);
	msg.AddInt32("windowtab",windowTab);
	msg.AddInt32("timeflags",timeFlags);
	msg.AddInt32("timefade", blankTime);
	msg.AddInt32("timestandby", standbyTime);
	msg.AddInt32("timesuspend", suspendTime);
	msg.AddInt32("timeoff", offTime);
	msg.AddInt32("cornernow", blank);
	msg.AddInt32("cornernever", neverBlank);
	msg.AddBool("lockenable", lockenable);
	msg.AddInt32("lockdelay", passwordTime);
	msg.AddString("modulename", moduleName);
	BString stateMsgName("modulesettings_");
	stateMsgName+=moduleName;
	msg.AddMessage(stateMsgName.String(),&stateMsg);	
	return &msg;
}


void 
ScreenSaverPrefs::SaveSettings (void) 
{
	GetSettings();
  	BPath path;
  	find_directory(B_USER_SETTINGS_DIRECTORY,&path);
  	path.Append("ScreenSaver_settings",true);
  	BFile file(path.Path(),B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	if ((file.InitCheck()!=B_OK) || (msg.Flatten(&file)!=B_OK))
		printf ("Problem with saving preferences file!\n");
}
