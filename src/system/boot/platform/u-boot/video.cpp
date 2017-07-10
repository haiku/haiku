/*
 * Copyright 2009, Johannes Wischert
 * Distributed under the terms of the MIT License.
 */


#include "video.h"

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

#include "arch_framebuffer.h"


ArchFramebuffer *gFramebuffer = NULL;


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
platform_blit4(addr_t frameBuffer, const uint8 *data, uint16 width,
	uint16 height, uint16 imageWidth, uint16 left, uint16 top)
{
}


extern "C" void
platform_switch_to_logo(void)
{
	// in debug mode, we'll never show the logo
	if ((platform_boot_options() & BOOT_OPTION_DEBUG_OUTPUT) != 0)
		return;

	status_t err;

	if (gFramebuffer != NULL) {
		err = gFramebuffer->SetDefaultMode();
		if (err < B_OK) {
			ERROR("Framebuffer SetDefaultMode failed!\n");
			return;
		}

		err = video_display_splash(gFramebuffer->Base());
	}
}


extern "C" void
platform_switch_to_text_mode(void)
{
}


extern "C" status_t
platform_init_video(void)
{

#warning TODO: Fix u-boot arm framebuffer location from fdt!
#ifdef __ARM__
	#if defined(BOARD_CPU_ARM920T)
		extern ArchFramebuffer *arch_get_fb_arm_920(addr_t base);
		gFramebuffer = arch_get_fb_arm_920(0x88000000);
	#elif defined(BOARD_CPU_BCM2835) || defined(BOARD_CPU_BCM2836)
		extern ArchFramebuffer *arch_get_fb_arm_bcm2835(addr_t base);
		// BCM2835/BCM2836 gets their framebuffer base from a Mailbox
		gFramebuffer = arch_get_fb_arm_bcm2835(0x0);
	#elif defined(BOARD_CPU_OMAP3)
		extern ArchFramebuffer *arch_get_fb_arm_omap3(addr_t base);
		gFramebuffer = arch_get_fb_arm_omap3(FB_BASE);
	#elif defined(BOARD_CPU_PXA270)
		ArchFramebuffer *arch_get_fb_arm_pxa270(addr_t base);
		gFramebuffer = arch_get_fb_arm_pxa270(0xA3000000);
	#endif
#endif

	if (gFramebuffer == NULL) {
		ERROR("No framebuffer device found!\n");
		return B_ERROR;
	}

	status_t result = gFramebuffer->Probe();
	if (result != B_OK)
		return result;

	gFramebuffer->Init();
	if (result != B_OK)
		return result;

	return B_OK;
}
