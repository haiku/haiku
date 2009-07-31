/*
 * Copyright 2001-2009, Haiku.
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

	fWindowFrame.Set(0, 0, 450, 250);

	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append(kSettingsFileName);

		BFile file(path.Path(), B_READ_ONLY);
		if (file.InitCheck() == B_OK) {
			file.Read(&fWindowFrame, sizeof(BRect));

			// make sure the window is visible on screen
			if (fWindowFrame.Width() > screenFrame.Width())
				fWindowFrame.right = fWindowFrame.left + 450;
			if (fWindowFrame.Height() > screenFrame.Height())
				fWindowFrame.bottom = fWindowFrame.top + 250;

			if (screenFrame.right >= fWindowFrame.left + 40
				&& screenFrame.bottom >= fWindowFrame.top + 40
				&& screenFrame.left <= fWindowFrame.right - 40
				&& screenFrame.top <= fWindowFrame.bottom - 40)
				return;
		}
	}

	// Center on screen
	fWindowFrame.OffsetTo(
		screenFrame.left + (screenFrame.Width() - fWindowFrame.Width()) / 2,
		screenFrame.top + (screenFrame.Height() - fWindowFrame.Height()) /2);
}


ScreenSettings::~ScreenSettings()
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) < B_OK)
		return;

	path.Append(kSettingsFileName);

	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE);
	if (file.InitCheck() == B_OK)
		file.Write(&fWindowFrame, sizeof(BRect));
}


void
ScreenSettings::SetWindowFrame(BRect frame)
{
	fWindowFrame = frame;
}
