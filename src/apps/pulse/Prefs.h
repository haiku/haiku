//****************************************************************************************
//
//	File:		Prefs.h
//
//	Written by:	Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//****************************************************************************************

#ifndef PREFS_H
#define PREFS_H

#include <File.h>
#include <Rect.h>

class Prefs {
	public:
		Prefs();
		bool Save();
		~Prefs();
		
		int window_mode, deskbar_icon_width;
		BRect normal_window_rect, mini_window_rect, prefs_window_rect;
		int normal_bar_color, mini_active_color, mini_idle_color, mini_frame_color,
			deskbar_active_color, deskbar_idle_color, deskbar_frame_color;
		bool normal_fade_colors;

	private:
		BFile *fFile;

		bool GetInt(const char *name, int *value, int *defaultvalue);
		bool GetBool(const char *name, bool *value, bool *defaultvalue);
		bool GetRect(const char *name, BRect *value, BRect *defaultvalue);
		bool PutInt(const char *name, int *value);
		bool PutBool(const char *name, bool *value);
		bool PutRect(const char *name, BRect *value);
		
		float GetNormalWindowHeight();
		void ComputeNormalWindowSize();
		BRect GetNormalWindowRect();
		BRect GetMiniWindowRect();

		bool fFatalError;
};

#endif	// PREFS_H
