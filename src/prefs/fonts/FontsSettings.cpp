/*
	FontsSettings.cpp
*/

#include <Application.h>
#include <FindDirectory.h>
#include <File.h>
#include <Path.h>
#include <String.h>
#include <stdio.h>

#include "FontsSettings.h"

const char FontsSettings::kSettingsFile[] = "Font_Settings";
const BPoint kDEFAULT = BPoint(100, 100);

FontsSettings::FontsSettings()
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append(kSettingsFile);
		BFile file(path.Path(), B_READ_ONLY);
		if (file.InitCheck() == B_OK) {
			float x, y;
			if (file.Read(&x, sizeof(float)) != sizeof(float)) {
				x = kDEFAULT.x;
			}
			if (file.Read(&y, sizeof(float)) != sizeof(float)) {
				y = kDEFAULT.y;
			}
			f_corner = BPoint(x, y);
		} else {
			f_corner = kDEFAULT;
		}
	}
}	
	
FontsSettings::~FontsSettings()
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) < B_OK) 
		return;
	path.Append(kSettingsFile);
	BFile file(path.Path(), B_WRITE_ONLY|B_CREATE_FILE);
	if (file.InitCheck() == B_OK) {
		float x = f_corner.x;
		float y = f_corner.y;
		file.Write(&x, sizeof(float));
		file.Write(&y, sizeof(float));
	}
}


void
FontsSettings::SetWindowCorner(BPoint where)
{
	f_corner = where;
}
