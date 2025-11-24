/*
 * Copyright 2019-2025, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Preetpal Kaur <preetpalok123@gmail.com>
 *		Pawan Yerramilli <me@pawanyerramilli.com>
 *		Samuel Rodríguez Pérez <samuelrp84@gmail.com>
 */


#include "InputTouchpadPref.h"
#include "InterfaceDefs.h"

#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <List.h>
#include <String.h>

#include <InputServerDevice.h>


TouchpadPref::TouchpadPref(BInputDevice* device)
	:
	fTouchPad(device)
{
	// default center position
	fWindowPosition.x = -1;
	fWindowPosition.y = -1;

	if (LoadSettings() != B_OK)
		Defaults();

	fStartSettings = fSettings;
}


TouchpadPref::~TouchpadPref()
{
	delete fTouchPad;

	SaveSettings();
}


void
TouchpadPref::Revert()
{
	fSettings = fStartSettings;
	set_mouse_speed(fTouchPad->Name(), fSettings.trackpad_speed);
	set_mouse_acceleration(fTouchPad->Name(), fSettings.trackpad_acceleration);
}


BMessage
TouchpadPref::BuildSettingsMessage()
{
	BMessage msg;
	msg.AddBool("scroll_reverse", fSettings.scroll_reverse);
	msg.AddBool("scroll_twofinger", fSettings.scroll_twofinger);
	msg.AddBool(
		"scroll_twofinger_horizontal", fSettings.scroll_twofinger_horizontal);
	msg.AddFloat("scroll_rightrange", fSettings.scroll_rightrange);
	msg.AddFloat("scroll_bottomrange", fSettings.scroll_bottomrange);
	msg.AddInt16("scroll_xstepsize", fSettings.scroll_xstepsize);
	msg.AddInt16("scroll_ystepsize", fSettings.scroll_ystepsize);
	msg.AddInt8("scroll_acceleration", fSettings.scroll_acceleration);
	msg.AddInt8("tapgesture_sensibility", fSettings.tapgesture_sensibility);
	msg.AddInt16("padblocker_threshold", fSettings.padblocker_threshold);
	msg.AddInt32("trackpad_speed", fSettings.trackpad_speed);
	msg.AddInt32("trackpad_acceleration", fSettings.trackpad_acceleration);
	msg.AddBool("scroll_twofinger_natural_scrolling", fSettings.scroll_twofinger_natural_scrolling);
	msg.AddInt8("edge_motion", fSettings.edge_motion);

	return msg;
}


status_t
TouchpadPref::UpdateRunningSettings()
{
	BMessage msg = BuildSettingsMessage();
	return fTouchPad->Control(B_SET_TOUCHPAD_SETTINGS, &msg);
}

void
TouchpadPref::Defaults()
{
	fSettings = kDefaultTouchpadSettings;
	set_mouse_speed(fTouchPad->Name(), fSettings.trackpad_speed);
	set_mouse_acceleration(fTouchPad->Name(), fSettings.trackpad_acceleration);
}


status_t
TouchpadPref::GetSettingsPath(BPath& path)
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

	BMessage settingsMsg;
	status = settingsMsg.Unflatten(&settingsFile);
	// Load an old settings file if we have that instead; remove after Beta 6?
	if (status != B_OK) {
		off_t size;
		settingsFile.Seek(0, SEEK_SET);
		// Old settings were 20 bytes, BPoint added 8
		if (settingsFile.GetSize(&size) == B_OK && size == 28) {
			if (settingsFile.Read(&fSettings, 20) != 20) {
				LOG("failed to load old settings\n");
				return B_ERROR;
			} else {
				fSettings.scroll_reverse = kDefaultTouchpadSettings.scroll_reverse;
				fSettings.trackpad_speed = kDefaultTouchpadSettings.trackpad_speed;
				fSettings.trackpad_acceleration = kDefaultTouchpadSettings.trackpad_acceleration;
				return B_OK;
			}
		} else {
			LOG("failed to load settings\n");
			return B_ERROR;
		}
	}

	settingsMsg.FindBool("scroll_reverse", &fSettings.scroll_reverse);
	settingsMsg.FindBool("scroll_twofinger", &fSettings.scroll_twofinger);
	settingsMsg.FindBool("scroll_twofinger_horizontal", &fSettings.scroll_twofinger_horizontal);
	settingsMsg.FindFloat("scroll_rightrange", &fSettings.scroll_rightrange);
	settingsMsg.FindFloat("scroll_bottomrange", &fSettings.scroll_bottomrange);
	settingsMsg.FindInt16("scroll_xstepsize", (int16*)&fSettings.scroll_xstepsize);
	settingsMsg.FindInt16("scroll_ystepsize", (int16*)&fSettings.scroll_ystepsize);
	settingsMsg.FindInt8("scroll_acceleration", (int8*)&fSettings.scroll_acceleration);
	settingsMsg.FindInt8("tapgesture_sensibility", (int8*)&fSettings.tapgesture_sensibility);
	settingsMsg.FindInt16("padblocker_threshold", (int16*)&fSettings.padblocker_threshold);
	settingsMsg.FindInt32("trackpad_speed", &fSettings.trackpad_speed);
	settingsMsg.FindInt32("trackpad_acceleration", &fSettings.trackpad_acceleration);
	settingsMsg.FindPoint("window_position", &fWindowPosition);

	fSettings.scroll_twofinger_natural_scrolling = settingsMsg.GetBool(
		"scroll_twofinger_natural_scrolling",
		kDefaultTouchpadSettings.scroll_twofinger_natural_scrolling);
	fSettings.edge_motion = settingsMsg.GetInt8(
		"edge_motion",
		kDefaultTouchpadSettings.edge_motion);

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

	BMessage settingsMsg = BuildSettingsMessage();
	settingsMsg.AddPoint("window_position", fWindowPosition);

	status = settingsMsg.Flatten(&settingsFile);
	if (status != B_OK) {
		LOG("can't save settings\n");
		return status;
	}

	return B_OK;
}

void
TouchpadPref::SetSpeed(int32 speed)
{
	int32 value = (int32)pow(2, speed * 6.0 / 1000) * 8192;
		// slow = 8192, fast = 524287; taken from InputMouse.cpp
	if (set_mouse_speed(fTouchPad->Name(), value) == B_OK) {
		fSettings.trackpad_speed = value;
		UpdateRunningSettings();
	}
}

void
TouchpadPref::SetAcceleration(int32 accel)
{
	int32 value = (int32)pow(accel * 4.0 / 1000, 2) * 16384;
		// slow = 0, fast = 262144; taken from InputMouse.cpp
	if (set_mouse_acceleration(fTouchPad->Name(), value) == B_OK) {
		fSettings.trackpad_acceleration = value;
		UpdateRunningSettings();
	}
}
