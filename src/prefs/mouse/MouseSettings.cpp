// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:			MouseSettings.h
//  Authors:		Jérôme Duval,
//					Andrew McCall (mccall@digitalparadise.co.uk),
//					Axel Dörfler (axeld@pinc-software.de)
//  Description:	Mouse Preferences
//  Created:		December 10, 2003
//
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#include <FindDirectory.h>
#include <File.h>
#include <Path.h>
#include <View.h>

#include <stdio.h>

#include "MouseSettings.h"


// The R5 settings file differs from that of OpenBeOS;
// the latter maps 16 different mouse buttons
#define R5_COMPATIBLE 1

static const char kMouseSettingsFile[] = "Mouse_settings";

static const bigtime_t kDefaultClickSpeed = 500000;
static const int32 kDefaultMouseSpeed = 65536;
static const int32 kDefaultMouseType = 3;	// 3 button mouse
static const int32 kDefaultAccelerationFactor = 65536;


MouseSettings::MouseSettings()
{
	RetrieveSettings();

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

	path.Append(kMouseSettingsFile);
	return B_OK;
}


void 
MouseSettings::RetrieveSettings()
{
	// retrieve current values

	get_mouse_map(&fSettings.map);
	get_click_speed(&fSettings.click_speed);
	get_mouse_speed(&fSettings.accel.speed);
	get_mouse_acceleration(&fSettings.accel.accel_factor);
	get_mouse_type(&fSettings.type);

	fMode = mouse_mode();

	// also try to load the window position from disk

	BPath path;
	if (GetSettingsPath(path) < B_OK)
		return;

	BFile file(path.Path(), B_READ_ONLY);
	if (file.InitCheck() < B_OK)
		return;

#if R5_COMPATIBLE
	const off_t kOffset = sizeof(mouse_settings) - sizeof(mouse_map) + sizeof(int32) * 3;
		// we have to do this because mouse_map counts 16 buttons in OBOS
#else
	const off_t kOffset = sizeof(mouse_settings);
#endif

	if (file.ReadAt(kOffset, &fWindowPosition, sizeof(BPoint)) != sizeof(BPoint)) {
		// set default window position (invalid positions will be
		// corrected by the application; the window will be centered
		// in this case)
		fWindowPosition.x = -1;
		fWindowPosition.y = -1;
	}
	
#ifdef DEBUG
	Dump();
#endif
}


status_t 
MouseSettings::SaveSettings()
{
	BPath path;
	status_t status = GetSettingsPath(path);
	if (status < B_OK)
		return status;

	BFile file(path.Path(), B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	if (status < B_OK)
		return status;

	// we have to do this because mouse_map counts 16 buttons in OBOS
#if R5_COMPATIBLE
	file.Write(&fSettings.type, sizeof(fSettings.type));
	file.Write(&fSettings.map, 3*sizeof(int32));
	file.Write(&fSettings.accel, sizeof(fSettings.accel));
	file.Write(&fSettings.click_speed, sizeof(fSettings.click_speed));
#else
	file.Write(fSettings, sizeof(fSettings));
#endif

	file.Write(&fWindowPosition, sizeof(BPoint));

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

	mouse_map map;
	if (get_mouse_map(&map) == B_OK) {
		map.button[0] = B_PRIMARY_MOUSE_BUTTON;
		map.button[1] = B_SECONDARY_MOUSE_BUTTON;
		map.button[2] = B_TERTIARY_MOUSE_BUTTON;
		SetMapping(map);
	}
}


/**	Reverts to the active settings at program startup
 */

void 
MouseSettings::Revert()
{
	set_mouse_type(fOriginalSettings.type);
	set_click_speed(fOriginalSettings.click_speed);
	set_mouse_speed(fOriginalSettings.accel.speed);
	set_mouse_acceleration(fOriginalSettings.accel.accel_factor);
	set_mouse_map(&fOriginalSettings.map);

	set_mouse_mode(fOriginalMode);
}


void
MouseSettings::SetWindowPosition(BPoint corner)
{
	fWindowPosition = corner;
}


void
MouseSettings::SetMouseType(int32 type)
{
	if (set_mouse_type(type) == B_OK)
		fSettings.type = type;
}


bigtime_t 
MouseSettings::ClickSpeed() const
{
	return 1000000LL - fSettings.click_speed;
		// to correct the Sliders 0-100000 scale
}


void
MouseSettings::SetClickSpeed(bigtime_t clickSpeed)
{
	clickSpeed = 1000000LL - clickSpeed;

	if (set_click_speed(clickSpeed) == B_OK)
		fSettings.click_speed = clickSpeed;
}


void
MouseSettings::SetMouseSpeed(int32 speed)
{
	if (set_mouse_speed(speed) == B_OK)
		fSettings.accel.speed = speed;
}


void
MouseSettings::SetAccelerationFactor(int32 factor)
{
	if (set_mouse_acceleration(factor) == B_OK)
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
	if (set_mouse_map(&fSettings.map) != B_OK)	// message better for syslog?
		fprintf(stderr, "could not set mouse mapping\n");
}


void
MouseSettings::SetMapping(mouse_map &map)
{
	if (set_mouse_map(&map) == B_OK)
		fSettings.map = map;
}


void 
MouseSettings::SetMouseMode(mode_mouse mode)
{
	set_mouse_mode(mode);
	fMode = mode;
}

