/*
 * Copyright 2002-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		<unknown, please fill in who knows>
 */


#include "AutoSettingsMenu.h"
#include "MenuSettings.h"


AutoSettingsMenu::AutoSettingsMenu(const char *name, menu_layout layout)
	: BMenu(name, layout)
{
}


void
AutoSettingsMenu::AttachedToWindow()
{
	menu_info info;
	MenuSettings::GetInstance()->Get(info);

	BFont font;		
	font.SetFamilyAndStyle(info.f_family, info.f_style);
	font.SetSize(info.font_size);
	SetFont(&font);
	SetViewColor(info.background_color);

	BMenu::AttachedToWindow();
}
