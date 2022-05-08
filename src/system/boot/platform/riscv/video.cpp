/*
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "console.h"
#include "video.h"
//#include "mmu.h"
//#include "images.h"
#include "graphics.h"
#include "virtio.h"
#include "FwCfg.h"

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


//#define TRACE_VIDEO
#ifdef TRACE_VIDEO
# define TRACE(x...) dprintf(x)
#else
# define TRACE(x...) ;
#endif


//	#pragma mark -

bool
video_mode_hook(Menu* menu, MenuItem* item)
{
	// nothing yet
	return true;
}


Menu*
video_mode_menu()
{
	Menu* menu = new(nothrow) Menu(CHOICE_MENU, "Select Video Mode");
	MenuItem* item;

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


//	#pragma mark - blit


extern "C" void
platform_blit4(addr_t frameBuffer, const uint8* data,
	uint16 width, uint16 height, uint16 imageWidth, uint16 left, uint16 top)
{
}


extern "C" void
platform_set_palette(const uint8* palette)
{
}


//	#pragma mark -


extern "C" void
platform_switch_to_logo(void)
{
	gKernelArgs.frame_buffer.physical_buffer.start = (addr_t)gFramebuf.colors;
	gKernelArgs.frame_buffer.physical_buffer.size
		= 4 * gFramebuf.stride * gFramebuf.height;
	gKernelArgs.frame_buffer.width = gFramebuf.width;
	gKernelArgs.frame_buffer.height = gFramebuf.height;
	gKernelArgs.frame_buffer.depth = 32;
	gKernelArgs.frame_buffer.bytes_per_row = 4 * gFramebuf.stride;
	gKernelArgs.frame_buffer.enabled = gFramebuf.width > 0 && gFramebuf.height > 0;

	video_display_splash(gKernelArgs.frame_buffer.physical_buffer.start);
}


extern "C" void
platform_switch_to_text_mode(void)
{
	gKernelArgs.frame_buffer.enabled = false;
}


extern "C" status_t
platform_init_video(void)
{
	FwCfg::Init();
	virtio_init(); // we want heap initalized
	Clear(gFramebuf, 0xff000000);
	console_init();
	return B_OK;
}
