// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:        MouseSettings.h
//  Author:      Jérôme Duval, Andrew McCall (mccall@digitalparadise.co.uk)
//  Description: Media Preferences
//  Created :   December 10, 2003
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

const char kMouseSettingsFile[] = "Mouse_settings";

MouseSettings::MouseSettings()
{
	fInitCheck = B_OK;
	BPath path;

	if (find_directory(B_USER_SETTINGS_DIRECTORY,&path) == B_OK) {
		path.Append(kMouseSettingsFile);
		BFile file(path.Path(), B_READ_ONLY);
		if (file.InitCheck() == B_OK) {
			// Now read in the data
			// we have to do this because mouse_map counts 16 buttons in OBOS
			if ((file.Read(&fSettings.type, sizeof(fSettings.type)) != sizeof(fSettings.type))
				||(file.Read(&fSettings.map, (3*sizeof(int32))) != (3*sizeof(int32)))
				|| (file.Read(&fSettings.accel, sizeof(fSettings.accel)) != sizeof(fSettings.accel))
				|| (file.Read(&fSettings.click_speed, sizeof(fSettings.click_speed)) != sizeof(fSettings.click_speed))) {
				if (get_mouse_type((int32*)&fSettings.type) != B_OK)
					fInitCheck = B_ERROR;
				if (get_mouse_map(&fSettings.map) != B_OK)
					fInitCheck = B_ERROR;
				if (get_mouse_speed(&fSettings.accel.speed) != B_OK)
					fInitCheck = B_ERROR;
				if (get_click_speed(&fSettings.click_speed) != B_OK)
					fInitCheck = B_ERROR;
			}

			if (file.Read(&fCorner, sizeof(BPoint)) != sizeof(BPoint)) {
					fCorner.x=50;
					fCorner.y=50;
			}
		} else {
			if (get_mouse_type((int32*)&fSettings.type) != B_OK)
				fInitCheck = B_ERROR;
			if (get_mouse_map(&fSettings.map) != B_OK)
				fInitCheck = B_ERROR;
			if (get_mouse_speed(&fSettings.accel.speed) != B_OK)
					fInitCheck = B_ERROR;
			if (get_click_speed(&fSettings.click_speed) != B_OK)
				fInitCheck = B_ERROR;
			fCorner.x=50;
			fCorner.y=50;
		}
	} else
		fInitCheck = B_ERROR;	
}

MouseSettings::~MouseSettings()
{	
	BPath path;

	if (find_directory(B_USER_SETTINGS_DIRECTORY,&path) < B_OK)
		return;

	path.Append(kMouseSettingsFile);

	BFile file(path.Path(), B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	if (file.InitCheck() == B_OK) {
		// we have to do this because mouse_map counts 16 buttons in OBOS
		file.Write(&fSettings.type, sizeof(fSettings.type));
		file.Write(&fSettings.map, 3*sizeof(int32));
		file.Write(&fSettings.accel, sizeof(fSettings.accel));
		file.Write(&fSettings.click_speed, sizeof(fSettings.click_speed));
		file.Write(&fCorner, sizeof(BPoint));
	}
}


status_t
MouseSettings::InitCheck()
{
	return fInitCheck;
}


void
MouseSettings::SetWindowCorner(BPoint corner)
{
	fCorner = corner;
}


void
MouseSettings::SetMouseType(int32 type)
{
	fSettings.type = type;
}


void
MouseSettings::SetClickSpeed(bigtime_t click_speed)
{
	fSettings.click_speed = -(click_speed-1000000);
}


void
MouseSettings::SetMouseSpeed(int32 accel)
{
	fSettings.accel.speed=accel;
}
