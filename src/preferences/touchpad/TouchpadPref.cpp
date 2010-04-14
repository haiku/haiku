/*
 * Copyright 2008-2010, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler (haiku@Clemens-Zeidler.de)
 */


#include "TouchpadPref.h"

#include <List.h>
#include <FindDirectory.h>
#include <File.h>
#include <String.h>

#include <keyboard_mouse_driver.h>


TouchpadPref::TouchpadPref()
{
	fConnected = false;
	// default center position
	fWindowPosition.x = -1;
	fWindowPosition.y = -1;

	ConnectToTouchPad();

	if (LoadSettings() != B_OK)
		Defaults();

	fStartSettings = fSettings;
}


TouchpadPref::~TouchpadPref()
{
	if (fConnected)
		delete fTouchPad;

	SaveSettings();
}


void
TouchpadPref::Revert()
{
	fSettings = fStartSettings;
}


status_t
TouchpadPref::UpdateSettings()
{
	if (!fConnected)
		return B_ERROR;

	LOG("UpdateSettings of device %s\n", fTouchPad->Name());

	BMessage msg;
	msg.AddBool("scroll_twofinger", fSettings.scroll_twofinger);
	msg.AddBool("scroll_twofinger_horizontal",
		fSettings.scroll_twofinger_horizontal);
	msg.AddFloat("scroll_rightrange", fSettings.scroll_rightrange);
	msg.AddFloat("scroll_bottomrange", fSettings.scroll_bottomrange);
	msg.AddInt16("scroll_xstepsize", fSettings.scroll_xstepsize);
	msg.AddInt16("scroll_ystepsize", fSettings.scroll_ystepsize);
	msg.AddInt8("scroll_acceleration", fSettings.scroll_acceleration);
	msg.AddInt8("tapgesture_sensibility", fSettings.tapgesture_sensibility);

	return fTouchPad->Control(MS_SET_TOUCHPAD_SETTINGS, &msg);
}


void
TouchpadPref::Defaults()
{
	fSettings = kDefaultTouchpadSettings;
}


status_t
TouchpadPref::GetSettingsPath(BPath &path)
{
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status < B_OK)
		return status;

	return path.Append(TOUCHPAD_SETTINGS_FILE);
}


status_t
TouchpadPref::LoadSettings()
{
	BPath path;
	status_t status = GetSettingsPath(path);
	if (status != B_OK)
		return status;

	BFile settingsFile(path.Path(), B_READ_ONLY);
	status = settingsFile.InitCheck();
	if (status != B_OK)
		return status;

	if (settingsFile.Read(&fSettings, sizeof(touchpad_settings))
			!= sizeof(touchpad_settings)) {
		LOG("failed to load settings\n");
		return B_ERROR;
	}

	if (settingsFile.Read(&fWindowPosition, sizeof(BPoint))
			!= sizeof(BPoint)) {
		LOG("failed to load settings\n");
		return B_ERROR;
	}

	return B_OK;
}


status_t
TouchpadPref::SaveSettings()
{
	BPath path;
	status_t status = GetSettingsPath(path);
	if (status != B_OK)
		return status;

	BFile settingsFile(path.Path(), B_READ_WRITE | B_CREATE_FILE);
	status = settingsFile.InitCheck();
	if (status != B_OK)
		return status;

	if (settingsFile.Write(&fSettings, sizeof(touchpad_settings))
			!= sizeof(touchpad_settings)) {
		LOG("can't save settings\n");
		return B_ERROR;
	}

	if (settingsFile.Write(&fWindowPosition, sizeof(BPoint))
			!= sizeof(BPoint)) {
		LOG("can't save window position\n");
		return B_ERROR;
	}

	return B_OK;
}


status_t
TouchpadPref::ConnectToTouchPad()
{
	BList devList;
	status_t status = get_input_devices(&devList);
	if (status != B_OK)
		return status;

	int32 i = 0;
	while (true) {
		BInputDevice* dev = (BInputDevice*)devList.ItemAt(i);
		if (dev == NULL)
			break;
		i++;

		LOG("input device %s\n", dev->Name());

		BString name = dev->Name();

		if (name.FindFirst("Touchpad") >= 0
			&& dev->Type() == B_POINTING_DEVICE
			&& !fConnected) {
			fConnected = true;
			fTouchPad = dev;
			// Don't bail out here, since we need to delete the other devices
			// yet.
		} else {
			delete dev;
		}
	}
	if (fConnected)
		return B_OK;

	LOG("touchpad input device NOT found\n");
	return B_ENTRY_NOT_FOUND;
}
