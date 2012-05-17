/*
 * Copyright 2009, Johannes Wischert
 * Distributed under the terms of the MIT License.
 */


#include "video.h"
//XXX
#ifdef __ARM__
#include "arch_video.h"
#endif

#include <arch/cpu.h>
#include <boot/stage2.h>
#include <boot/platform.h>
#include <boot/menu.h>
#include <boot/kernel_args.h>
#include <boot/platform/generic/video.h>
#include <util/list.h>
#include <drivers/driver_settings.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define TRACE_VIDEO
#ifdef TRACE_VIDEO
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

void *gFrameBufferBase = NULL;


//	#pragma mark -


bool
video_mode_hook(Menu *menu, MenuItem *item)
{
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


	menu->AddSeparatorItem();
	menu->AddItem(item = new(nothrow) MenuItem("Return to main menu"));
	item->SetType(MENU_ITEM_NO_CHOICE);

	return menu;
}



//	#pragma mark -


extern "C" void
platform_set_palette(const uint8 *palette)
{
}


extern "C" void
platform_blit4(addr_t frameBuffer, const uint8 *data, uint16 width, uint16 height,
	uint16 imageWidth, uint16 left, uint16 top)
{
}

extern "C" void
platform_switch_to_logo(void)
{
	TRACE(("%s()\n", __FUNCTION__));
	// in debug mode, we'll never show the logo
	if ((platform_boot_options() & BOOT_OPTION_DEBUG_OUTPUT) != 0)
		return;

	//XXX: not yet, DISABLED
	return;

	status_t err;

#ifdef __ARM__
	err = arch_set_default_video_mode();
	dprintf("set video mode: 0x%08x\n", err);
	if (err < B_OK)
		return;
#endif

	err = video_display_splash((addr_t)gFrameBufferBase);
	dprintf("video_display_splash: 0x%08x\n", err);
	#warning U-Boot:TODO
}


extern "C" void
platform_switch_to_text_mode(void)
{
	TRACE(("%s()\n", __FUNCTION__));
	#warning U-Boot:TODO
}


extern "C" status_t
platform_init_video(void)
{
	TRACE(("%s()\n", __FUNCTION__));
    	#warning U-Boot:TODO
#ifdef __ARM__
	arch_probe_video_mode();
#endif
	//XXX for testing
	//platform_switch_to_logo();
	//return arch_probe_video_mode();
	//return B_OK;
}

