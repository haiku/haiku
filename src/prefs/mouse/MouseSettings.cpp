/*
 * MouseSettings.cpp
 * Mouse mccall@digitalparadise.co.uk
 *
 */
 
#include <Application.h>
#include <FindDirectory.h>
#include <File.h>
#include <Path.h>
#include <String.h>
#include <stdio.h>

#include "MouseSettings.h"
#include "MouseMessages.h"

const char MouseSettings::kMouseSettingsFile[] = "Mouse_settings";

MouseSettings::MouseSettings()
{
	BPath path;

	if (find_directory(B_USER_SETTINGS_DIRECTORY,&path) == B_OK) {
		path.Append(kMouseSettingsFile);
		BFile file(path.Path(), B_READ_ONLY);
		if (file.InitCheck() == B_OK) {
			// Now read in the data
			if (file.Read(&fSettings, sizeof(mouse_settings)) != sizeof(mouse_settings)) {
				if (get_mouse_type((int32*)&fSettings.type) != B_OK)
					be_app->PostMessage(ERROR_DETECTED);
				if (get_mouse_map(&fSettings.map) != B_OK)
					be_app->PostMessage(ERROR_DETECTED);
				if (get_mouse_speed((int32 *)&fSettings.accel.speed) != B_OK)
					be_app->PostMessage(ERROR_DETECTED);
				if (get_click_speed(&fSettings.click_speed) != B_OK)
					be_app->PostMessage(ERROR_DETECTED);
			}

			if (file.Read(&fCorner, sizeof(BPoint)) != sizeof(BPoint)) {
					fCorner.x=50;
					fCorner.y=50;
				}
		}
		else {
			if (get_mouse_type((int32*)&fSettings.type) != B_OK)
				be_app->PostMessage(ERROR_DETECTED);
			if (get_mouse_map(&fSettings.map) != B_OK)
				be_app->PostMessage(ERROR_DETECTED);

			if (get_click_speed(&fSettings.click_speed) != B_OK)
				be_app->PostMessage(ERROR_DETECTED);
			fCorner.x=50;
			fCorner.y=50;
		}
	}
	else
		be_app->PostMessage(ERROR_DETECTED);
	
	printf("Size is : %ld\n",sizeof(mouse_settings)+sizeof(BPoint));
		
}

MouseSettings::~MouseSettings()
{	
	BPath path;

	if (find_directory(B_USER_SETTINGS_DIRECTORY,&path) < B_OK)
		return;

	path.Append(kMouseSettingsFile);

	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE);
	if (file.InitCheck() == B_OK) {
		file.Write(&fSettings, sizeof(mouse_settings));
		file.Write(&fCorner, sizeof(BPoint));
	}
}

void
MouseSettings::SetWindowCorner(BPoint corner)
{
	fCorner=corner;
}

void
MouseSettings::SetMouseType(mouse_type type)
{
	fSettings.type=type;
}

void
MouseSettings::SetClickSpeed(bigtime_t click_speed)
{
	fSettings.click_speed=-(click_speed-1000000);
}

void
MouseSettings::SetMouseSpeed(int32 accel)
{
	fSettings.accel.speed=accel;
}