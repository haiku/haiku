/*
 * Copyright 2008-2010, François Revol, revol@free.fr. All rights reserved.
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "nextrom.h"
#include "video.h"
#include "mmu.h"
//#include "images.h"

#include <algorithm>

#include <arch/cpu.h>
#include <boot/stage2.h>
#include <boot/platform.h>
#include <boot/menu.h>
#include <boot/kernel_args.h>
#include <boot/platform/generic/video.h>
#include <util/list.h>
#include <drivers/driver_settings.h>
#include <GraphicsDefs.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define TRACE_VIDEO
#ifdef TRACE_VIDEO
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


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

	//menu->AddItem(new(nothrow) MenuItem("Standard VGA"));

	menu->AddSeparatorItem();
	menu->AddItem(item = new(nothrow) MenuItem("Return to main menu"));
	item->SetType(MENU_ITEM_NO_CHOICE);

	return menu;
}


void
platform_blit4(addr_t frameBuffer, const uint8 *data,
	uint16 width, uint16 height, uint16 imageWidth, uint16 left, uint16 top)
{
	if (!data)
		return;
}


extern "C" void
platform_set_palette(const uint8 *palette)
{
}


//	#pragma mark -


extern "C" void
platform_switch_to_logo(void)
{
	// in debug mode, we'll never show the logo
	if ((platform_boot_options() & BOOT_OPTION_DEBUG_OUTPUT) != 0)
		return;

	// I believe we should use mg->km_coni.fb_num but turbo color has 0 here !?
	char fb_num = KM_CON_FRAMEBUFFER;

	gKernelArgs.frame_buffer.width = mg->km_coni.dspy_w;
	gKernelArgs.frame_buffer.height = mg->km_coni.dspy_h;
	gKernelArgs.frame_buffer.bytes_per_row = mg->km_coni.bytes_per_scanline;
	// we fake 2bpp as 4bpp for simplicity
	gKernelArgs.frame_buffer.depth =
		std::max(4, 32 / mg->km_coni.pixels_per_word);
	gKernelArgs.frame_buffer.physical_buffer.size =
		mg->km_coni.map_addr[fb_num].size;
	gKernelArgs.frame_buffer.physical_buffer.start =
		mg->km_coni.map_addr[fb_num].phys_addr;

	//TODO: pass a custom color_space in the KMessage?

	gKernelArgs.frame_buffer.enabled = true;
	video_display_splash(mg->km_coni.map_addr[fb_num].virt_addr);
}


extern "C" void
platform_switch_to_text_mode(void)
{
	if (!gKernelArgs.frame_buffer.enabled) {
		return;
	}

	gKernelArgs.frame_buffer.enabled = false;
}


extern "C" status_t
platform_init_video(void)
{
	gKernelArgs.frame_buffer.enabled = false;

	return B_OK;
}

