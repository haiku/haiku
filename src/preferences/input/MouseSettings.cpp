/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
 */


#include "MouseSettings.h"

#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <String.h>
#include <View.h>

#include <stdio.h>


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


MouseSettings::~MouseSettings()
{
}


status_t
MouseSettings::_RetrieveSettings()
{
	// retrieve current values
	if (get_mouse_map(fName, &fSettings.map) != B_OK)
		return B_ERROR;
	if (get_click_speed(fName, &fSettings.click_speed) != B_OK)
		return B_ERROR;
	if (get_mouse_speed(fName, &fSettings.accel.speed) != B_OK)
		return B_ERROR;
	if (get_mouse_acceleration(fName, &fSettings.accel.accel_factor) != B_OK)
		return B_ERROR;
	if (get_mouse_type(fName, &fSettings.type) != B_OK)
		return B_ERROR;

	fMode = mouse_mode();
	fFocusFollowsMouseMode = focus_follows_mouse_mode();
	fAcceptFirstClick = accept_first_click();

	return B_OK;
}


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
	for (int i = 0; i < B_MAX_MOUSE_BUTTONS; i++)
		map.button[i] = B_MOUSE_BUTTON(i + 1);
	SetMapping(map);
}


// Checks if the settings are different then the system defaults
bool
MouseSettings::IsDefaultable() const
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
MouseSettings::IsRevertable() const
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
	if (set_mouse_type(fName, type) == B_OK)
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
	if (set_click_speed(fName, clickSpeed) == B_OK)
		fSettings.click_speed = clickSpeed;
}


void
MouseSettings::SetMouseSpeed(int32 speed)
{
	if (set_mouse_speed(fName, speed) == B_OK)
		fSettings.accel.speed = speed;
}


void
MouseSettings::SetAccelerationFactor(int32 factor)
{
	if (set_mouse_acceleration(fName, factor) == B_OK)
		fSettings.accel.accel_factor = factor;
}


uint32
MouseSettings::Mapping(int32 index) const
{
	return fSettings.map.button[index];
}


void
MouseSettings::Mapping(mouse_map& map) const
{
	map = fSettings.map;
}


void
MouseSettings::SetMapping(int32 index, uint32 button)
{
	fSettings.map.button[index] = button;
	set_mouse_map(fName, &fSettings.map);
}


void
MouseSettings::SetMapping(mouse_map& map)
{
	if (set_mouse_map(fName, &map) == B_OK)
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
MouseSettings::GetSettings()
{
	return &fSettings;
}


MultipleMouseSettings::MultipleMouseSettings()
{
}


MultipleMouseSettings::~MultipleMouseSettings()
{
	std::map<BString, MouseSettings*>::iterator itr;
	for (itr = fMouseSettingsObject.begin(); itr != fMouseSettingsObject.end(); ++itr)
		delete itr->second;
}


MouseSettings*
MultipleMouseSettings::AddMouseSettings(BString mouse_name)
{
	std::map<BString, MouseSettings*>::iterator itr;
	itr = fMouseSettingsObject.find(mouse_name);

	if (itr != fMouseSettingsObject.end())
		return itr->second;

	MouseSettings* settings = new(std::nothrow) MouseSettings(mouse_name);
	if (settings == NULL)
		return NULL;

	fMouseSettingsObject.insert(
		std::pair<BString, MouseSettings*>(mouse_name, settings));
	return settings;
}
