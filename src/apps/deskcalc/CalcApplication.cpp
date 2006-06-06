/*
 * Copyright 2006 Haiku, Inc. All Rights Reserved.
 * Copyright 1997, 1998 R3 Software Ltd. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Timothy Wayper <timmy@wunderbear.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "CalcApplication.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>

#include "CalcWindow.h"


static const char* kSettingsFileName	= "DeskCalc_settings";
extern const char* kAppSig				= "application/x-vnd.Haiku-DeskCalc";

static const float kDefaultWindowWidth	= 220.0;
static const float kDefaultWindowHeight	= 140.0;


CalcApplication::CalcApplication()
	: BApplication(kAppSig),
	  fCalcWindow(NULL)
{
}


CalcApplication::~CalcApplication()
{
}


void
CalcApplication::ReadyToRun()
{
	BRect frame(0, 0, kDefaultWindowWidth - 1, kDefaultWindowHeight - 1);
	fCalcWindow = new CalcWindow(frame);

	if (!_LoadSettings())
		fCalcWindow->SetFrame(frame, true);

	// reveal window
	fCalcWindow->Show();
}


void
CalcApplication::AboutRequested()
{
	// TODO: implement me!
	return BApplication::AboutRequested();
}


bool
CalcApplication::QuitRequested()
{
	// save current user preferences
	_SaveSettings();

	return true;
}


// #pragma mark -


bool
CalcApplication::_LoadSettings()
{
	// locate preferences file
	BFile prefsFile;
	if (_InitSettingsFile(&prefsFile, false) < B_OK) {
		printf("no preference file found.\n");
		return false;
	}

	// unflatten settings data
	BMessage archive;
	if (archive.Unflatten(&prefsFile) < B_OK) {
		printf("error unflattening settings.\n");
		return false;
	}

	// apply settings
	return fCalcWindow->LoadSettings(&archive);
}


void
CalcApplication::_SaveSettings()
{
	if (!fCalcWindow->Lock())
		return;

	// archive the current state of the calculator
	BMessage archive;
	status_t ret = fCalcWindow->SaveSettings(&archive);

	fCalcWindow->Unlock();

	if (ret < B_OK) {
		fprintf(stderr, "CalcApplication::_SaveSettings() - "
						"error from window: %s\n", strerror(ret));
		return;
	}

	// flatten entire acrhive and write to settings file
	BFile prefsFile;
	ret = _InitSettingsFile(&prefsFile, true);
	if (ret < B_OK) {
		fprintf(stderr, "CalcApplication::_SaveSettings() - "
						"error creating file: %s\n", strerror(ret));
		return;
	}

	ret = archive.Flatten(&prefsFile);
	if (ret < B_OK) {
		fprintf(stderr, "CalcApplication::_SaveSettings() - "
						"error flattening to file: %s\n", strerror(ret));
		return;
	}
}

status_t
CalcApplication::_InitSettingsFile(BFile* file, bool write)
{
	// find user settings directory	
	BPath prefsPath;
	status_t ret = find_directory(B_USER_SETTINGS_DIRECTORY, &prefsPath);
	if (ret < B_OK)
		return ret;

	ret = prefsPath.Append(kSettingsFileName);
	if (ret < B_OK)
		return ret;

	if (write)
		ret = file->SetTo(prefsPath.Path(), B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY);
	else
		ret = file->SetTo(prefsPath.Path(), B_READ_ONLY);

	return ret;
}


