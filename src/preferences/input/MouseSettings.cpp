/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
*/


#include "MouseSettings.h"

#include <FindDirectory.h>
#include <File.h>
#include <Path.h>
#include <String.h>
#include <View.h>

#include <stdio.h>


// The R5 settings file differs from that of OpenBeOS;
// the latter maps 16 different mouse buttons
#define R5_COMPATIBLE 1

static const bigtime_t kDefaultClickSpeed = 500000;
static const int32 kDefaultMouseSpeed = 65536;
static const int32 kDefaultMouseType = 3;	// 3 button mouse
static const int32 kDefaultAccelerationFactor = 65536;
static const bool kDefaultAcceptFirstClick = true;


MouseSettings::MouseSettings(BString name)
	:
	fName(name)
{
	if (_RetrieveSettings() != B_OK)
		Defaults();

	fOriginalSettings = fSettings;
	fOriginalMode = fMode;
	fOriginalFocusFollowsMouseMode = fFocusFollowsMouseMode;
	fOriginalAcceptFirstClick = fAcceptFirstClick;
}


MouseSettings::MouseSettings(mouse_settings settings, BString name)
	:
	fSettings(settings)
{
	fName = name;

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
}


status_t
MouseSettings::_GetSettingsPath(BPath &path)
{
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status < B_OK)
		return status;

	path.Append(mouse_settings_file);
	return B_OK;
}


status_t
MouseSettings::_RetrieveSettings()
{
	// retrieve current values
	if (get_mouse_map(&fSettings.map) != B_OK)
		return B_ERROR;
	if (get_click_speed(&fSettings.click_speed) != B_OK)
		return B_ERROR;
	if (get_mouse_speed_by_name(fName, &fSettings.accel.speed) != B_OK)
		return B_ERROR;
	if (get_mouse_acceleration(&fSettings.accel.accel_factor) != B_OK)
		return B_ERROR;
	if (get_mouse_type_by_name(fName, &fSettings.type) != B_OK)
		return B_ERROR;

	fMode = mouse_mode();
	fFocusFollowsMouseMode = focus_follows_mouse_mode();
	fAcceptFirstClick = accept_first_click();

	return B_OK;
}


status_t
MouseSettings::_LoadLegacySettings()
{
	BPath path;
	if (_GetSettingsPath(path) < B_OK)
		return B_ERROR;

	BFile file(path.Path(), B_READ_ONLY);
	if (file.InitCheck() < B_OK)
		return B_ERROR;

	// Read the settings from the file
	file.Read((void*)&fSettings, sizeof(mouse_settings));

#ifdef DEBUG
	Dump();
#endif

	return B_OK;
}


#ifdef DEBUG
void
MouseSettings::Dump()
{
	printf("type:\t\t%" B_PRId32 " button mouse\n", fSettings.type);
	for (int i = 0; i < 5; i++) {
		printf("button[%d]: %" B_PRId32 "\n", i, fSettings.map.button[i]);
	}
	printf("click speed:\t%" B_PRId64 "\n", fSettings.click_speed);
	printf("accel:\t\t%s\n", fSettings.accel.enabled ? "enabled" : "disabled");
	printf("accel factor:\t%" B_PRId32 "\n", fSettings.accel.accel_factor);
	printf("speed:\t\t%" B_PRId32 "\n", fSettings.accel.speed);

	const char *mode = "unknown";
	switch (fMode) {
		case B_NORMAL_MOUSE:
			mode = "click to focus and raise";
			break;
		case B_CLICK_TO_FOCUS_MOUSE:
			mode = "click to focus";
			break;
		case B_FOCUS_FOLLOWS_MOUSE:
			mode = "focus follows mouse";
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
	printf("accept first click:\t%s\n",
		fAcceptFirstClick ? "enabled" : "disabled");
}
#endif


// Resets the settings to the system defaults
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

	mouse_map map;
	if (get_mouse_map(&map) != B_OK) {
		// Set some default values
		map.button[0] = B_PRIMARY_MOUSE_BUTTON;
		map.button[1] = B_SECONDARY_MOUSE_BUTTON;
		map.button[2] = B_TERTIARY_MOUSE_BUTTON;
		map.button[3] = B_MOUSE_BUTTON(4);
		map.button[4] = B_MOUSE_BUTTON(5);
		map.button[5] = B_MOUSE_BUTTON(6);
	}
	SetMapping(map);
}


// Checks if the settings are different then the system defaults
bool
MouseSettings::IsDefaultable()
{
	return fSettings.click_speed != kDefaultClickSpeed
		|| fSettings.accel.speed != kDefaultMouseSpeed
		|| fSettings.type != kDefaultMouseType
		|| fSettings.accel.accel_factor != kDefaultAccelerationFactor
		|| fMode != B_NORMAL_MOUSE
		|| fFocusFollowsMouseMode != B_NORMAL_FOCUS_FOLLOWS_MOUSE
		|| fAcceptFirstClick != kDefaultAcceptFirstClick
		|| fSettings.map.button[0] != B_PRIMARY_MOUSE_BUTTON
		|| fSettings.map.button[1] != B_SECONDARY_MOUSE_BUTTON
		|| fSettings.map.button[2] != B_TERTIARY_MOUSE_BUTTON
		|| fSettings.map.button[3] != B_MOUSE_BUTTON(4)
		|| fSettings.map.button[4] != B_MOUSE_BUTTON(5)
		|| fSettings.map.button[5] != B_MOUSE_BUTTON(6);
}


//	Reverts to the active settings at program startup
void
MouseSettings::Revert()
{
	SetClickSpeed(fOriginalSettings.click_speed);
	SetMouseSpeed(fOriginalSettings.accel.speed);
	SetMouseType(fOriginalSettings.type);
	SetAccelerationFactor(fOriginalSettings.accel.accel_factor);
	SetMouseMode(fOriginalMode);
	SetFocusFollowsMouseMode(fOriginalFocusFollowsMouseMode);
	SetAcceptFirstClick(fOriginalAcceptFirstClick);

	SetMapping(fOriginalSettings.map);
}


// Checks if the settings are different then the original settings
bool
MouseSettings::IsRevertable()
{
	return fSettings.click_speed != fOriginalSettings.click_speed
		|| fSettings.accel.speed != fOriginalSettings.accel.speed
		|| fSettings.type != fOriginalSettings.type
		|| fSettings.accel.accel_factor != fOriginalSettings.accel.accel_factor
		|| fMode != fOriginalMode
		|| fFocusFollowsMouseMode != fOriginalFocusFollowsMouseMode
		|| fAcceptFirstClick != fOriginalAcceptFirstClick
		|| fSettings.map.button[0] != fOriginalSettings.map.button[0]
		|| fSettings.map.button[1] != fOriginalSettings.map.button[1]
		|| fSettings.map.button[2] != fOriginalSettings.map.button[2]
		|| fSettings.map.button[3] != fOriginalSettings.map.button[3]
		|| fSettings.map.button[4] != fOriginalSettings.map.button[4]
		|| fSettings.map.button[5] != fOriginalSettings.map.button[5];
}


void
MouseSettings::SetMouseType(int32 type)
{
	if (set_mouse_type_by_name(fName, type) == B_OK)
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
	if (set_mouse_speed_by_name(fName, speed) == B_OK)
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
	set_mouse_map(&fSettings.map);
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


void
MouseSettings::SetFocusFollowsMouseMode(mode_focus_follows_mouse mode)
{
	set_focus_follows_mouse_mode(mode);
	fFocusFollowsMouseMode = mode;
}


void
MouseSettings::SetAcceptFirstClick(bool accept_first_click)
{
	set_accept_first_click(accept_first_click);
	fAcceptFirstClick = accept_first_click;
}


mouse_settings*
MouseSettings::GetSettings() {
	return &fSettings;
}


MultipleMouseSettings::MultipleMouseSettings()
{
	fDeprecatedMouseSettings = NULL;
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
	for (itr = fMouseSettingsObject.begin(); itr != fMouseSettingsObject.end();
		++itr)
		delete itr->second;

	delete fDeprecatedMouseSettings;
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
	// retrieve current values
	// also try to load the window position from disk

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
			MouseSettings* mouseSettings
				= new MouseSettings(*settings, deviceName);
			fMouseSettingsObject.insert(std::pair<BString, MouseSettings*>
				(deviceName, mouseSettings));
			i++;
		}
	} else {
		// Does not look like a BMessage, try loading using the old format
		fDeprecatedMouseSettings = new MouseSettings("");
		if (fDeprecatedMouseSettings->_LoadLegacySettings() != B_OK) {
			delete fDeprecatedMouseSettings;
			fDeprecatedMouseSettings = NULL;
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
	for (itr = fMouseSettingsObject.begin(); itr != fMouseSettingsObject.end();
		++itr) {
		printf("mouse_name:\t%s\n", itr->first.String());
		itr->second->Dump();
		printf("\n");
	}

}
#endif


MouseSettings*
MultipleMouseSettings::AddMouseSettings(BString mouse_name)
{
	if(fDeprecatedMouseSettings != NULL) {
		MouseSettings* RetrievedSettings = new (std::nothrow) MouseSettings
			(*(fDeprecatedMouseSettings->GetSettings()), mouse_name);

		if (RetrievedSettings != NULL) {
			fMouseSettingsObject.insert(std::pair<BString, MouseSettings*>
				(mouse_name, RetrievedSettings));

			return RetrievedSettings;
		}
	}

	MouseSettings* settings = GetMouseSettings(mouse_name);
	if (settings)
		return settings;

	settings = new (std::nothrow) MouseSettings(mouse_name);
	if (settings == NULL)
		return NULL;

	fMouseSettingsObject.insert(std::pair<BString, MouseSettings*>
		(mouse_name, settings));
	return settings;
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
