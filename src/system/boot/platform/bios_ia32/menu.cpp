/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "smp.h"
#include "video.h"

#include <boot/menu.h>
#include <boot/platform/generic/text_menu.h>
#include <safemode.h>


void
platform_add_menus(Menu *menu)
{
	MenuItem *item;

	switch (menu->Type()) {
		case MAIN_MENU:
			menu->AddItem(item = new MenuItem("Select fail-safe video mode", video_mode_menu()));
			item->SetTarget(video_mode_hook);
			break;
		case SAFE_MODE_MENU:
			smp_add_safemode_menus(menu);

			menu->AddItem(item = new MenuItem("Don't call the BIOS"));
			item->SetType(MENU_ITEM_MARKABLE);
			break;
		default:
			break;
	}
}


void 
platform_update_menu_item(Menu *menu, MenuItem *item)
{
	platform_generic_update_text_menu_item(menu, item);
}


void
platform_run_menu(Menu *menu)
{
	platform_generic_run_text_menu(menu);
}

