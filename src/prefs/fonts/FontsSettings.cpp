/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 */
#include <Application.h>
#include <FindDirectory.h>
#include <File.h>
#include <Path.h>
#include <Font.h>
#include <String.h>
#include <stdio.h>
#include <Message.h>

#include "FontsSettings.h"

static const char kSettingsFile[] = "Font_Settings";

FontsSettings::FontsSettings()
{
	BPath path;
	BMessage msg;
	
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append(kSettingsFile);
		BFile file(path.Path(), B_READ_ONLY);
		
		if (file.InitCheck() != B_OK) 
			SetDefaults();
		else
		if (msg.Unflatten(&file) != B_OK)
			SetDefaults();
		else
		{
			msg.FindPoint("windowlocation",&fCorner);
		}
	}
}	
	
FontsSettings::~FontsSettings()
{
	BPath path;
	BMessage msg;
	
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) < B_OK) 
		return;
	
	path.Append(kSettingsFile);
	BFile file(path.Path(), B_WRITE_ONLY|B_CREATE_FILE);
	
	if (file.InitCheck() == B_OK) {
		msg.AddPoint("windowlocation",fCorner);
		
		// We need to add this to the settings file so that the app_server
		// can get the font cache info on startup. This is the only
		// place in the Fonts app that we actually need to do this because
		// any other changes made are made through the API
		struct font_cache_info cacheInfo;
		
		get_font_cache_info(B_SCREEN_FONT_CACHE|B_DEFAULT_CACHE_SETTING, 
							&cacheInfo);
		msg.AddInt32("screencachesize",cacheInfo.cache_size);
		
		get_font_cache_info(B_PRINTING_FONT_CACHE|B_DEFAULT_CACHE_SETTING,
							&cacheInfo);
		msg.AddInt32("printcachesize",cacheInfo.cache_size);
		
		msg.Flatten(&file);
	}
}


void
FontsSettings::SetWindowCorner(BPoint where)
{
	fCorner = where;
}

void
FontsSettings::SetDefaults(void)
{
	fCorner.Set(100,100);
}

