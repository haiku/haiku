/*
 * DataTranslationsSettings.cpp
 * DataTranslations Oliver Siebenmarck
 *
 */
 
#include <Application.h>
#include <FindDirectory.h>
#include <File.h>
#include <Path.h>
#include <String.h>
#include <stdio.h>

#include "DataTranslationsSettings.h"
#include "DataTranslationsMessages.h"

const char DataTranslationsSettings::kDataTranslationsSettingsFile[] = "Translation Settings";

DataTranslationsSettings::DataTranslationsSettings()
{
	BPath path;
	
	if (find_directory(B_USER_SETTINGS_DIRECTORY,&path) == B_OK) {
		path.Append(kDataTranslationsSettingsFile);
		BFile file(path.Path(), B_READ_ONLY);
		if (file.InitCheck() == B_OK) {
			// Now read in the data
			file.Seek(-8,SEEK_END); // Skip over the first 497 bytes to just the window
									// position.
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
		
	fCorner.PrintToStream();
}

DataTranslationsSettings::~DataTranslationsSettings()
{	
	BPath path;

	if (find_directory(B_USER_SETTINGS_DIRECTORY,&path) < B_OK)
		return;

	path.Append(kDataTranslationsSettingsFile);

	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE);
	if (file.InitCheck() == B_OK) {
		file.Seek(-8,SEEK_END); // Skip over the first 497 bytes to just the window
								// position.
		fCorner.PrintToStream();
		file.Write(&fCorner, sizeof(BPoint));
	}
}

void
DataTranslationsSettings::SetWindowCorner(BPoint corner)
{
	fCorner=corner;
}