/*
 * Copyright 2016 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <boot/menu.h>
#include <boot/platform/generic/text_menu.h>

#include "efi_platform.h"
#include "video.h"


void
platform_add_menus(Menu *menu)
{
	MenuItem *item;

	switch (menu->Type()) {
		case MAIN_MENU:
			item = new(std::nothrow)MenuItem("Select video mode", video_mode_menu());
			if (item != NULL) {
				menu->AddItem(item);
				item->SetTarget(video_mode_hook);
				item->SetShortcut('v');
			}

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


size_t
platform_get_user_input_text(Menu *menu, MenuItem *item, char *buffer,
	size_t bufferSize)
{
	return platform_generic_get_user_input_text(menu, item, buffer,
		bufferSize);
}
