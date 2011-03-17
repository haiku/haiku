/*
 * Copyright 2002-2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Oliver Siebenmarck
 *		Axel Dörfler
 */


#include "DataTranslationsSettings.h"

#include <stdio.h>

#include <Application.h>
#include <File.h>
#include <FindDirectory.h>
#include <Message.h>
#include <Path.h>


static DataTranslationsSettings sDataTranslationsSettings;


DataTranslationsSettings::DataTranslationsSettings()
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return;

	fCorner = BPoint(-1, -1);

	path.Append("system/DataTranslations settings");
	BFile file(path.Path(), B_READ_ONLY);
	BMessage settings;

	if (file.InitCheck() == B_OK
		&& settings.Unflatten(&file) == B_OK) {
		BPoint corner;
		if (settings.FindPoint("window corner", &corner) == B_OK)
			fCorner = corner;
	}
}


DataTranslationsSettings::~DataTranslationsSettings()
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) < B_OK)
		return;

	BMessage settings;
	settings.AddPoint("window corner", fCorner);

	path.Append("system/DataTranslations settings");
	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (file.InitCheck() == B_OK)
		settings.Flatten(&file);
}


void
DataTranslationsSettings::SetWindowCorner(BPoint corner)
{
	fCorner = corner;
}


DataTranslationsSettings*
DataTranslationsSettings::Instance()
{
	return &sDataTranslationsSettings;
}
