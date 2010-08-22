/*
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2010 Andreas Färber <andreas.faerber@web.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <boot/platform.h>
#include <boot/stage2.h>
#include <boot/platform/generic/video.h>
#include <edid.h>
#include <platform/openfirmware/openfirmware.h>


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
			if (of_call_method(sScreen, "set-colors", 3, 0,
					256, 0, palette) == OF_FAILED) {
				for (int index = 0; index < 256; index++) {
					of_call_method(sScreen, "color!", 4, 0, index,
						palette[index * 3 + 2],
						palette[index * 3 + 1],
						palette[index * 3 + 0]);
				}
			}
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

	sScreen = of_open("screen");
	if (sScreen == OF_FAILED)
		return;

	int package = of_instance_to_package(sScreen);
	if (package == OF_FAILED)
		return;
	uint32 width, height;
	if (of_call_method(sScreen, "dimensions", 0, 2, &height, &width)
			== OF_FAILED) {
		if (of_getprop(package, "width", &width, sizeof(int32)) == OF_FAILED)
			return;
		if (of_getprop(package, "height", &height, sizeof(int32)) == OF_FAILED)
			return;
	}
	uint32 depth;
	if (of_getprop(package, "depth", &depth, sizeof(uint32)) == OF_FAILED)
		return;
	uint32 lineBytes;
	if (of_getprop(package, "linebytes", &lineBytes, sizeof(uint32))
			== OF_FAILED)
		return;
	uint32 address;
		// address is always 32 bit
	if (of_getprop(package, "address", &address, sizeof(uint32)) == OF_FAILED)
		return;
	gKernelArgs.frame_buffer.physical_buffer.start = address;
	gKernelArgs.frame_buffer.physical_buffer.size = lineBytes * height;
	gKernelArgs.frame_buffer.width = width;
	gKernelArgs.frame_buffer.height = height;
	gKernelArgs.frame_buffer.depth = depth;
	gKernelArgs.frame_buffer.bytes_per_row = lineBytes;

	dprintf("video mode: %ux%ux%u\n", gKernelArgs.frame_buffer.width,
		gKernelArgs.frame_buffer.height, gKernelArgs.frame_buffer.depth);

	gKernelArgs.frame_buffer.enabled = true;

	// the memory will be identity-mapped already
	video_display_splash(gKernelArgs.frame_buffer.physical_buffer.start);
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

	int screen = of_finddevice("screen");
	if (screen == OF_FAILED)
		return B_NO_INIT;
	edid1_raw edidRaw;
	if (of_getprop(screen, "EDID", &edidRaw, sizeof(edidRaw)) != OF_FAILED) {
		edid1_info info;
		edid_decode(&info, &edidRaw);
#ifdef TRACE_VIDEO
		edid_dump(&info);
#endif
		gKernelArgs.edid_info = kernel_args_malloc(sizeof(edid1_info));
		if (gKernelArgs.edid_info != NULL)
			memcpy(gKernelArgs.edid_info, &info, sizeof(edid1_info));
	}

	return B_OK;
}

