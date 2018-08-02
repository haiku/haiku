/*
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "rom_calls.h"
#include "console.h"
#include "video.h"
//#include "mmu.h"
//#include "images.h"

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
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

static addr_t sFrameBuffer;


static void
probe_video_mode()
{
	if (gScreen == NULL) {
		gKernelArgs.frame_buffer.enabled = false;
		return;
	}
	/*
	if (gScreen->RastPort.BitMap->Depth < 8) {
		gKernelArgs.frame_buffer.enabled = false;
		return;
	}
	*/

	/*
	dprintf("Video mode:\n");
	dprintf("BytesPerRow %d\n", gScreen->RastPort.BitMap->BytesPerRow);
	dprintf("Rows %d\n", gScreen->RastPort.BitMap->Rows);
	dprintf("Flags %02x\n", gScreen->RastPort.BitMap->Flags);
	dprintf("Depth %d\n", gScreen->RastPort.BitMap->Depth);
	for (int i = 0; i < 8; i++)
		dprintf("Planes[%d] %p\n", i, gScreen->RastPort.BitMap->Planes[i]);
	*/

	//XXX how do we tell it's a planar framebuffer ??

	gKernelArgs.frame_buffer.width = gScreen->RastPort.BitMap->BytesPerRow * 8;
	gKernelArgs.frame_buffer.height = gScreen->RastPort.BitMap->Rows;
	gKernelArgs.frame_buffer.bytes_per_row = gScreen->RastPort.BitMap->BytesPerRow;
	gKernelArgs.frame_buffer.depth = gScreen->RastPort.BitMap->Depth;
	gKernelArgs.frame_buffer.physical_buffer.size
		= gKernelArgs.frame_buffer.width
		* gKernelArgs.frame_buffer.height
		* gScreen->RastPort.BitMap->Depth / 8;
	gKernelArgs.frame_buffer.physical_buffer.start
		= (phys_addr_t)(gScreen->RastPort.BitMap->Planes[0]);

	dprintf("video mode: %ux%ux%u\n", gKernelArgs.frame_buffer.width,
		gKernelArgs.frame_buffer.height, gKernelArgs.frame_buffer.depth);

	gKernelArgs.frame_buffer.enabled = true;
}


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
		if (info.PropertyFlags & DIPF_IS_HAM)
			continue;
		if (info.PropertyFlags & DIPF_IS_DUALPF)
			continue;
		if (dimension.MaxDepth < 4)
			continue;
		// skip 5 & 6 bit modes
		if (dimension.MaxDepth < 8 && dimension.MaxDepth != 4)
			continue;
		//dprintf("name: %s\n", name.Name);
		/*
		dprintf("mode 0x%08lx: %dx%d flags: 0x%08lx bpp: %d\n",
			modeID, info.Resolution.x, info.Resolution.y, info.PropertyFlags,
			info.RedBits + info.GreenBits + info.BlueBits);
		dprintf("mode: %dx%d -> %dx%d\n",
			dimension.MinRasterWidth, dimension.MinRasterHeight,
			dimension.MaxRasterWidth, dimension.MaxRasterHeight);
		*/
		dprintf("mode: %dx%d %dbpp flags: 0x%08lx\n",
			dimension.Nominal.MaxX - dimension.Nominal.MinX + 1,
			dimension.Nominal.MaxY - dimension.Nominal.MinY + 1,
			dimension.MaxDepth, info.PropertyFlags);
		
		char label[128];
		sprintf(label, "%ux%u %u bit %08lx%s%s",
			dimension.Nominal.MaxX - dimension.Nominal.MinX + 1,
			dimension.Nominal.MaxY - dimension.Nominal.MinY + 1,
			dimension.MaxDepth, info.PropertyFlags,
			(info.PropertyFlags & DIPF_IS_LACE) ? " i" : "",
			(info.PropertyFlags & DIPF_IS_PAL) ? " pal" : "");

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


//	#pragma mark - blit


extern "C" void
platform_blit4(addr_t frameBuffer, const uint8 *data,
	uint16 width, uint16 height, uint16 imageWidth, uint16 left, uint16 top)
{
	if (!data)
		return;
	// TODO
}


extern "C" void
platform_set_palette(const uint8 *palette)
{
	switch (gKernelArgs.frame_buffer.depth) {
		case 4:
			//vga_set_palette((const uint8 *)kPalette16, 0, 16);
			break;
		case 8:
			//if (vesa_set_palette((const uint8 *)palette, 0, 256) != B_OK)
			//	dprintf("set palette failed!\n");

			break;
		default:
			break;
	}
}


//	#pragma mark -


extern "C" void
platform_switch_to_logo(void)
{
	return;
	// in debug mode, we'll never show the logo
	if ((platform_boot_options() & BOOT_OPTION_DEBUG_OUTPUT) != 0)
		return;

	addr_t lastBase = gKernelArgs.frame_buffer.physical_buffer.start;
	size_t lastSize = gKernelArgs.frame_buffer.physical_buffer.size;

	// TODO: implement me
	
	probe_video_mode();

	// map to virtual memory
	// (should be ok in bootloader thanks to TT0)
	
	sFrameBuffer = gKernelArgs.frame_buffer.physical_buffer.start;

	//video_display_splash(sFrameBuffer);

}


extern "C" void
platform_switch_to_text_mode(void)
{
	// TODO: implement me
	// force Intuition to redraw everything
	RemakeDisplay();
}


extern "C" status_t
platform_init_video(void)
{
	// TODO: implement me
	probe_video_mode();

	return B_OK;
}

