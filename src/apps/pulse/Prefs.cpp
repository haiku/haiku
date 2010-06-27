//****************************************************************************************
//
//	File:		Prefs.cpp
//
//	Written by:	Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//****************************************************************************************

#include "Prefs.h"

#include <stdio.h>
#include <string.h>

#include <FindDirectory.h>
#include <OS.h>
#include <Path.h>
#include <Screen.h>

#include "Common.h"
#include "PulseApp.h"


Prefs::Prefs()
	:
	fFatalError(false)
{
	BPath path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	path.Append("Pulse_settings");
	fFile = new BFile(path.Path(), B_READ_WRITE | B_CREATE_FILE);
	if (fFile->InitCheck() != B_OK) {
		// try to open read-only
		if (fFile->SetTo(path.Path(), B_READ_ONLY) != B_OK) {
			fFatalError = true;
			return;
		}
	}

	int i = NORMAL_WINDOW_MODE;
	if (!GetInt("window_mode", &window_mode, &i)) {
		fFatalError = true;
		return;
	}

	// These three prefs require a connection to the app_server
	BRect r = GetNormalWindowRect();
	if (!GetRect("normal_window_rect", &normal_window_rect, &r)) {
		fFatalError = true;
		return;
	}
	// While normal window position is under user control, size is not.
	// Width is fixed and height must be dynamically computed each time, 
	// as number of CPUs could change since boot.
	ComputeNormalWindowSize();
	
	r = GetMiniWindowRect();
	if (!GetRect("mini_window_rect", &mini_window_rect, &r)) {
		fFatalError = true;
		return;
	}

	r.Set(100, 100, 415, 329);
	if (!GetRect("prefs_window_rect", &prefs_window_rect, &r)) {
		fFatalError = true;
		return;
	}

	i = DEFAULT_NORMAL_BAR_COLOR;
	if (!GetInt("normal_bar_color", &normal_bar_color, &i)) {
		fFatalError = true;
		return;
	}

	i = DEFAULT_MINI_ACTIVE_COLOR;
	if (!GetInt("mini_active_color", &mini_active_color, &i)) {
		fFatalError = true;
		return;
	}

	i = DEFAULT_MINI_IDLE_COLOR;
	if (!GetInt("mini_idle_color", &mini_idle_color, &i)) {
		fFatalError = true;
		return;
	}

	i = DEFAULT_MINI_FRAME_COLOR;
	if (!GetInt("mini_frame_color", &mini_frame_color, &i)) {
		fFatalError = true;
		return;
	}

	i = DEFAULT_DESKBAR_ACTIVE_COLOR;
	if (!GetInt("deskbar_active_color", &deskbar_active_color, &i)) {
		fFatalError = true;
		return;
	}

	i = DEFAULT_DESKBAR_IDLE_COLOR;
	if (!GetInt("deskbar_idle_color", &deskbar_idle_color, &i)) {
		fFatalError = true;
		return;
	}

	i = DEFAULT_DESKBAR_FRAME_COLOR;
	if (!GetInt("deskbar_frame_color", &deskbar_frame_color, &i)) {
		fFatalError = true;
		return;
	}

	bool b = DEFAULT_NORMAL_FADE_COLORS;
	if (!GetBool("normal_fade_colors", &normal_fade_colors, &b)) {
		fFatalError = true;
		return;
	}

	// Use the default size unless it would prevent having at least
	// a one pixel wide display per CPU... this will only happen with > quad
	i = DEFAULT_DESKBAR_ICON_WIDTH;
	if (i < GetMinimumViewWidth())
		i = GetMinimumViewWidth();
	if (!GetInt("deskbar_icon_width", &deskbar_icon_width, &i)) {
		fFatalError = true;
		return;
	}
}


Prefs::~Prefs()
{
	delete fFile;
}


float
Prefs::GetNormalWindowHeight()
{
	system_info sys_info;
	get_system_info(&sys_info);

	float height = PROGRESS_MTOP + PROGRESS_MBOTTOM
		+ sys_info.cpu_count * ITEM_OFFSET;
	if (PULSEVIEW_MIN_HEIGHT > height)
		height = PULSEVIEW_MIN_HEIGHT;

	return height;
}


void
Prefs::ComputeNormalWindowSize()
{
	normal_window_rect.right = normal_window_rect.left + PULSEVIEW_WIDTH;
	normal_window_rect.bottom = normal_window_rect.top + GetNormalWindowHeight();
}


BRect
Prefs::GetNormalWindowRect()
{
	// Dock the window in the lower right hand corner just like the original
	BRect r(0, 0, PULSEVIEW_WIDTH, GetNormalWindowHeight());
	BRect screen_rect = BScreen(B_MAIN_SCREEN_ID).Frame();
	r.OffsetTo(screen_rect.right - r.Width() - 5, screen_rect.bottom - r.Height() - 5);
	return r;
}


BRect
Prefs::GetMiniWindowRect()
{
	// Lower right hand corner by default
	BRect screen_rect = BScreen(B_MAIN_SCREEN_ID).Frame();
	screen_rect.left = screen_rect.right - 30;
	screen_rect.top = screen_rect.bottom - 150;
	screen_rect.OffsetBy(-5, -5);
	return screen_rect;
}


bool
Prefs::GetInt(const char *name, int *value, int *defaultvalue)
{
	status_t err = fFile->ReadAttr(name, B_INT32_TYPE, 0, value, 4);
	if (err == B_ENTRY_NOT_FOUND) {
		*value = *defaultvalue;
		if (fFile->WriteAttr(name, B_INT32_TYPE, 0, defaultvalue, 4) < 0) {
			printf("WriteAttr on %s died\n", name);
			fFatalError = true;
		}
	} else if  (err < 0) {
		printf("Unknown error reading %s:\n%s\n", name, strerror(err));
		fFatalError = true;
		return false;
	}
	return true;
}


bool
Prefs::GetBool(const char *name, bool *value, bool *defaultvalue)
{
	status_t err = fFile->ReadAttr(name, B_BOOL_TYPE, 0, value, 1);
	if (err == B_ENTRY_NOT_FOUND) {
		*value = *defaultvalue;
		if (fFile->WriteAttr(name, B_BOOL_TYPE, 0, defaultvalue, 1) < 0) {
			printf("WriteAttr on %s died\n", name);
			fFatalError = true;
		}
	} else if (err < 0) {
		printf("Unknown error reading %s:\n%s\n", name, strerror(err));
		fFatalError = true;
		return false;
	}
	return true;
}


bool
Prefs::GetRect(const char *name, BRect *value, BRect *defaultvalue)
{
	status_t err = fFile->ReadAttr(name, B_RECT_TYPE, 0, value, sizeof(BRect));
	if (err == B_ENTRY_NOT_FOUND) {
		*value = *defaultvalue;
		if (fFile->WriteAttr(name, B_RECT_TYPE, 0, defaultvalue, sizeof(BRect)) < 0) {
			printf("WriteAttr on %s died\n", name);
			fFatalError = true;
		}
	} else if (err < 0) {
		printf("Unknown error reading %s:\n%s\n", name, strerror(err));
		fFatalError = true;
		return false;
	}
	return true;
}


bool
Prefs::PutInt(const char *name, int *value)
{
	status_t err = fFile->WriteAttr(name, B_INT32_TYPE, 0, value, 4);
	if (err < 0) {
		printf("Unknown error writing %s:\n%s\n", name, strerror(err));
		fFatalError = true;
		return false;
	}
	return true;
}


bool
Prefs::PutBool(const char *name, bool *value)
{
	status_t err = fFile->WriteAttr(name, B_BOOL_TYPE, 0, value, 1);
	if (err < 0) {
		printf("Unknown error writing %s:\n%s\n", name, strerror(err));
		fFatalError = true;
		return false;
	}
	return true;
}


bool
Prefs::PutRect(const char *name, BRect *value)
{
	status_t err = fFile->WriteAttr(name, B_RECT_TYPE, 0, value, sizeof(BRect));
	if (err < 0) {
		printf("Unknown error writing %s:\n%s\n", name, strerror(err));
		fFatalError = true;
		return false;
	}
	return true;
}


bool
Prefs::Save()
{
	if (fFatalError)
		return false;

	if (!PutInt("window_mode", &window_mode)
		|| !PutRect("normal_window_rect", &normal_window_rect)
		|| !PutRect("mini_window_rect", &mini_window_rect)
		|| !PutRect("prefs_window_rect", &prefs_window_rect)
		|| !PutInt("normal_bar_color", &normal_bar_color)
		|| !PutInt("mini_active_color", &mini_active_color)
		|| !PutInt("mini_idle_color", &mini_idle_color)
		|| !PutInt("mini_frame_color", &mini_frame_color)
		|| !PutInt("deskbar_active_color", &deskbar_active_color)
		|| !PutInt("deskbar_idle_color", &deskbar_idle_color)
		|| !PutInt("deskbar_frame_color", &deskbar_frame_color)
		|| !PutBool("normal_fade_colors", &normal_fade_colors)
		|| !PutInt("deskbar_icon_width", &deskbar_icon_width))
		return false;

	return true;
}

