/*
 * Copyright 2004-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 *		Andrew McCall (mccall@digitalparadise.co.uk)
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "MouseSettings.h"

#include <stdio.h>

#include <FindDirectory.h>
#include <File.h>
#include <Path.h>
#include <View.h>


static const bigtime_t kDefaultClickSpeed = 500000;
static const int32 kDefaultMouseSpeed = 65536;
static const int32 kDefaultMouseType = 3;	// 3 button mouse
static const int32 kDefaultAccelerationFactor = 65536;
static const bool kDefaultAcceptFirstClick = false;



MouseSettings::MouseSettings()
{
	Defaults();
	RetrieveSettings();

#ifdef DEBUG
        Dump();
#endif

	fOriginalSettings = fSettings;
	fOriginalMode = fMode;
	fOriginalFocusFollowsMouseMode = fFocusFollowsMouseMode;
	fOriginalAcceptFirstClick = fAcceptFirstClick;
}


MouseSettings::~MouseSettings()
{
	SaveSettings();
}


status_t
MouseSettings::GetSettingsPath(BPath &path)
{
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status < B_OK)
		return status;

	path.Append(mouse_settings_file);
	return B_OK;
}


void
MouseSettings::RetrieveSettings()
{
	// retrieve current values

	fMode = mouse_mode();
	fAcceptFirstClick = accept_first_click();
	Defaults();

	// also try to load the window position from disk

	BPath path;
	if (GetSettingsPath(path) < B_OK)
		return;

	BFile file(path.Path(), B_READ_ONLY);
	if (file.InitCheck() < B_OK)
		return;

	if (file.ReadAt(0, &fSettings, sizeof(mouse_settings))
			!= sizeof(mouse_settings)) {
		Defaults();
	}

	if ((fSettings.click_speed == 0)
		|| (fSettings.type == 0)) {
		Defaults();
	}
}


status_t
MouseSettings::SaveSettings()
{
#ifdef DEBUG
	Dump();
#endif

	BPath path;
	status_t status = GetSettingsPath(path);
	if (status < B_OK)
		return status;

	BFile file(path.Path(), B_READ_WRITE | B_CREATE_FILE);
	status = file.InitCheck();
	if (status != B_OK)
		return status;

	file.Write(&fSettings, sizeof(fSettings));

	// who is responsible for saving the mouse mode and accept_first_click?

	return B_OK;
}


#ifdef DEBUG
void
MouseSettings::Dump()
{
	printf("type:\t\t%ld button mouse\n", fSettings.type);
	printf("map:\t\tleft = %lu : middle = %lu : right = %lu\n",
		fSettings.map.button[0], fSettings.map.button[2],
		fSettings.map.button[1]);
	printf("click speed:\t%Ld\n", fSettings.click_speed);
	printf("accel:\t\t%s\n", fSettings.accel.enabled
		? "enabled" : "disabled");
	printf("accel factor:\t%ld\n", fSettings.accel.accel_factor);
	printf("speed:\t\t%ld\n", fSettings.accel.speed);

	char *mode = "unknown";
	switch (fMode) {
		case B_NORMAL_MOUSE:
			mode = "activate";
			break;
		case B_CLICK_TO_FOCUS_MOUSE:
			mode = "focus";
			break;
		case B_FOCUS_FOLLOWS_MOUSE:
			mode = "auto-focus";
			break;
	}
	printf("mouse mode:\t%s\n", mode);

	char *focus_follows_mouse_mode = "unknown";
	switch (fMode) {
		case B_NORMAL_FOCUS_FOLLOWS_MOUSE:
			focus_follows_mouse_mode = "normal";
			break;
		case B_WARP_FOCUS_FOLLOWS_MOUSE:
			focus_follows_mouse_mode = "warp";
			break;
		case B_INSTANT_WARP_FOCUS_FOLLOWS_MOUSE:
			focus_follows_mouse_mode = "instant warp";
			break;
	}
	printf("focus follows mouse mode:\t%s\n", focus_follows_mouse_mode);
	printf("accept first click:\t%s\n", fAcceptFirstClick
		? "enabled" : "disabled");
}
#endif


/**	Resets the settings to the system defaults
 */

void
MouseSettings::Defaults()
{
	SetClickSpeed(kDefaultClickSpeed);
	SetMouseSpeed(kDefaultMouseSpeed);
	SetMouseType(kDefaultMouseType);
	SetAccelerationFactor(kDefaultAccelerationFactor);
	SetMouseMode(B_NORMAL_MOUSE);
	SetFocusFollowsMouseMode(B_NORMAL_FOCUS_FOLLOWS_MOUSE);
	SetAcceptFirstClick(kDefaultAcceptFirstClick);

	fSettings.map.button[0] = B_PRIMARY_MOUSE_BUTTON;
	fSettings.map.button[1] = B_SECONDARY_MOUSE_BUTTON;
	fSettings.map.button[2] = B_TERTIARY_MOUSE_BUTTON;

}


void
MouseSettings::SetMouseType(int32 type)
{
	fSettings.type = type;
}


bigtime_t
MouseSettings::ClickSpeed() const
{
	return fSettings.click_speed;
}


void
MouseSettings::SetClickSpeed(bigtime_t clickSpeed)
{
	fSettings.click_speed = clickSpeed;
}


void
MouseSettings::SetMouseSpeed(int32 speed)
{
	fSettings.accel.speed = speed;
}


void
MouseSettings::SetAccelerationFactor(int32 factor)
{
	fSettings.accel.accel_factor = factor;
}


uint32
MouseSettings::Mapping(int32 index) const
{
	return fSettings.map.button[index];
}


void
MouseSettings::Mapping(mouse_map &map) const
{
	map = fSettings.map;
}


void
MouseSettings::SetMapping(int32 index, uint32 button)
{
	fSettings.map.button[index] = button;
}


void
MouseSettings::SetMapping(mouse_map &map)
{
	fSettings.map = map;
}


void
MouseSettings::SetMouseMode(mode_mouse mode)
{
	fMode = mode;
}


void
MouseSettings::SetFocusFollowsMouseMode(mode_focus_follows_mouse mode)
{
	fFocusFollowsMouseMode = mode;
}


void
MouseSettings::SetAcceptFirstClick(bool acceptFirstClick)
{
	fAcceptFirstClick = acceptFirstClick;
}


