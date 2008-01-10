/*
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "video.h"
//#include "mmu.h"
#include "images.h"

#include <arch/cpu.h>
#include <boot/stage2.h>
#include <boot/platform.h>
#include <boot/menu.h>
#include <boot/kernel_args.h>
#include <util/list.h>
#include <drivers/driver_settings.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//#define TRACE_VIDEO
#ifdef TRACE_VIDEO
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


// XXX: use falcon video monitor detection and build possible mode list there...


//	#pragma mark -


bool
video_mode_hook(Menu *menu, MenuItem *item)
{
	// nothing yet
#if 0
	// find selected mode
	video_mode *mode = NULL;

	menu = item->Submenu();
	item = menu->FindMarked();
	if (item != NULL) {
		switch (menu->IndexOf(item)) {
			case 0:
				// "Default" mode special
				sMode = sDefaultMode;
				sModeChosen = false;
				return true;
			case 1:
				// "Standard VGA" mode special
				// sets sMode to NULL which triggers VGA mode
				break;
			default:
				mode = (video_mode *)item->Data();
				break;
		}
	}

	if (mode != sMode) {
		// update standard mode
		// ToDo: update fb settings!
		sMode = mode;
	}

	sModeChosen = true;
#endif
	return true;
}


Menu *
video_mode_menu()
{
	Menu *menu = new(nothrow) Menu(CHOICE_MENU, "Select Video Mode");
	MenuItem *item;

	menu->AddItem(item = new(nothrow) MenuItem("Default"));
	item->SetMarked(true);
	item->Select(true);
	item->SetHelpText("The Default video mode is the one currently configured "
		"in the system. If there is no mode configured yet, a viable mode will "
		"be chosen automatically.");

#if 0
	menu->AddItem(new(nothrow) MenuItem("Standard VGA"));

	video_mode *mode = NULL;
	while ((mode = (video_mode *)list_get_next_item(&sModeList, mode)) != NULL) {
		char label[64];
		sprintf(label, "%ux%u %u bit", mode->width, mode->height,
			mode->bits_per_pixel);

		menu->AddItem(item = new(nothrow) MenuItem(label));
		item->SetData(mode);
	}
#endif

	menu->AddSeparatorItem();
	menu->AddItem(item = new(nothrow) MenuItem("Return to main menu"));
	item->SetType(MENU_ITEM_NO_CHOICE);

	return menu;
}


//	#pragma mark -


extern "C" void
platform_switch_to_logo(void)
{
	// ToDo: implement me
}


extern "C" void
platform_switch_to_text_mode(void)
{
	// ToDo: implement me
}


extern "C" status_t
platform_init_video(void)
{
	// ToDo: implement me
	return B_OK;
}

