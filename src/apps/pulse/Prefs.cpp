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
#include "Common.h"
#include "PulseApp.h"
#include <FindDirectory.h>
#include <Path.h>
#include <interface/Screen.h>
#include <kernel/OS.h>
#include <stdio.h>
#include <string.h>

Prefs::Prefs() {
	BPath path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	path.Append("Pulse_settings");
	file = new BFile(path.Path(), B_READ_WRITE | B_CREATE_FILE);
	
	int i = NORMAL_WINDOW_MODE;
	if (!GetInt("window_mode", &window_mode, &i)) {
		fatalerror = true;
		return;
	}
	
	// These three prefs require a connection to the app_server
	BRect r = GetNormalWindowRect();
	if (!GetRect("normal_window_rect", &normal_window_rect, &r)) {
		fatalerror = true;
		return;
	}
	
	r = GetMiniWindowRect();
	if (!GetRect("mini_window_rect", &mini_window_rect, &r)) {
		fatalerror = true;
		return;
	}
	
	r.Set(100, 100, 415, 329);
	if (!GetRect("prefs_window_rect", &prefs_window_rect, &r)) {
		fatalerror = true;
		return;
	}
	
	i = DEFAULT_NORMAL_BAR_COLOR;
	if (!GetInt("normal_bar_color", &normal_bar_color, &i)) {
		fatalerror = true;
		return;
	}
	
	i = DEFAULT_MINI_ACTIVE_COLOR;
	if (!GetInt("mini_active_color", &mini_active_color, &i)) {
		fatalerror = true;
		return;
	}
	
	i = DEFAULT_MINI_IDLE_COLOR;
	if (!GetInt("mini_idle_color", &mini_idle_color, &i)) {
		fatalerror = true;
		return;
	}
	
	i = DEFAULT_MINI_FRAME_COLOR;
	if (!GetInt("mini_frame_color", &mini_frame_color, &i)) {
		fatalerror = true;
		return;
	}
	
	i = DEFAULT_DESKBAR_ACTIVE_COLOR;
	if (!GetInt("deskbar_active_color", &deskbar_active_color, &i)) {
		fatalerror = true;
		return;
	}
	
	i = DEFAULT_DESKBAR_IDLE_COLOR;
	if (!GetInt("deskbar_idle_color", &deskbar_idle_color, &i)) {
		fatalerror = true;
		return;
	}
	
	i = DEFAULT_DESKBAR_FRAME_COLOR;
	if (!GetInt("deskbar_frame_color", &deskbar_frame_color, &i)) {
		fatalerror = true;
		return;
	}
	
	bool b = DEFAULT_NORMAL_FADE_COLORS;
	if (!GetBool("normal_fade_colors", &normal_fade_colors, &b)) {
		fatalerror = true;
		return;
	}
	
	// Use the default size unless it would prevent having at least
	// a one pixel wide display per CPU... this will only happen with > quad
	i = DEFAULT_DESKBAR_ICON_WIDTH;
	if (i < GetMinimumViewWidth()) i = GetMinimumViewWidth();
	if (!GetInt("deskbar_icon_width", &deskbar_icon_width, &i)) {
		fatalerror = true;
		return;
	}
}

BRect Prefs::GetNormalWindowRect() {
	system_info sys_info;
	get_system_info(&sys_info);
	
	float height = PROGRESS_MTOP + PROGRESS_MBOTTOM + sys_info.cpu_count * ITEM_OFFSET;
	if (PULSEVIEW_MIN_HEIGHT > height) {
		height = PULSEVIEW_MIN_HEIGHT;
	}
	
	// Dock the window in the lower right hand corner just like the original
	BRect r(0, 0, PULSEVIEW_WIDTH, height);
	BRect screen_rect = BScreen(B_MAIN_SCREEN_ID).Frame();
	r.OffsetTo(screen_rect.right - r.Width() - 5, screen_rect.bottom - r.Height() - 5);
	return r;
}

BRect Prefs::GetMiniWindowRect() {
	// Lower right hand corner by default
	BRect screen_rect = BScreen(B_MAIN_SCREEN_ID).Frame();
	screen_rect.left = screen_rect.right - 30;
	screen_rect.top = screen_rect.bottom - 150;
	screen_rect.OffsetBy(-5, -5);
	return screen_rect;
}

bool Prefs::GetInt(char *name, int *value, int *defaultvalue) {
	status_t err = file->ReadAttr(name, B_INT32_TYPE, 0, value, 4);
	if (err == B_ENTRY_NOT_FOUND) {
		*value = *defaultvalue;
		if (file->WriteAttr(name, B_INT32_TYPE, 0, defaultvalue, 4) < 0) {
			printf("WriteAttr on %s died\n", name);
			fatalerror = true;
			return false;
		}
	} else if  (err < 0) {
		printf("Unknown error reading %s:\n%s\n", name, strerror(err));
		fatalerror = true;
		return false;
	}
	return true;
}

bool Prefs::GetBool(char *name, bool *value, bool *defaultvalue) {
	status_t err = file->ReadAttr(name, B_BOOL_TYPE, 0, value, 1);
	if (err == B_ENTRY_NOT_FOUND) {
		*value = *defaultvalue;
		if (file->WriteAttr(name, B_BOOL_TYPE, 0, defaultvalue, 1) < 0) {
			printf("WriteAttr on %s died\n", name);
			fatalerror = true;
			return false;
		}
	} else if (err < 0) {
		printf("Unknown error reading %s:\n%s\n", name, strerror(err));
		fatalerror = true;
		return false;
	}
	return true;
}

bool Prefs::GetRect(char *name, BRect *value, BRect *defaultvalue) {
	status_t err = file->ReadAttr(name, B_RECT_TYPE, 0, value, sizeof(BRect));
	if (err == B_ENTRY_NOT_FOUND) {
		*value = *defaultvalue;
		if (file->WriteAttr(name, B_RECT_TYPE, 0, defaultvalue, sizeof(BRect)) < 0) {
			printf("WriteAttr on %s died\n", name);
			fatalerror = true;
			return false;
		}
	} else if (err < 0) {
		printf("Unknown error reading %s:\n%s\n", name, strerror(err));
		fatalerror = true;
		return false;
	}
	return true;
}

bool Prefs::PutInt(char *name, int *value){
	status_t err = file->WriteAttr(name, B_INT32_TYPE, 0, value, 4);
	if (err < 0) {
		printf("Unknown error writing %s:\n%s\n", name, strerror(err));
		fatalerror = true;
		return false;
	}
	return true;
}

bool Prefs::PutBool(char *name, bool *value) {
	status_t err = file->WriteAttr(name, B_BOOL_TYPE, 0, value, 1);
	if (err < 0) {
		printf("Unknown error writing %s:\n%s\n", name, strerror(err));
		fatalerror = true;
		return false;
	}
	return true;
}

bool Prefs::PutRect(char *name, BRect *value) {
	status_t err = file->WriteAttr(name, B_RECT_TYPE, 0, value, sizeof(BRect));
	if (err < 0) {
		printf("Unknown error writing %s:\n%s\n", name, strerror(err));
		fatalerror = true;
		return false;
	}
	return true;
}

bool Prefs::Save() {
	if (!PutInt("window_mode", &window_mode)) return false;
	if (!PutRect("normal_window_rect", &normal_window_rect)) return false;
	if (!PutRect("mini_window_rect", &mini_window_rect)) return false;
	if (!PutRect("prefs_window_rect", &prefs_window_rect)) return false;
	if (!PutInt("normal_bar_color", &normal_bar_color)) return false;
	if (!PutInt("mini_active_color", &mini_active_color)) return false;
	if (!PutInt("mini_idle_color", &mini_idle_color)) return false;
	if (!PutInt("mini_frame_color", &mini_frame_color)) return false;
	if (!PutInt("deskbar_active_color", &deskbar_active_color)) return false;
	if (!PutInt("deskbar_idle_color", &deskbar_idle_color)) return false;
	if (!PutInt("deskbar_frame_color", &deskbar_frame_color)) return false;
	if (!PutBool("normal_fade_colors", &normal_fade_colors)) return false;
	if (!PutInt("deskbar_icon_width", &deskbar_icon_width)) return false;
	return true;
}

Prefs::~Prefs() {
	delete file;
}