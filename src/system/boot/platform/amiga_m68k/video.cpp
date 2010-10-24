/*
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "rom_calls.h"
#include "video.h"
//#include "mmu.h"
//#include "images.h"

#include <arch/cpu.h>
#include <boot/stage2.h>
#include <boot/platform.h>
#include <boot/menu.h>
#include <boot/kernel_args.h>
#include <util/list.h>
#include <drivers/driver_settings.h>
#include <GraphicsDefs.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//#define TRACE_VIDEO
#ifdef TRACE_VIDEO
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif





//	#pragma mark -


bool
video_mode_hook(Menu *menu, MenuItem *item)
{
	// nothing yet
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

/*
	video_mode *mode = NULL;
	while ((mode = (video_mode *)list_get_next_item(&sModeList, mode)) != NULL) {
		char label[64];
		sprintf(label, "%ux%u %u bit", mode->width, mode->height,
			mode->bits_per_pixel);

		menu->AddItem(item = new(nothrow) MenuItem(label));
		item->SetData(mode);
	}
*/
#if 1
	uint32 modeID = INVALID_ID;
	while ((modeID = NextDisplayInfo(modeID)) != INVALID_ID) {
		//DisplayInfoHandle handle = FindDisplayInfo(modeID);
		//if (handle == NULL)
		//	continue;
		struct DisplayInfo info;
		struct DimensionInfo dimension;
		struct NameInfo name;
		if (GetDisplayInfoData(NULL, (uint8 *)&info, sizeof(info),
			DTAG_DISP, modeID) < 48/*sizeof(struct DisplayInfo)*/)
			continue;
		if (GetDisplayInfoData(NULL, (uint8 *)&dimension, sizeof(dimension),
			DTAG_DIMS, modeID) < 66)
			continue;
		/*if (GetDisplayInfoData(NULL, (uint8 *)&name, sizeof(name),
			DTAG_NAME, modeID) < sizeof(name) - 8)
			continue;*/
		if (info.NotAvailable)
			continue;
		if (dimension.MaxDepth < 4)
			continue;
		//dprintf("name: %s\n", name.Name);
		/*
		dprintf("mode 0x%08lx: %dx%d flags: 0x%08lx bpp: %d\n",
			modeID, info.Resolution.x, info.Resolution.y, info.PropertyFlags,
			info.RedBits + info.GreenBits + info.BlueBits);
		dprintf("mode: %dx%d -> %dx%d\n",
			dimension.MinRasterWidth, dimension.MinRasterHeight,
			dimension.MaxRasterWidth, dimension.MaxRasterHeight);
		dprintf("mode: %dx%d %dbpp flags: 0x%08lx\n",
			dimension.Nominal.MaxX - dimension.Nominal.MinX + 1,
			dimension.Nominal.MaxY - dimension.Nominal.MinY + 1,
			dimension.MaxDepth, info.PropertyFlags);
		*/
		char label[64];
		sprintf(label, "%ux%u %u bit",
			dimension.Nominal.MaxX - dimension.Nominal.MinX + 1,
			dimension.Nominal.MaxY - dimension.Nominal.MinY + 1,
			dimension.MaxDepth);

		menu->AddItem(item = new(nothrow) MenuItem(label));
		item->SetData((void *)modeID);
	}
	
#endif
	dprintf("done\n");

	menu->AddSeparatorItem();
	menu->AddItem(item = new(nothrow) MenuItem("Return to main menu"));
	item->SetType(MENU_ITEM_NO_CHOICE);

	return menu;
}


//	#pragma mark -


extern "C" void
platform_switch_to_logo(void)
{
	// TODO: implement me
}


extern "C" void
platform_switch_to_text_mode(void)
{
	// TODO: implement me
}


extern "C" status_t
platform_init_video(void)
{
	// TODO: implement me
	return B_OK;
}

