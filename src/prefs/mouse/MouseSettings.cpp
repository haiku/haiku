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

#include <Application.h>
#include <FindDirectory.h>
#include <File.h>
#include <Path.h>
#include <String.h>
#include <stdio.h>

#include "MouseSettings.h"
#include "MouseMessages.h"


// The R5 settings file differs from that of OpenBeOS;
// the latter maps 16 different mouse buttons
#define R5_COMPATIBLE 1

static const char kMouseSettingsFile[] = "Mouse_settings";


MouseSettings::MouseSettings()
{
	RetrieveSettings();
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
	get_mouse_acceleration(&fSettings.accel.accel_factor);
	get_mouse_type(&fSettings.type);

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
}


void
MouseSettings::SetWindowPosition(BPoint corner)
{
	fWindowPosition = corner;
}


void
MouseSettings::SetMouseType(int32 type)
{
	fSettings.type = type;
}


bigtime_t 
MouseSettings::ClickSpeed() const
{
	return -(fSettings.click_speed - 1000000);
		// -1000000 to correct the Sliders 0-100000 scale
}


void
MouseSettings::SetClickSpeed(bigtime_t click_speed)
{
	fSettings.click_speed = -(click_speed-1000000);
}


void
MouseSettings::SetMouseSpeed(int32 accel)
{
	fSettings.accel.speed = accel;
}
