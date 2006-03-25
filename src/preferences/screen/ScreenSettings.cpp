/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Rafael Romo
 *		Stefano Ceccherini (burton666@libero.it)
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <StorageKit.h>
#include <Screen.h>

#include "ScreenSettings.h"


static const char* kSettingsFileName = "Screen_data";


ScreenSettings::ScreenSettings()
{
	BScreen screen(B_MAIN_SCREEN_ID);
	BRect screenFrame = screen.Frame();

	fWindowFrame.Set(0, 0, 356, 202);

	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append(kSettingsFileName);

		BFile file(path.Path(), B_READ_ONLY);
		if (file.InitCheck() == B_OK) {
			BPoint point;
			file.Read(&point, sizeof(BPoint));
			fWindowFrame.OffsetTo(point);

			// make sure the window is visible on screen
			if (screenFrame.right >= fWindowFrame.left + 40
				&& screenFrame.bottom >= fWindowFrame.top + 40
				&& screenFrame.left <= fWindowFrame.right - 40
				&& screenFrame.top <= fWindowFrame.bottom - 40)
				return;
		}
	}

	fWindowFrame.OffsetTo(screenFrame.left + (screenFrame.Width() - fWindowFrame.Width()) / 2,
		screenFrame.top + (screenFrame.Height() - fWindowFrame.Height()) /2);
}


ScreenSettings::~ScreenSettings()
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) < B_OK)
		return;

	path.Append(kSettingsFileName);

	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE);
	if (file.InitCheck() == B_OK) {
		BPoint point(fWindowFrame.LeftTop());
		file.Write(&point, sizeof(BPoint));
	}
}


void
ScreenSettings::SetWindowFrame(BRect frame)
{
	fWindowFrame = frame;
}
