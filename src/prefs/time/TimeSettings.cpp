/*
 * TimeSettings.cpp
 * Time mccall@@digitalparadise.co.uk
 *
 */
 
#include <Application.h>
#include <FindDirectory.h>
#include <File.h>
#include <Path.h>
#include <String.h>
#include <stdio.h>

#include "TimeSettings.h"
#include "TimeMessages.h"

const char TimeSettings::kTimeSettingsFile[] = "Time_settings";

TimeSettings::TimeSettings()
{
	BPath path;
	
	if (find_directory(B_USER_SETTINGS_DIRECTORY,&path) == B_OK) {
		path.Append(kTimeSettingsFile);
		BFile file(path.Path(), B_READ_ONLY);
		if (file.InitCheck() == B_OK) {
			// Now read in the data
			if (file.Read(&fCorner, sizeof(BPoint)) != sizeof(BPoint)) {
					fCorner.x=50;
					fCorner.y=50;
				};
		}
		else {
			fCorner.x=50;
			fCorner.y=50;
		}
	}
	else
		be_app->PostMessage(ERROR_DETECTED);	
}

TimeSettings::~TimeSettings()
{	
	BPath path;

	if (find_directory(B_USER_SETTINGS_DIRECTORY,&path) < B_OK)
		return;

	path.Append(kTimeSettingsFile);

	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE);
	if (file.InitCheck() == B_OK) {
		file.Write(&fCorner, sizeof(BPoint));
	}
	
		
}

void
TimeSettings::SetWindowCorner(BPoint corner)
{
	fCorner=corner;
}
