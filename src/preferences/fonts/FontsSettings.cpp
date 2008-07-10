/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mark Hogben
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <Application.h>
#include <FindDirectory.h>
#include <File.h>
#include <Path.h>
#include <Font.h>
#include <String.h>
#include <stdio.h>
#include <Message.h>

#include "FontsSettings.h"

static const char kSettingsFile[] = "Font_Settings";


FontsSettings::FontsSettings()
{
	BPath path;
	BMessage msg;

	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append(kSettingsFile);
		BFile file(path.Path(), B_READ_ONLY);

		if (file.InitCheck() != B_OK)
			SetDefaults();
		else if (msg.Unflatten(&file) != B_OK)
			SetDefaults();
		else
			msg.FindPoint("windowlocation", &fCorner);
	}
}


FontsSettings::~FontsSettings()
{
	BPath path;
	BMessage msg;

	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) < B_OK)
		return;

	path.Append(kSettingsFile);
	BFile file(path.Path(), B_WRITE_ONLY|B_CREATE_FILE);

	if (file.InitCheck() == B_OK) {
		msg.AddPoint("windowlocation", fCorner);
		msg.Flatten(&file);
	}
}


void
FontsSettings::SetWindowCorner(BPoint where)
{
	fCorner = where;
}


void
FontsSettings::SetDefaults()
{
	fCorner.Set(-1, -1);
}

