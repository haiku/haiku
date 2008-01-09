/*
 * Copyright 2002-2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		<unknown, please fill in who knows>
 *		Stefano Ceccherini (stefano.ceccherini@gmail.com)
 */


#include "MenuSettings.h"
#include <Roster.h>
#include <Errors.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

 
MenuSettings::MenuSettings()
{
	// the default settings. possibly a call to the app_server
	// would provide and execute this information, as it does
	// for get_menu_info and set_menu_info (or is this information
	// coming from libbe.so? or else where?). 
	fDefaultSettings.font_size = 12;	
	sprintf(fDefaultSettings.f_family,"%s","Bitstream Vera Sans");
	sprintf(fDefaultSettings.f_style,"%s","Roman");
	rgb_color color;
	color.red = 216;
	color.blue = 216;
	color.green = 216;
	color.alpha = 255;
	fDefaultSettings.background_color = color;//ui_color(B_MENU_BACKGROUND_COLOR);
	fDefaultSettings.separator = 0;
	fDefaultSettings.click_to_open = true;
	fDefaultSettings.triggers_always_shown = false;
	fDefaultAltAsShortcut = true;
	
	get_menu_info(&fPreviousSettings);
	fPreviousAltAsShortcut = AltAsShortcut();
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
	set_menu_info(&info);
}


bool
MenuSettings::AltAsShortcut() const
{
	key_map *keys; 
	char* chars; 
	
	get_key_map(&keys, &chars);
	bool altAsShortcut = (keys->left_command_key == 0x5d) 
		&& (keys->right_command_key == 0x5f); 

	free(chars);
	free(keys);
	
	return altAsShortcut;	
}


void
MenuSettings::SetAltAsShortcut(bool altAsShortcut)
{
	if (altAsShortcut) {
		// This might not be the same for all keyboards
		set_modifier_key(B_LEFT_COMMAND_KEY, 0x5d);
		set_modifier_key(B_RIGHT_COMMAND_KEY, 0x5f);
		set_modifier_key(B_LEFT_CONTROL_KEY, 0x5c);
		set_modifier_key(B_RIGHT_OPTION_KEY, 0x60);
	} else {
		// This might not be the same for all keyboards
		set_modifier_key(B_LEFT_COMMAND_KEY, 0x5c);
		set_modifier_key(B_RIGHT_COMMAND_KEY, 0x60);
		set_modifier_key(B_LEFT_CONTROL_KEY, 0x5d);
		set_modifier_key(B_RIGHT_OPTION_KEY, 0x5f);
	}
	be_roster->Broadcast(new BMessage(B_MODIFIERS_CHANGED));	
}


void
MenuSettings::Revert()
{
	set_menu_info(&fPreviousSettings);
	SetAltAsShortcut(fPreviousAltAsShortcut);
}


void
MenuSettings::ResetToDefaults()
{
	set_menu_info(&fDefaultSettings);
	SetAltAsShortcut(fDefaultAltAsShortcut);
}


rgb_color
MenuSettings::BackgroundColor() const
{
	menu_info info;
	get_menu_info(&info);	
	return info.background_color;
}


rgb_color
MenuSettings::PreviousBackgroundColor() const
{
	return fPreviousSettings.background_color;
}


rgb_color
MenuSettings::DefaultBackgroundColor() const
{
	return fDefaultSettings.background_color;
}


bool
MenuSettings::IsDefaultable()
{
	menu_info info;
	get_menu_info(&info);

	return info.font_size !=  fDefaultSettings.font_size
		|| strcmp(info.f_family, fDefaultSettings.f_family) != 0
		|| strcmp(info.f_style, fDefaultSettings.f_style) != 0
		|| info.background_color != fDefaultSettings.background_color
		|| info.separator != fDefaultSettings.separator
		|| info.click_to_open != fDefaultSettings.click_to_open
		|| info.triggers_always_shown != fDefaultSettings.triggers_always_shown
		|| AltAsShortcut() != fDefaultAltAsShortcut;
}


bool
MenuSettings::IsRevertable()
{
	menu_info info;
	get_menu_info(&info);

	return info.font_size !=  fPreviousSettings.font_size
		|| strcmp(info.f_family, fPreviousSettings.f_family) != 0
		|| strcmp(info.f_style, fPreviousSettings.f_style) != 0
		|| info.background_color != fPreviousSettings.background_color
		|| info.separator != fPreviousSettings.separator
		|| info.click_to_open != fPreviousSettings.click_to_open
		|| info.triggers_always_shown != fPreviousSettings.triggers_always_shown
		|| AltAsShortcut() != fPreviousAltAsShortcut;
}

