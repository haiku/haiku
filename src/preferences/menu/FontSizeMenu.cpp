/*
 * Copyright 2002-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		<unknown, please fill in who knows>
 */


#include "FontMenu.h"
#include "MenuSettings.h"
#include "msg.h"

#include <MenuItem.h>
#include <Message.h>
#include <String.h>
#include <Window.h>


FontSizeMenu::FontSizeMenu()
	:AutoSettingsMenu("Font Size", B_ITEMS_IN_COLUMN)
{		
	BMessage *msg = new BMessage(MENU_FONT_SIZE);
	msg->AddFloat("size", 9);
	BMenuItem *item = new BMenuItem("9", msg, 0, 0);
	AddItem(item);
	
	msg = new BMessage(MENU_FONT_SIZE);
	msg->AddFloat("size", 10);
	item = new BMenuItem("10", msg, 0, 0);
	AddItem(item);
	
	msg = new BMessage(MENU_FONT_SIZE);
	msg->AddFloat("size", 11);
	item = new BMenuItem("11", msg, 0, 0);
	AddItem(item);
	
	msg = new BMessage(MENU_FONT_SIZE);
	msg->AddFloat("size", 12);
	item = new BMenuItem("12", msg, 0, 0);
	AddItem(item);
	
	msg = new BMessage(MENU_FONT_SIZE);
	msg->AddFloat("size", 14);
	item = new BMenuItem("14", msg, 0, 0);
	AddItem(item);
	
	msg = new BMessage(MENU_FONT_SIZE);
	msg->AddFloat("size", 18);
	item = new BMenuItem("18", msg, 0, 0);
	AddItem(item);
	
	SetRadioMode(true);
}


void
FontSizeMenu::AttachedToWindow()
{
	AutoSettingsMenu::AttachedToWindow();

	// Mark the menuitem with the current font size
	menu_info info;
	MenuSettings::GetInstance()->Get(info);
	BString name;
	name << (int)info.font_size;
	BMenuItem *item = FindItem(name.String());
	if (item != NULL)
		item->SetMarked(true);

}
