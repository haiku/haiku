/*
 * Copyright 2003-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval,
 *		Axel Dörfler (axeld@pinc-software.de)
 *		Andrew McCall (mccall@digitalparadise.co.uk)
 *		Brecht Machiels (brecht@mos6581.org)
 */

#include <FindDirectory.h>
#include <File.h>
#include <Path.h>
#include <View.h>

#include <stdio.h>

#include "MouseSettings.h"

// The R5 settings file differs from that of OpenBeOS;
// the latter maps 16 different mouse buttons
#define R5_COMPATIBLE 1

static const bigtime_t kDefaultClickSpeed = 500000;
static const int32 kDefaultMouseSpeed = 65536;
static const int32 kDefaultMouseType = 3;	// 3 button mouse
static const int32 kDefaultAccelerationFactor = 65536;
static const bool kDefaultAcceptFirstClick = false;


MouseSettings::MouseSettings()
	:
	fWindowPosition(-1, -1)
{
	_RetrieveSettings();

	fOriginalSettings = fSettings;
	fOriginalMode = fMode;
	fOriginalFocusFollowsMouseMode = fFocusFollowsMouseMode;
	fOriginalAcceptFirstClick = fAcceptFirstClick;
}


MouseSettings::~MouseSettings()
{
	_SaveSettings();
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


void
MouseSettings::_RetrieveSettings()
{
	// retrieve current values

	if (get_mouse_map(&fSettings.map) != B_OK)
		fprintf(stderr, "error when get_mouse_map\n");
	if (get_click_speed(&fSettings.click_speed) != B_OK)
		fprintf(stderr, "error when get_click_speed\n");
	if (get_mouse_speed(&fSettings.accel.speed) != B_OK)
		fprintf(stderr, "error when get_mouse_speed\n");
	if (get_mouse_acceleration(&fSettings.accel.accel_factor) != B_OK)
		fprintf(stderr, "error when get_mouse_acceleration\n");
	if (get_mouse_type(&fSettings.type) != B_OK)
		fprintf(stderr, "error when get_mouse_type\n");

	fMode = mouse_mode();
	fFocusFollowsMouseMode = focus_follows_mouse_mode();
	fAcceptFirstClick = accept_first_click();

	// also try to load the window position from disk

	BPath path;
	if (_GetSettingsPath(path) < B_OK)
		return;

	BFile file(path.Path(), B_READ_ONLY);
	if (file.InitCheck() < B_OK)
		return;

#if R5_COMPATIBLE
	const off_t kOffset = sizeof(mouse_settings) - sizeof(mouse_map)
		+ sizeof(int32) * 3;
		// we have to do this because mouse_map counts 16 buttons in OBOS
#else
	const off_t kOffset = sizeof(mouse_settings);
#endif

	if (file.ReadAt(kOffset, &fWindowPosition, sizeof(BPoint))
		!= sizeof(BPoint)) {
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
MouseSettings::_SaveSettings()
{
	BPath path;
	status_t status = _GetSettingsPath(path);
	if (status < B_OK)
		return status;

	BFile file(path.Path(), B_READ_WRITE | B_CREATE_FILE);
	status = file.InitCheck();
	if (status < B_OK)
		return status;

#if R5_COMPATIBLE
	const off_t kOffset = sizeof(mouse_settings) - sizeof(mouse_map)
		+ sizeof(int32) * 3;
	// we have to do this because mouse_map counts 16 buttons in OBOS
#else
	const off_t kOffset = sizeof(mouse_settings);
#endif

	file.WriteAt(kOffset, &fWindowPosition, sizeof(BPoint));

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
	printf("accel:\t\t%s\n", fSettings.accel.enabled ? "enabled" : "disabled");
	printf("accel factor:\t%ld\n", fSettings.accel.accel_factor);
	printf("speed:\t\t%ld\n", fSettings.accel.speed);

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
	if (get_mouse_map(&map) == B_OK) {
		map.button[0] = B_PRIMARY_MOUSE_BUTTON;
		map.button[1] = B_SECONDARY_MOUSE_BUTTON;
		map.button[2] = B_TERTIARY_MOUSE_BUTTON;
		SetMapping(map);
	}
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
		|| fSettings.map.button[2] != B_TERTIARY_MOUSE_BUTTON;
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
	else
		fprintf(stderr, "error when set_mouse_type\n");
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
	else
		fprintf(stderr, "error when set_click_speed\n");
}


void
MouseSettings::SetMouseSpeed(int32 speed)
{
	if (set_mouse_speed(speed) == B_OK)
		fSettings.accel.speed = speed;
	else
		fprintf(stderr, "error when set_mouse_speed\n");
}


void
MouseSettings::SetAccelerationFactor(int32 factor)
{
	if (set_mouse_acceleration(factor) == B_OK)
		fSettings.accel.accel_factor = factor;
	else
		fprintf(stderr, "error when set_mouse_acceleration\n");
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
	if (set_mouse_map(&fSettings.map) != B_OK)
		fprintf(stderr, "error when set_mouse_map\n");
}


void
MouseSettings::SetMapping(mouse_map &map)
{
	if (set_mouse_map(&map) == B_OK)
		fSettings.map = map;
	else
		fprintf(stderr, "error when set_mouse_map\n");
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

