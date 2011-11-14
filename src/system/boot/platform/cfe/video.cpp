/*
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2010 Andreas Färber <andreas.faerber@web.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <boot/platform.h>
#include <boot/stage2.h>
#include <boot/platform/generic/video.h>
#include <edid.h>
#include <boot/platform/cfe/cfe.h>


//#define TRACE_VIDEO


static int sScreen;


void
platform_blit4(addr_t frameBuffer, const uint8 *data,
	uint16 width, uint16 height, uint16 imageWidth, uint16 left, uint16 top)
{
	panic("platform_blit4(): not implemented\n");
}


extern "C" void
platform_set_palette(const uint8 *palette)
{
	switch (gKernelArgs.frame_buffer.depth) {
		case 8:
			break;
		default:
			break;
	}
}


extern "C" void
platform_switch_to_logo(void)
{
	// in debug mode, we'll never show the logo
	if ((platform_boot_options() & BOOT_OPTION_DEBUG_OUTPUT) != 0)
		return;

	return;
}


extern "C" void
platform_switch_to_text_mode(void)
{
	// nothing to do if we're in text mode
	if (!gKernelArgs.frame_buffer.enabled)
		return;

	// ToDo: implement me

	gKernelArgs.frame_buffer.enabled = false;
}


extern "C" status_t
platform_init_video(void)
{
	gKernelArgs.frame_buffer.enabled = false;

	return B_NO_INIT;
}

