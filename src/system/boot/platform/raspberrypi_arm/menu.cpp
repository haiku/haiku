/*
 * Copyright 2009 Jonas Sundström, jonas@kirilla.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <boot/menu.h>
#include <boot/platform/generic/text_menu.h>


void
platform_add_menus(Menu* menu)
{
	MenuItem* item;

	switch (menu->Type()) {
		case MAIN_MENU:
		case SAFE_MODE_MENU:
			menu->AddItem(item = new(nothrow) MenuItem("A menu item"));
			item->SetType(MENU_ITEM_MARKABLE);
			item->SetData(0);
			item->SetHelpText("A helpful text.");

			menu->AddItem(item = new(nothrow) MenuItem("Another menu item"));
			item->SetHelpText("Some more helpful text.");
			item->SetType(MENU_ITEM_MARKABLE);
			break;
		default:
			break;
	}
}


void
platform_update_menu_item(Menu* menu, MenuItem* item)
{
	platform_generic_update_text_menu_item(menu, item);
}


void
platform_run_menu(Menu* menu)
{
	platform_generic_run_text_menu(menu);
}


void
platform_get_user_input_text(Menu *menu, MenuItem *item, char *buffer,
	size_t bufferSize)
{
	platform_generic_get_user_input_text(menu, item, buffer, bufferSize);
}
