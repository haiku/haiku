/*
 * Copyright 2002-2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew McCall, mccall@digitalparadise.co.uk
 *		Mike Berg <mike@berg-net.us>
 *		Julun <host.haiku@gmx.de>
 *
 */

#include "TimeSettings.h"
#include "TimeMessages.h"


#include <File.h>
#include <FindDirectory.h>
#include <Path.h>


TimeSettings::TimeSettings()
	:
	fSettingsFile("Time_preflet_window")
{
}


TimeSettings::~TimeSettings()
{
}


BPoint
TimeSettings::LeftTop() const
{
	BPath path;
	BPoint leftTop(-1000.0, -1000.0);

	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append(fSettingsFile.String());

		BFile file(path.Path(), B_READ_ONLY);
		if (file.InitCheck() == B_OK) {
			BPoint tmp;
			if (file.Read(&tmp, sizeof(BPoint)) == sizeof(BPoint))
				leftTop = tmp;
		}
	}

	return leftTop;
}


void
TimeSettings::SetLeftTop(const BPoint leftTop)
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return;

	path.Append(fSettingsFile.String());

	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE);
	if (file.InitCheck() == B_OK)
		file.Write(&leftTop, sizeof(BPoint));
}

