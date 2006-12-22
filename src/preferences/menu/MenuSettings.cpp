/*
 * Copyright 2002-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		<unknown, please fill in who knows>
 */


#include "MenuSettings.h"

 
MenuSettings::MenuSettings()
{
	// the default settings. possibly a call to the app_server
	// would provide and execute this information, as it does
	// for get_menu_info and set_menu_info (or is this information
	// coming from libbe.so? or else where?). 
	fDefaultSettings.font_size = 12;
	//info.f_family = "test";
	//info.f_style = "test";
	fDefaultSettings.background_color = ui_color(B_MENU_BACKGROUND_COLOR);
	fDefaultSettings.separator = 0;
	fDefaultSettings.click_to_open = true;
	fDefaultSettings.triggers_always_shown = false;

	fPreviousSettings = fDefaultSettings;
}


MenuSettings::~MenuSettings()
{
}


/* static */
MenuSettings *
MenuSettings::GetInstance()
{
	static MenuSettings *settings = NULL;
	if (settings == NULL)
		settings = new MenuSettings();
	return settings;
}


void
MenuSettings::Get(menu_info &info) const
{
	get_menu_info(&info);
}


void
MenuSettings::Set(menu_info &info)
{
	get_menu_info(&fPreviousSettings);
	set_menu_info(&info);
}


void
MenuSettings::Revert()
{
	set_menu_info(&fPreviousSettings);
}


void
MenuSettings::ResetToDefaults()
{
	get_menu_info(&fPreviousSettings);
	set_menu_info(&fDefaultSettings);
}

