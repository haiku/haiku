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


MouseSettings::MouseSettings()
{
	Defaults();

#ifdef DEBUG
	Dump();
#endif
}


MouseSettings::MouseSettings(const mouse_settings* originalSettings)
{
	Defaults();

	if (originalSettings != NULL) {
		fMode = mouse_mode();
		fFocusFollowsMouseMode = focus_follows_mouse_mode();
		fAcceptFirstClick = accept_first_click();

		fSettings = *originalSettings;

		if (MouseType() < 1 || MouseType() > B_MAX_MOUSE_BUTTONS)
			SetMouseType(kDefaultMouseType);
		_AssureValidMapping();
	}

#ifdef DEBUG
	Dump();
#endif
}


MouseSettings::~MouseSettings()
{
}


#ifdef DEBUG
void
MouseSettings::Dump()
{
	printf("type:\t\t%" B_PRId32 " button mouse\n", fSettings.type);
	printf("map:\t\tleft = %" B_PRIu32 " : middle = %" B_PRIu32 " : "
		"right = %" B_PRIu32 "\n",
		fSettings.map.button[0], fSettings.map.button[2],
		fSettings.map.button[1]);
	printf("click speed:\t%" B_PRId64 "\n", fSettings.click_speed);
	printf("accel:\t\t%s\n", fSettings.accel.enabled
		? "enabled" : "disabled");
	printf("accel factor:\t%" B_PRId32 "\n", fSettings.accel.accel_factor);
	printf("speed:\t\t%" B_PRId32 "\n", fSettings.accel.speed);

	const char *mode = "unknown";
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

	const char *focus_follows_mouse_mode = "unknown";
	switch (fFocusFollowsMouseMode) {
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

	for (int i = 0; i < B_MAX_MOUSE_BUTTONS; i++)
		fSettings.map.button[i] = B_MOUSE_BUTTON(i + 1);
}


void
MouseSettings::SetMouseType(int32 type)
{
	if (type <= 0 || type > B_MAX_MOUSE_BUTTONS)
		return;

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
	if (index < 0 || index >= B_MAX_MOUSE_BUTTONS)
		return 0;

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
	if (index < 0 || index >= B_MAX_MOUSE_BUTTONS)
		return;

	fSettings.map.button[index] = button;
	_AssureValidMapping();
}


void
MouseSettings::SetMapping(mouse_map &map)
{
	fSettings.map = map;
	_AssureValidMapping();
}


void
MouseSettings::_AssureValidMapping()
{
	bool hasPrimary = false;

	for (int i = 0; i < MouseType(); i++) {
		if (fSettings.map.button[i] == 0)
			fSettings.map.button[i] = B_MOUSE_BUTTON(i + 1);
		hasPrimary |= fSettings.map.button[i] & B_MOUSE_BUTTON(1);
	}

	if (!hasPrimary)
		fSettings.map.button[0] = B_MOUSE_BUTTON(1);
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


/* MultiMouseSettings functions */

MultipleMouseSettings::MultipleMouseSettings()
{
	RetrieveSettings();

#ifdef DEBUG
	Dump();
#endif
}


MultipleMouseSettings::~MultipleMouseSettings()
{
	SaveSettings();

#ifdef DEBUG
	Dump();
#endif

	std::map<BString, MouseSettings*>::iterator itr;
	for (itr = fMouseSettingsObject.begin(); itr != fMouseSettingsObject.end(); ++itr)
		delete itr->second;
}


status_t
MultipleMouseSettings::GetSettingsPath(BPath &path)
{
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status < B_OK)
		return status;

	path.Append(mouse_settings_file);
	return B_OK;
}


void
MultipleMouseSettings::RetrieveSettings()
{
	BPath path;
	if (GetSettingsPath(path) < B_OK)
		return;

	BFile file(path.Path(), B_READ_ONLY);
	if (file.InitCheck() < B_OK)
		return;

	BMessage message;

	if (message.Unflatten(&file) == B_OK) {
		int i = 0;
		BString deviceName;
		mouse_settings* settings;
		ssize_t size = 0;

		while (message.FindString("mouseDevice", i, &deviceName) == B_OK) {
			message.FindData("mouseSettings", B_ANY_TYPE, i,
				(const void**)&settings, &size);
			MouseSettings* mouseSettings = new MouseSettings(settings);
			fMouseSettingsObject.insert(std::pair<BString, MouseSettings*>
				(deviceName, mouseSettings));
			i++;
		}
	}
}


status_t
MultipleMouseSettings::Archive(BMessage* into, bool deep) const
{
	std::map<BString, MouseSettings*>::const_iterator itr;
	for (itr = fMouseSettingsObject.begin(); itr != fMouseSettingsObject.end();
		++itr) {
		into->AddString("mouseDevice", itr->first);
		into->AddData("mouseSettings", B_ANY_TYPE, itr->second->GetSettings(),
			sizeof(*(itr->second->GetSettings())));
	}

	return B_OK;
}


status_t
MultipleMouseSettings::SaveSettings()
{
	BPath path;
	status_t status = GetSettingsPath(path);
	if (status < B_OK)
		return status;

	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	status = file.InitCheck();
	if (status != B_OK)
		return status;

	BMessage message;
	Archive(&message, true);
	message.Flatten(&file);

	return B_OK;
}


void
MultipleMouseSettings::Defaults()
{
	std::map<BString, MouseSettings*>::iterator itr;
	for (itr = fMouseSettingsObject.begin(); itr != fMouseSettingsObject.end();
		++itr) {
		itr->second->Defaults();
	}
}


#ifdef DEBUG
void
MultipleMouseSettings::Dump()
{
	std::map<BString, MouseSettings*>::iterator itr;
	for (itr = fMouseSettingsObject.begin();
		itr != fMouseSettingsObject.end(); ++itr) {
		printf("mouse_name:\t%s\n", itr->first.String());
		itr->second->Dump();
		printf("\n");
	}

}
#endif


MouseSettings*
MultipleMouseSettings::AddMouseSettings(BString mouse_name)
{
	MouseSettings* settings = GetMouseSettings(mouse_name);
	if (settings != NULL)
		return settings;

	settings = new(std::nothrow) MouseSettings();

	if(settings != NULL) {
		fMouseSettingsObject.insert(std::pair<BString, MouseSettings*>
			(mouse_name, settings));
		return settings;
	}
	return NULL;
}


MouseSettings*
MultipleMouseSettings::GetMouseSettings(BString mouse_name)
{
	std::map<BString, MouseSettings*>::iterator itr;
	itr = fMouseSettingsObject.find(mouse_name);

	if (itr != fMouseSettingsObject.end())
		return itr->second;
	return NULL;
 }

