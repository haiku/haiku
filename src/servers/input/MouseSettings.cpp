// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2004, Haiku
//
//  This software is part of the Haiku distribution and is covered 
//  by the Haiku license.
//
//
//  File:			MouseSettings.h
//  Authors:		Jérôme Duval,
//					Andrew McCall (mccall@digitalparadise.co.uk),
//					Axel Dörfler (axeld@pinc-software.de)
//  Description:	Input Server
//  Created:		August 29, 2004
//
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#include <FindDirectory.h>
#include <File.h>
#include <Path.h>
#include <View.h>

#include <stdio.h>

#include "MouseSettings.h"

static const bigtime_t kDefaultClickSpeed = 500000;
static const int32 kDefaultMouseSpeed = 65536;
static const int32 kDefaultMouseType = 3;	// 3 button mouse
static const int32 kDefaultAccelerationFactor = 65536;


MouseSettings::MouseSettings()
{
	RetrieveSettings();

#ifdef DEBUG
        Dump();
#endif

	fOriginalSettings = fSettings;
	fOriginalMode = fMode;
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
	Defaults();
	
	// also try to load the window position from disk

	BPath path;
	if (GetSettingsPath(path) < B_OK)
		return;

	BFile file(path.Path(), B_READ_ONLY);
	if (file.InitCheck() < B_OK)
		return;

#ifdef COMPILE_FOR_R5
	// we have to do this because mouse_map counts 16 buttons in OBOS
	
	if ((file.Read(&fSettings.type, sizeof(int32)) != sizeof(int32))
		|| (file.Read(&fSettings.map, sizeof(int32) * 3) != sizeof(int32) * 3)
		|| (file.Read(&fSettings.accel, sizeof(mouse_accel)) != sizeof(mouse_accel))
		|| (file.Read(&fSettings.click_speed, sizeof(bigtime_t)) != sizeof(bigtime_t))) {
		Defaults();
	}
#else
	if (file.ReadAt(0, &fSettings, sizeof(mouse_settings)) != sizeof(mouse_settings)) {
		Defaults();
	}
#endif

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
	if (status < B_OK)
		return status;

	// we have to do this because mouse_map counts 16 buttons in OBOS
#ifdef COMPILE_FOR_R5
	file.Write(&fSettings.type, sizeof(fSettings.type));
	file.Write(&fSettings.map, 3*sizeof(int32));
	file.Write(&fSettings.accel, sizeof(fSettings.accel));
	file.Write(&fSettings.click_speed, sizeof(fSettings.click_speed));
#else
	file.Write(&fSettings, sizeof(fSettings));
#endif

	// who is responsible for saving the mouse mode?

	return B_OK;
}


#ifdef DEBUG
void
MouseSettings::Dump()
{
	printf("type:\t\t%ld button mouse\n", fSettings.type);
	printf("map:\t\tleft = %lu : middle = %lu : right = %lu\n", fSettings.map.button[0], fSettings.map.button[2], fSettings.map.button[1]);
	printf("click speed:\t%Ld\n", fSettings.click_speed);
	printf("accel:\t\t%s\n", fSettings.accel.enabled ? "enabled" : "disabled");
	printf("accel factor:\t%ld\n", fSettings.accel.accel_factor);
	printf("speed:\t\t%ld\n", fSettings.accel.speed);

	char *mode = "unknown";
	switch (fMode) {
		case B_NORMAL_MOUSE:
			mode = "normal";
			break;
		case B_FOCUS_FOLLOWS_MOUSE:
			mode = "focus follows mouse";
			break;
		case B_WARP_MOUSE:
			mode = "warp mouse";
			break;
		case B_INSTANT_WARP_MOUSE:
			mode = "instant warp mouse";
			break;
	}
	printf("mode:\t\t%s\n", mode);
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

