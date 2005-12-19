/*
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <boot/platform.h>
#include <boot/menu.h>


void
platform_add_menus(Menu *menu)
{
	// ToDo: implement me!

	switch (menu->Type()) {
		case MAIN_MENU:
			break;
		case SAFE_MODE_MENU:
			break;
		default:
			break;
	}
}


void
platform_update_menu_item(Menu *menu, MenuItem *item)
{
	if (menu->IsHidden())
		return;

	// ToDo: implement me!
}


void
platform_run_menu(Menu *menu)
{
	// ToDo: implement me!
}

